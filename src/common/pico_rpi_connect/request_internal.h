/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _REQUEST_INTERNAL_H
#define _REQUEST_INTERNAL_H

#if PICO_ON_DEVICE

#include "connect_http.h"
// Structure to hold async request context for Pico
struct request_context {
    connect_http_request_t *http_req;
    struct altcp_tls_config *tls_config;
    char *extra_headers_buffer;
    struct curl_slist *headers;
    char *url;
    memory_struct_t *response;
    async_context_t *async_context;
    async_when_pending_worker_t worker;
};
#else

#include <stdbool.h>

// Structure to hold async request context for CURL
struct request_context {
    CURL *curl;
    CURLM *multi_handle;
    struct curl_slist *headers;
    char *url;
    long http_code;
    bool complete;
    bool streaming; // true for SSE/event-stream: return early once headers arrive
    memory_struct_t *response;
};
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct request_context request_context_t;

// Internal function to set up request context
request_context_t *rpi_connect_request_setup_internal(
    http_method_t method,
    const char *hostname,
    const char *endpoint,
    const char *token,
    struct curl_slist *additional_headers,
    const char *content_type,
    const void *data,
    size_t data_size,
    memory_struct_t *response,
    const char *serial_number);

#ifdef __cplusplus
}
#endif

#endif // _REQUEST_INTERNAL_H
