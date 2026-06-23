/**
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "connect_http.h"

#include "pico/rpi_connect_util.h"
#include "lwip/altcp.h"
#include "lwip/altcp_tls.h"
#include "mbedtls/base64.h"

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

static err_t internal_header_fn(__unused httpc_state_t *connection, void *arg, struct pbuf *hdr, u16_t hdr_len, u32_t content_len) {
    connect_http_request_t *req = (connect_http_request_t*)arg;
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

static void internal_result_fn(void *arg, httpc_result_t httpc_result, u32_t rx_content_len, u32_t srv_res, err_t err) {
    assert(arg);
    connect_http_request_t *req = (connect_http_request_t*)arg;
    RPI_CONNECT_DEBUG("Request completed with result %d len %" PRIu32 " server_response %" PRIu32 " err %d in %" PRIu32 "ms\n", httpc_result, rx_content_len, srv_res, err,
        us_to_ms(absolute_time_diff_us(req->before_request, get_absolute_time())));
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
    mbedtls_ssl_set_hostname(altcp_tls_context(pcb), http_client_hostname);
    if (req->tls_session) {
        // Best-effort: ERR_VAL on the first call when no session has been saved.
        if (altcp_tls_set_session(pcb, req->tls_session) == ERR_OK) {
            RPI_CONNECT_INFO("TLS session restored\n");
        }
    }
    req->tls_pcb = pcb;
    return pcb;
}

// Make a http request, complete when req->complete returns true
int rpi_connect_http_request_async(async_context_t *context, connect_http_request_t *req, const char *uri, u16_t data_len, const char *data) {
#if LWIP_ALTCP
    const uint16_t default_port = req->tls_config ? 443 : 80;
    if (req->tls_config) {
        if (!req->tls_allocator.alloc) {
            req->tls_allocator.alloc = altcp_tls_alloc_sni;
            req->tls_allocator.arg = req;
        }
        req->settings.altcp_allocator = &req->tls_allocator;
    }
#else
    const uint16_t default_port = 80;
#endif
    req->complete = false;
    req->content_len = CONNECT_HTTP_CONTENT_LEN_UNKNOWN;
    req->settings.extra_headers_fn = req->extra_headers_fn;
    req->settings.extra_headers_arg = req->extra_headers_arg;
    req->settings.headers_done_fn = internal_header_fn;
    req->settings.result_fn = internal_result_fn;
    req->context = context;
    req->before_request = get_absolute_time();
    async_context_acquire_lock_blocking(context);
    err_t ret;
    if (data_len == 0) {
        ret = httpc_get_file_dns(http_client_hostname, default_port, uri, &req->settings, internal_recv_fn, req, &req->http_state);
    } else {
        ret = httpc_post_file_dns(http_client_hostname, default_port, uri, &req->settings, internal_recv_fn, req, data_len, data, &req->http_state);
    }
    async_context_release_lock(context);
    if (ret != ERR_OK) {
        RPI_CONNECT_ERROR("http request failed: %d\n", ret);
    }
    return ret;
}

void rpi_connect_http_set_hostname(const char *hostname) {
    http_client_hostname = hostname;
}
