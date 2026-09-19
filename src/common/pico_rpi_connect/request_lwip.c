/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/rpi_connect.h"

#include "lwip/altcp.h"
#include "lwip/altcp_tls.h"
#include "mbedtls/ssl.h"
#include "mbedtls/x509_crt.h"
#ifndef NDEBUG
#include "mbedtls/debug.h"
#endif
#include "connect_time.h"
#include "request.h"
#include "request_internal.h"
#include "slist.h"

static const char *http_extra_headers_fn(void *arg) {
    return arg ? (const char *)arg : "";
}

// Learn wall-clock time from the response Date header. Only the plain
// HTTP time endpoint sends the epoch-seconds format this parses; RFC 9110
// dates from regular API responses are ignored.
static void http_client_headers_fn(__unused connect_http_request_t *req, struct pbuf *hdr, u16_t hdr_len) {
    char *buf = malloc(hdr_len);
    if (!buf)
        return;
    u16_t len = pbuf_copy_partial(hdr, buf, hdr_len, 0);
    rpi_connect_time_observe_headers(buf, len);
    free(buf);
}

static void http_client_result_fn(connect_http_request_t *req, httpc_result_t httpc_result, __unused u32_t rx_content_len, u32_t srv_res, err_t err) {
    RPI_CONNECT_DEBUG("HTTP client result: result=%d, err=%d, srv_res=%u\n", httpc_result, err, (unsigned)srv_res);

    if (httpc_result == HTTPC_RESULT_OK) {
        req->http_code = srv_res;
        if (req->ca_cert && req->tls_pcb) {
            mbedtls_ssl_context *ssl = altcp_tls_context(req->tls_pcb);
            if (ssl) {
                uint32_t flags = mbedtls_ssl_get_verify_result(ssl);
                if (flags != 0) {
                    char vrfy_buf[256];
                    mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), "  ! ", flags);
                    RPI_CONNECT_ERROR("Certificate verification failed:\n%s\n", vrfy_buf);
                    req->http_code = -1;
                }
            }
        }
    } else {
        RPI_CONNECT_ERROR("HTTP client request failed: result=%d, err=%d\n", httpc_result, err);
        req->http_code = -1;
    }

    // recv_fn_arg carries the request_context_t and is the only handle we have
    // to the async worker from this callback. Wake the worker so listen-style
    // users learn about completion without the application polling.
    request_context_t *rctx = (request_context_t *)req->recv_fn_arg;
    if (rctx && rctx->async_context)
        async_context_set_work_pending(rctx->async_context, &rctx->worker);
}

// Cap on buffered-but-unconsumed response data before the receive path
// drains it inline. Bounds memory when the consumer (e.g. an OTA flash
// write) is slower than the radio.
#ifndef REQUEST_LWIP_RECV_DRAIN_THRESHOLD
#define REQUEST_LWIP_RECV_DRAIN_THRESHOLD 4096
#endif

