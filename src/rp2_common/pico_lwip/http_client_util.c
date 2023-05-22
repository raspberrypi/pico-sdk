/**
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include "pico/http_client_util.h"
#include "pico/async_context.h"
#include "lwip/altcp.h"
#include "lwip/altcp_tls.h"

#ifndef HTTP_INFO
#define HTTP_INFO printf
#endif

#ifndef HTTP_INFOC
#define HTTP_INFOC putchar
#endif

#ifndef HTTP_INFOC
#define HTTP_INFOC putchar
#endif

#ifndef HTTP_DEBUG
#ifdef NDEBUG
#define HTTP_DEBUG
#else
#define HTTP_DEBUG printf
#endif
#endif

#ifndef HTTP_ERROR
#define HTTP_ERROR printf
#endif

// Print headers to stdout
err_t http_client_header_print_fn(__unused httpc_state_t *connection, __unused void *arg, struct pbuf *hdr, u16_t hdr_len, __unused u32_t content_len) {
    HTTP_INFO("\nheaders %u\n", hdr_len);
    u16_t offset = 0;
    while (offset < hdr->tot_len && offset < hdr_len) {
        char c = (char)pbuf_get_at(hdr, offset++);
        HTTP_INFOC(c);
    }
    return ERR_OK;
}

// Print body to stdout
err_t http_client_receive_print_fn(__unused void *arg, __unused struct altcp_pcb *conn, struct pbuf *p, err_t err) {
    HTTP_INFO("\ncontent err %d\n", err);
    u16_t offset = 0;
    while (offset < p->tot_len) {
        char c = (char)pbuf_get_at(p, offset++);
        HTTP_INFOC(c);
    }
    return ERR_OK;
}


static err_t internal_header_fn(httpc_state_t *connection, void *arg, struct pbuf *hdr, u16_t hdr_len, u32_t content_len) {
    assert(arg);
    PICO_HTTP_REQUEST_T *req = (PICO_HTTP_REQUEST_T*)arg;
    if (req->headers_fn) {
        return req->headers_fn(connection, req->callback_arg, hdr, hdr_len, content_len);
    }
    return ERR_OK;
}

static err_t internal_recv_fn(void *arg, struct altcp_pcb *conn, struct pbuf *p, err_t err) {
    assert(arg);
    PICO_HTTP_REQUEST_T *req = (PICO_HTTP_REQUEST_T*)arg;
    if (req->recv_fn) {
        return req->recv_fn(req->callback_arg, conn, p, err);
    }
    return ERR_OK;
}

static void internal_result_fn(void *arg, httpc_result_t httpc_result, u32_t rx_content_len, u32_t srv_res, err_t err) {
    assert(arg);
    PICO_HTTP_REQUEST_T *req = (PICO_HTTP_REQUEST_T*)arg;
    HTTP_DEBUG("result %d len %u server_response %u err %d\n", httpc_result, rx_content_len, srv_res, err);
    req->complete = true;
    req->result = httpc_result;
    if (req->result_fn) {
        req->result_fn(req->callback_arg, httpc_result, rx_content_len, srv_res, err);
    }
}

// Override altcp_tls_alloc to set sni
static struct altcp_pcb *altcp_tls_alloc_sni(void *arg, u8_t ip_type) {
    assert(arg);
    PICO_HTTP_REQUEST_T *req = (PICO_HTTP_REQUEST_T*)arg;
    struct altcp_pcb *pcb = altcp_tls_alloc(req->tls_config, ip_type);
    if (!pcb) {
        HTTP_ERROR("Failed to allocate PCB\n");
        return NULL;
    }
    mbedtls_ssl_set_hostname(altcp_tls_context(pcb), req->hostname);
    return pcb;
}

// Make a http request, complete when req->complete returns true
int http_client_request_async(async_context_t *context, PICO_HTTP_REQUEST_T *req) {
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
    req->settings.headers_done_fn = req->headers_fn ? internal_header_fn : NULL;
    req->settings.result_fn = internal_result_fn;
    async_context_acquire_lock_blocking(context);
    err_t ret = httpc_get_file_dns(req->hostname, req->port ? req->port : default_port, req->url, &req->settings, internal_recv_fn, req, NULL);
    async_context_release_lock(context);
    if (ret != ERR_OK) {
        HTTP_ERROR("http request failed: %d", ret);
    }
    return ret;
}

// Make a http request and only return when it has completed. Returns true on success
int http_client_request_sync(async_context_t *context, PICO_HTTP_REQUEST_T *req) {
    assert(req);
    int ret = http_client_request_async(context, req);
    if (ret != 0) {
        return ret;
    }
    while(!req->complete) {
        async_context_poll(context);
        async_context_wait_for_work_ms(context, 1000);
    }
    return req->result;
}
