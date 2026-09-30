/**
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "connect_http.h"

#include "pico/rpi_connect_util.h"
#include "lwip/altcp.h"
#include "lwip/altcp_tls.h"
#include "mbedtls/base64.h"
#include "mbedtls/ssl.h"

// Pi Connect signs its request headers, so the lwIP HTTP client must not add
// any of its own, and the event stream (server-sent events) needs the server
// to keep the connection open. Both are lwIP options (Raspberry Pi lwIP fork)
// that default to the upstream behaviour, so the application must turn them
// off in its lwipopts.h.
#if !defined(HTTPC_SEND_ACCEPT_HEADER) || HTTPC_SEND_ACCEPT_HEADER
#error "pico_rpi_connect requires '#define HTTPC_SEND_ACCEPT_HEADER 0' in lwipopts.h"
#endif
#if !defined(HTTPC_SEND_CONNECTION_CLOSE) || HTTPC_SEND_CONNECTION_CLOSE
#error "pico_rpi_connect requires '#define HTTPC_SEND_CONNECTION_CLOSE 0' in lwipopts.h"
#endif

static const char *http_client_hostname;

static err_t http_request_start(connect_http_request_t *req, const char *hostname, const char *uri, u16_t data_len, const char *data);
static void request_done(connect_http_request_t *req, httpc_result_t httpc_result, u32_t rx_content_len, u32_t srv_res, err_t err);

static const char *current_hostname(connect_http_request_t *req) {
    return req->redirect_host ? req->redirect_host : http_client_hostname;
}

// Find a header in a CRLF separated response header block, ignoring case.
// Returns the value with surrounding whitespace removed, and its length.
static const char *find_header(const char *hdrs, size_t hdrs_len, const char *name, size_t *value_len) {
    size_t name_len = strlen(name);
    const char *end = hdrs + hdrs_len;
    const char *line = hdrs;
    while (line < end) {
        const char *eol = line;
        while (eol < end && *eol != '\r' && *eol != '\n')
            eol++;
        if ((size_t)(eol - line) > name_len && line[name_len] == ':' && strncasecmp(line, name, name_len) == 0) {
            const char *value = line + name_len + 1;
            while (value < eol && (*value == ' ' || *value == '\t'))
                value++;
            const char *value_end = eol;
            while (value_end > value && (value_end[-1] == ' ' || value_end[-1] == '\t'))
                value_end--;
            *value_len = (size_t)(value_end - value);
            return value;
        }
        line = eol;
        while (line < end && (*line == '\r' || *line == '\n'))
            line++;
    }
    return NULL;
}

typedef enum {
    REDIRECT_NONE,   // not a redirect, handle the response normally
    REDIRECT_FOLLOW, // redirect target stored in req, abort and follow it
    REDIRECT_ERROR,  // redirect that cannot be followed, abort the request
} redirect_action_t;

// Check whether a response is a redirect, and if so store the target in req.
// Absolute URLs with the same scheme as the current request, scheme-relative
// URLs and absolute paths are supported; https to http downgrades and
// relative paths are not.
static redirect_action_t parse_redirect(connect_http_request_t *req, struct pbuf *hdr, u16_t hdr_len) {
    // Status line is "HTTP/1.x NNN ..."
    char status_line[13] = {0};
    if (pbuf_copy_partial(hdr, status_line, 12, 0) != 12 || strncmp(status_line, "HTTP/", 5) != 0)
        return REDIRECT_NONE;
    int status = atoi(status_line + 9);
    if (status != 301 && status != 302 && status != 303 && status != 307 && status != 308)
        return REDIRECT_NONE;

    char *hdrs = malloc(hdr_len);
    if (!hdrs)
        return REDIRECT_ERROR;
    u16_t len = pbuf_copy_partial(hdr, hdrs, hdr_len, 0);

    redirect_action_t action = REDIRECT_ERROR;
    char *host = NULL;
    char *uri = NULL;
    size_t loc_len;
    const char *loc = find_header(hdrs, len, "Location", &loc_len);
    if (!loc) {
        // Nothing to follow, so return the response to the caller
        action = REDIRECT_NONE;
        goto done;
    }
    if (req->redirect_count >= req->max_redirects) {
        RPI_CONNECT_ERROR("Too many redirects\n");
        goto done;
    }
    // The final response's certificate is checked when the request completes,
    // but a redirect is acted on here, so check that it came from a verified server
    if (req->ca_cert && req->tls_pcb) {
        mbedtls_ssl_context *ssl = altcp_tls_context(req->tls_pcb);
        if (!ssl || mbedtls_ssl_get_verify_result(ssl) != 0) {
            RPI_CONNECT_ERROR("Certificate verification failed for redirect\n");
            goto done;
        }
    }
    RPI_CONNECT_DEBUG("Redirect %d to %.*s\n", status, (int)loc_len, loc);

    // The fragment is not sent to the server
    const char *loc_end = memchr(loc, '#', loc_len);
    if (!loc_end)
        loc_end = loc + loc_len;

    const char *scheme = req->tls_config ? "https://" : "http://";
    const char *authority = NULL;
    if ((size_t)(loc_end - loc) >= strlen(scheme) && strncasecmp(loc, scheme, strlen(scheme)) == 0) {
        authority = loc + strlen(scheme);
    } else if (loc_end - loc >= 2 && loc[0] == '/' && loc[1] == '/') {
        authority = loc + 2;
    } else if (loc[0] != '/') {
        RPI_CONNECT_ERROR("Unsupported redirect location %.*s\n", (int)loc_len, loc);
        goto done;
    }

    const char *path = loc;
    u16_t port = req->port;
    if (authority) {
        const char *authority_end = authority;
        while (authority_end < loc_end && *authority_end != '/' && *authority_end != '?')
            authority_end++;
        const char *host_end = memchr(authority, ':', (size_t)(authority_end - authority));
        if (host_end) {
            char *port_end;
            unsigned long p = strtoul(host_end + 1, &port_end, 10);
            if (port_end != authority_end || p == 0 || p > 0xFFFF) {
                RPI_CONNECT_ERROR("Invalid port in redirect location %.*s\n", (int)loc_len, loc);
                goto done;
            }
            port = (u16_t)p;
        } else {
            host_end = authority_end;
            port = req->tls_config ? 443 : 80;
        }
        if (host_end == authority || memchr(authority, '@', (size_t)(host_end - authority)) || authority[0] == '[') {
            RPI_CONNECT_ERROR("Unsupported host in redirect location %.*s\n", (int)loc_len, loc);
            goto done;
        }
        host = strndup(authority, (size_t)(host_end - authority));
        path = authority_end;
    } else {
        host = strdup(current_hostname(req));
    }

    // An empty path, e.g. "https://host" or "https://host?query", means "/"
    size_t path_len = (size_t)(loc_end - path);
    bool add_slash = path_len == 0 || path[0] != '/';
    uri = malloc(path_len + add_slash + 1);
    if (!host || !uri)
        goto done;
    if (add_slash)
        uri[0] = '/';
    memcpy(uri + add_slash, path, path_len);
    uri[path_len + add_slash] = '\0';

    if (strcasecmp(host, current_hostname(req)) != 0) {
        // Don't leak credentials or request signatures to a different host,
        // and TLS sessions can only be resumed with the host that issued them
        req->extra_headers_fn = NULL;
        req->extra_headers_arg = NULL;
        req->tls_session = NULL;
    }

    free(req->redirect_host);
    free(req->redirect_uri);
    req->redirect_host = host;
    req->redirect_uri = uri;
    req->port = port;
    req->redirect_count++;
    req->redirect_pending = true;
    host = NULL;
    uri = NULL;
    action = REDIRECT_FOLLOW;

done:
    free(host);
    free(uri);
    free(hdrs);
    return action;
}

static err_t internal_header_fn(__unused httpc_state_t *connection, void *arg, struct pbuf *hdr, u16_t hdr_len, u32_t content_len) {
    connect_http_request_t *req = (connect_http_request_t*)arg;
    if (req && req->max_redirects) {
        // Abort rather than reading the body of a redirect: the server may keep
        // the connection open, and the body must not reach the caller
        if (parse_redirect(req, hdr, hdr_len) != REDIRECT_NONE)
            return ERR_ABRT;
    }
    if (req)
        req->content_len = content_len;
#if RPI_CONNECT_VERBOSE_DEBUG_ENABLE
    assert(arg);
    RPI_CONNECT_VERBOSE_DEBUG("\nheaders %u\n", hdr_len);
    u16_t offset = 0;
    while (offset < hdr->tot_len && offset < hdr_len) {
        char c = (char)pbuf_get_at(hdr, offset++);
        RPI_CONNECT_VERBOSE_DEBUG("%c", c);
    }
#endif
    if (req && req->headers_fn)
        req->headers_fn(req, hdr, hdr_len);
    return ERR_OK;
}

static err_t internal_recv_fn(void *arg, struct altcp_pcb *conn, struct pbuf *p, err_t err) {
    connect_http_request_t *req = (connect_http_request_t*)arg;
    assert(arg);
    if (err != 0) {
        RPI_CONNECT_ERROR("\ncontent err %d\n", err);
    }
#if RPI_CONNECT_VERBOSE_DEBUG_ENABLE
    u16_t offset = 0;
    while (offset < p->tot_len) {
        char c = (char)pbuf_get_at(p, offset++);
        RPI_CONNECT_VERBOSE_DEBUG("%c", c);
    }
    RPI_CONNECT_VERBOSE_DEBUG("\r");
    RPI_CONNECT_VERBOSE_DEBUG("\n");
#endif
    if (req->recv_fn) {
        u16_t offset = 0;
        while (offset < p->tot_len) {
            char buffer[128];
            u16_t remaining = p->tot_len - offset;
            u16_t to_copy = remaining < sizeof(buffer) ? remaining : (u16_t)sizeof(buffer);
            char *buf = pbuf_get_contiguous(p, buffer, sizeof(buffer), to_copy, offset);
            req->recv_fn(req->recv_fn_arg, buf, to_copy);
            offset += to_copy;
        }
    }
    altcp_recved(conn, p->tot_len);
    pbuf_free(p);
    return ERR_OK;
}

// Called with the async_context lock held
static void redirect_worker_fn(__unused async_context_t *context, async_at_time_worker_t *worker) {
    connect_http_request_t *req = (connect_http_request_t *)worker->user_data;
    err_t ret = http_request_start(req, current_hostname(req), req->redirect_uri, 0, NULL);
    if (ret != ERR_OK)
        request_done(req, HTTPC_RESULT_ERR_UNKNOWN, 0, 0, ret);
}

static void internal_result_fn(void *arg, httpc_result_t httpc_result, u32_t rx_content_len, u32_t srv_res, err_t err) {
    assert(arg);
    connect_http_request_t *req = (connect_http_request_t*)arg;
    RPI_CONNECT_DEBUG("Request completed with result %d len %" PRIu32 " server_response %" PRIu32 " err %d in %" PRIu32 "ms\n", httpc_result, rx_content_len, srv_res, err,
        us_to_ms(absolute_time_diff_us(req->before_request, get_absolute_time())));
    if (req->redirect_pending) {
        // The redirect response was aborted after its headers. httpc frees the
        // old connection after we return, so start the request to the new
        // location from a worker: two TLS connections may not fit in memory.
        req->redirect_pending = false;
        req->http_state = NULL;
        req->tls_pcb = NULL;
        req->redirect_worker.do_work = redirect_worker_fn;
        req->redirect_worker.user_data = req;
        if (async_context_add_at_time_worker_in_ms(req->context, &req->redirect_worker, 0))
            return;
        RPI_CONNECT_ERROR("Failed to schedule redirect\n");
        httpc_result = HTTPC_RESULT_ERR_UNKNOWN;
        err = ERR_MEM;
    }
    request_done(req, httpc_result, rx_content_len, srv_res, err);
}

static void request_done(connect_http_request_t *req, httpc_result_t httpc_result, u32_t rx_content_len, u32_t srv_res, err_t err) {
    if (req->result_fn)
        req->result_fn(req, httpc_result, rx_content_len, srv_res, err);
    // Save the TLS session for resumption on the next request. The pcb is
    // still alive at this point; httpc_free_state() will close it after we
    // return. NewSessionTicket records arrive before this callback in TLS 1.3.
    if (req->tls_session && req->tls_pcb && httpc_result == HTTPC_RESULT_OK) {
        if (altcp_tls_get_session(req->tls_pcb, req->tls_session) == ERR_OK) {
            RPI_CONNECT_INFO("TLS session saved\n");
        }
    }
    req->result = httpc_result;
    req->complete = true;
    RPI_CONNECT_DEBUG("%s - complete\n", __func__);
}

// Override altcp_tls_alloc to set sni
static struct altcp_pcb *altcp_tls_alloc_sni(void *arg, u8_t ip_type) {
    assert(arg);
    connect_http_request_t *req = (connect_http_request_t*)arg;
    struct altcp_pcb *pcb = altcp_tls_alloc(req->tls_config, ip_type);
    if (!pcb) {
        RPI_CONNECT_ERROR("Failed to allocate PCB\n");
        return NULL;
    }
    mbedtls_ssl_set_hostname(altcp_tls_context(pcb), current_hostname(req));
    if (req->tls_session) {
        // Best-effort: ERR_VAL on the first call when no session has been saved.
        if (altcp_tls_set_session(pcb, req->tls_session) == ERR_OK) {
            RPI_CONNECT_INFO("TLS session restored\n");
        }
    }
    req->tls_pcb = pcb;
    return pcb;
}

static err_t http_request_start(connect_http_request_t *req, const char *hostname, const char *uri, u16_t data_len, const char *data) {
#if LWIP_ALTCP
    if (req->tls_config) {
        if (!req->tls_allocator.alloc) {
            req->tls_allocator.alloc = altcp_tls_alloc_sni;
            req->tls_allocator.arg = req;
        }
        req->settings.altcp_allocator = &req->tls_allocator;
    }
    req->tls_pcb = NULL;
#endif
    req->complete = false;
    req->content_len = CONNECT_HTTP_CONTENT_LEN_UNKNOWN;
    req->settings.extra_headers_fn = req->extra_headers_fn;
    req->settings.extra_headers_arg = req->extra_headers_arg;
    req->settings.headers_done_fn = internal_header_fn;
    req->settings.result_fn = internal_result_fn;
    req->before_request = get_absolute_time();
    err_t ret;
    if (data_len == 0) {
        ret = httpc_get_file_dns(hostname, req->port, uri, &req->settings, internal_recv_fn, req, &req->http_state);
    } else {
        ret = httpc_post_file_dns(hostname, req->port, uri, &req->settings, internal_recv_fn, req, data_len, data, &req->http_state);
    }
    if (ret != ERR_OK) {
        RPI_CONNECT_ERROR("http request failed: %d\n", ret);
    }
    return ret;
}

// Make a http request, complete when req->complete returns true
int rpi_connect_http_request_async(async_context_t *context, connect_http_request_t *req, const char *uri, u16_t data_len, const char *data) {
#if LWIP_ALTCP
    req->port = req->tls_config ? 443 : 80;
#else
    req->port = 80;
#endif
    req->redirect_count = 0;
    req->redirect_pending = false;
    free(req->redirect_host);
    req->redirect_host = NULL;
    req->context = context;
    async_context_acquire_lock_blocking(context);
    err_t ret = http_request_start(req, http_client_hostname, uri, data_len, data);
    async_context_release_lock(context);
    return ret;
}

void rpi_connect_http_request_free(connect_http_request_t *req) {
    if (req->context)
        async_context_remove_at_time_worker(req->context, &req->redirect_worker);
    free(req->redirect_host);
    req->redirect_host = NULL;
    free(req->redirect_uri);
    req->redirect_uri = NULL;
}

void rpi_connect_http_set_hostname(const char *hostname) {
    http_client_hostname = hostname;
}