static size_t http_client_recv_fn(void *arg, const char *buf, size_t len) {
    request_context_t *ctx = (request_context_t *)arg;
    memory_struct_t *mem = ctx->response;

    char *ptr = realloc(mem->memory, mem->size + len + 1);
    if (!ptr) {
        RPI_CONNECT_ERROR("Failed to allocate memory\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), buf, len);
    mem->size += len;
    mem->memory[mem->size] = 0;

    // Streaming consumers run from a separate worker, which is starved while
    // the radio delivers at line rate (each segment is acked as it arrives,
    // so a fast sender never pauses). Drain oversized buffers inline: while
    // draining nothing is acked, so the TCP window throttles the sender.
    request_async_context_t *actx = (request_async_context_t *)ctx->worker.user_data;
    if (actx && actx->process_data && mem->size >= REQUEST_LWIP_RECV_DRAIN_THRESHOLD)
        actx->process_data(actx->process_data_arg);

    // Received some data so trigger the http worker again
    async_context_set_work_pending(ctx->async_context, &ctx->worker);
    return len;
}

long rpi_connect_request_http_async_poll(request_async_context_t *request_async_context) {
    connect_http_request_t *req = request_async_context->request_context->http_req;
    long http_code = 0;

    // Callable from the application as well as from the async worker: take
    // the (nestable) lock so the request state and response buffers cannot
    // change under us while lwIP runs in the background. On a polling
    // async_context this is a no-op.
    async_context_acquire_lock_blocking(req->context);

    // Always process buffered data first to avoid losing data from the final TCP segment
    if (request_async_context->process_data)
        request_async_context->process_data(request_async_context->process_data_arg);

    // Check if request is complete
    if (req->complete) {
        http_code = req->http_code;
        if (http_code <= 0)
            http_code = -1;

        RPI_CONNECT_DEBUG("Async request completed with code %ld result %d\n", http_code, req->result);

        // Fire the completion callback at most once, from the async worker.
        // This lets callback-style users (rpi_connect_*_listen) finish without
        // needing the application to poll.
        if (request_async_context->process_complete) {
            void (*cb)(long, void *) = request_async_context->process_complete;
            void *arg = request_async_context->process_complete_arg;
            request_async_context->process_complete = NULL;
            cb(http_code, arg);
        }
    }

    async_context_release_lock(req->context);
    return http_code;
}

size_t rpi_connect_request_http_content_length(request_async_context_t *request_async_context) {
    if (!request_async_context || !request_async_context->request_context)
        return 0;
    connect_http_request_t *req = request_async_context->request_context->http_req;
    if (!req)
        return 0;
    async_context_acquire_lock_blocking(req->context);
    size_t content_len = req->content_len == CONNECT_HTTP_CONTENT_LEN_UNKNOWN ?
        0 : (size_t)req->content_len;
    async_context_release_lock(req->context);
    return content_len;
}

static void request_lwip_async_poll(__unused async_context_t *request_context, async_when_pending_worker_t *worker) {
    request_async_context_t *request_async_context = (request_async_context_t *)worker->user_data;
    (void) rpi_connect_request_http_async_poll(request_async_context);
}

static void request_http_cleanup(request_context_t *request_context) {
    if (!request_context)
        return;

    if (request_context->extra_headers_buffer)
        free(request_context->extra_headers_buffer);

    if (request_context->http_req) {
        // If the request is still in progress the PCB must be aborted before
        // freeing http_req, otherwise the PCB stays stranded in lwIP's pool
        // and any later network teardown fires callbacks into freed memory.
        if (!request_context->http_req->complete && request_context->http_req->http_state)
            httpc_abort(request_context->http_req->http_state);
        free(request_context->http_req);
    }

    // Free TLS config after the PCB has been closed/aborted above.
    if (request_context->tls_config)
        altcp_tls_free_config(request_context->tls_config);

    if (request_context->url)
        free(request_context->url);

    if (request_context->headers)
        curl_slist_free_all(request_context->headers);

    free(request_context);
}

void rpi_connect_request_http_async_cleanup(request_async_context_t *request_async_context) {
    if (!request_async_context)
        return;

    // The caller of rpi_connect_request_perform_http is responsible for freeing the request_async_context
    // so just free the internal request_context here.
    if (request_async_context->request_context) {
        request_context_t *ctx = request_async_context->request_context;
        async_context_t *async_ctx = ctx->async_context;
        if (async_ctx)
            async_context_remove_when_pending_worker(async_ctx, &ctx->worker);
        // Acquire the lock so request_http_cleanup can safely call into lwIP
        // (httpc_abort → altcp_close/abort) for any in-progress connection.
        if (async_ctx)
            async_context_acquire_lock_blocking(async_ctx);
        request_http_cleanup(ctx);
        if (async_ctx)
            async_context_release_lock(async_ctx);
    }
    request_async_context->request_context = NULL;
}

// TLS session tickets are bound to the issuing server, so sessions are
// cached per hostname: a single shared slot would let the OTA download
// host clobber the Connect host's ticket between requests.
#ifndef RPI_CONNECT_TLS_SESSION_HOSTS
#define RPI_CONNECT_TLS_SESSION_HOSTS 4
#endif

typedef struct {
    char *host;
    uint32_t last_used;
    struct altcp_tls_session session;
} tls_session_slot_t;

// Call with the request's async_context lock held: eviction frees session
// state that in-flight requests on the same context save/restore from lwIP
// callbacks. Returns NULL (table churn with OOM) to run without resumption.
static struct altcp_tls_session *tls_session_for_host(const char *host) {
    static tls_session_slot_t slots[RPI_CONNECT_TLS_SESSION_HOSTS];
    static uint32_t stamp;
    tls_session_slot_t *evict = &slots[0];

    for (unsigned i = 0; i < RPI_CONNECT_TLS_SESSION_HOSTS; i++) {
        tls_session_slot_t *s = &slots[i];
        if (!s->host) {
            if (evict->host)
                evict = s;
        } else if (strcmp(s->host, host) == 0) {
            s->last_used = ++stamp;
            return &s->session;
        } else if (evict->host && s->last_used < evict->last_used) {
            evict = s;
        }
    }

    char *copy = strdup(host);
    if (!copy)
        return NULL;
    if (evict->host) {
        free(evict->host);
        altcp_tls_free_session(&evict->session);
    }
    altcp_tls_init_session(&evict->session);
    evict->host = copy;
    evict->last_used = ++stamp;
    return &evict->session;
}

#if RPI_CONNECT_DEBUG_ENABLE
static void dump_binary_data(const void *data, uint32_t len) {
    const uint8_t *bptr = data;
    unsigned int i = 0;
    char ascii[17] = {0};

    for (i = 0; i < len;) {
        if ((i & 0x0f) == 0) {
            RPI_CONNECT_DEBUG(" %s\n", ascii);
            memset(ascii, 0, sizeof(ascii));
        } else if ((i & 0x07) == 0) {
            RPI_CONNECT_DEBUG(" ");
        }
        RPI_CONNECT_DEBUG("%02x ", bptr[i]);
        ascii[i % 16] = (bptr[i] >= 32 && bptr[i] <= 126) ? (char)bptr[i] : '.';
        i++;
    }
    RPI_CONNECT_DEBUG(" %s\n", ascii);
}
#endif

long rpi_connect_request_perform_http(
    http_method_t method,
    const char *hostname,
    const char *endpoint,
    const char *token,
    struct curl_slist *additional_headers,
    const char *content_type,
    const void *data,
    size_t data_size,
    memory_struct_t *response,
    const char *serial_number,
    async_context_t *async_context,
    request_async_context_t *request_async_context,
    bool tls_no_verify) {
    long http_code = -1;
    // An explicit http:// prefix selects plain HTTP on port 80 (used for
    // the Connect time endpoint); everything else is TLS on 443.
    bool plain_http = strncmp(hostname, "http://", 7) == 0;
    request_context_t *ctx = rpi_connect_request_setup_internal(
        method,
        hostname,
        endpoint,
        token,
        additional_headers,
        content_type,
        data,
        data_size,
        response,
        serial_number);

    if (!ctx) {
        RPI_CONNECT_DEBUG("Failed to set up request\n");
        return -1;
    }
    ctx->async_context = async_context;

    ctx->http_req = calloc(1, sizeof(connect_http_request_t));
    if (!ctx->http_req) {
        RPI_CONNECT_DEBUG("Failed to allocate HTTP request\n");
        goto end;
    }

    // Set up HTTP request structure
    ctx->http_req->settings.use_proxy = 0;
    ctx->http_req->result_fn = http_client_result_fn;
    ctx->http_req->result_fn_arg = NULL;
    ctx->http_req->recv_fn = http_client_recv_fn;
    ctx->http_req->recv_fn_arg = ctx;

    if (ctx->headers) {
        ctx->http_req->extra_headers_fn = http_extra_headers_fn;
        ctx->extra_headers_buffer = curl_slist_to_string(ctx->headers);
        ctx->http_req->extra_headers_arg = ctx->extra_headers_buffer;
        RPI_CONNECT_DEBUG("HEADERS: \r\n%s\r\n", ctx->extra_headers_buffer);
    }

    // Snapshot the CA for this request: a concurrent rpi_connect_set_ca_cert
    // (e.g. around an OTA download) must not change what an in-flight request
    // verifies against.
    const char *ca_cert = NULL;
    if (plain_http) {
        hostname += 7;
    } else {
        if (!tls_no_verify)
            ca_cert = rpi_connect_ca_cert();
        if (ca_cert)
            ctx->tls_config = altcp_tls_create_config_client((const u8_t *)ca_cert, strlen(ca_cert) + 1);
        else
            ctx->tls_config = altcp_tls_create_config_client(NULL, 0);
    }

#if defined(MBEDTLS_SSL_PROTO_TLS1_3) && defined(MBEDTLS_SSL_SESSION_TICKETS)
    // Tell mbedtls to actually parse TLS 1.3 NewSessionTicket records instead
    // of silently dropping them. The mbedtls_ssl_config is the first field of
    // struct altcp_tls_config in altcp_tls_mbedtls.c.
    if (ctx->tls_config) {
        mbedtls_ssl_conf_tls13_enable_signal_new_session_tickets(
            (mbedtls_ssl_config *)ctx->tls_config,
            MBEDTLS_SSL_TLS1_3_SIGNAL_NEW_SESSION_TICKETS_ENABLED);
    }
#endif

#if defined(MBEDTLS_DEBUG_C)
    mbedtls_debug_set_threshold(RPI_CONNECT_MBEDTLS_DEBUG_LEVEL);
#endif
    ctx->http_req->tls_config = ctx->tls_config;
    ctx->http_req->ca_cert = ca_cert;
    ctx->http_req->headers_fn = http_client_headers_fn;

    const char *post_data = NULL;
    int post_data_len = 0;
    if (method == HTTP_POST && content_type) {
        post_data = (const char *)data;
        // Trust the caller-supplied size: msgpack / octet-stream bodies
        // routinely contain 0x00 bytes that strlen() would truncate at.
        post_data_len = (int)data_size;
        RPI_CONNECT_DEBUG("POST DATA: %d bytes\n", post_data_len);
#if RPI_CONNECT_DEBUG_ENABLE
        dump_binary_data(post_data, post_data_len);
#endif
    }

    // Starting the request calls into lwIP; hold the lock so this is safe
    // when lwIP runs from a background async_context (no-op when polling).
    async_context_acquire_lock_blocking(ctx->async_context);
    // On the first request to a host the slot's session is empty, so
    // altcp_tls_set_session() is a no-op; the completion callback then saves
    // the negotiated session here for the next request to resume.
    if (!plain_http)
        ctx->http_req->tls_session = tls_session_for_host(hostname);
    rpi_connect_http_set_hostname(hostname);
    int rc = rpi_connect_http_request_async(ctx->async_context, ctx->http_req, endpoint, post_data_len, post_data);
    async_context_release_lock(ctx->async_context);

    if (rc != 0) {
        RPI_CONNECT_DEBUG("Async request failed to start: %d\n", rc);
        async_context_acquire_lock_blocking(ctx->async_context);
        request_http_cleanup(ctx);
        async_context_release_lock(ctx->async_context);
        goto end;
    }

    ctx->worker.do_work = request_lwip_async_poll;
    if (request_async_context) {
        // Async mode - return context to caller
         ctx->worker.user_data = request_async_context;
         request_async_context->request_context = ctx;
         async_context_add_when_pending_worker(ctx->async_context, &ctx->worker);
        return 0;
    } else {
        // Sync mode - create an async context internally and wait here until complete
        request_async_context_t sync_async_ctx = {.request_context = ctx};
        ctx->worker.user_data = &sync_async_ctx;
        async_context_t *async_ctx = ctx->async_context;
        async_context_add_when_pending_worker(async_ctx, &ctx->worker);
        // Works with every async_context type: on a polling context the work
        // happens inside async_context_poll(); on threadsafe_background /
        // FreeRTOS contexts poll() is a no-op and the work runs in the
        // background, so the completion flag is checked under the context
        // lock (which orders it against the lwIP callbacks' writes, possibly
        // on the other core) and we sleep between checks instead of spinning.
        while (true) {
            async_context_poll(async_ctx);
            async_context_acquire_lock_blocking(async_ctx);
            // complete == 0 means still in progress
            // complete > 0 means completed with HTTP status code (which could be an error)
            // complete < 0 connection errror
            int complete = ctx->http_req->complete;
            async_context_release_lock(async_ctx);
            if (complete)
                break;
            async_context_wait_for_work_until(async_ctx, make_timeout_time_ms(10));
        }

        async_context_remove_when_pending_worker(async_ctx, &ctx->worker);

        // The connection may still be tearing down in the background: read
        // the result and free the request under the lock.
        async_context_acquire_lock_blocking(async_ctx);
        http_code = ctx->http_req->http_code;

        // Clean up context since caller doesn't want it
        request_http_cleanup(ctx);
        async_context_release_lock(async_ctx);
    }

    if (http_code != 200 && http_code != 204)
        RPI_CONNECT_DEBUG("Request failed %d\n", (int)http_code);
end:
    return http_code;
}
