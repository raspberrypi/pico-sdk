/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _REQUEST_H
#define _REQUEST_H

#include <stdbool.h>
#include <stddef.h>

#include "pico/rpi_connect.h"

#ifdef __cplusplus
extern "C" {
#endif

// mbedTLS debug verbosity passed to mbedtls_debug_set_threshold(). 0 disables
// mbedTLS debug output; higher values (up to 4) are increasingly verbose.
// Requires MBEDTLS_DEBUG_C to be enabled in the mbedTLS config to have any effect.
#ifndef RPI_CONNECT_MBEDTLS_DEBUG_LEVEL
#define RPI_CONNECT_MBEDTLS_DEBUG_LEVEL 0
#endif

// Structure to hold response data
typedef struct memory_struct {
    char *memory;
    size_t size;
} memory_struct_t;

void rpi_connect_memory_struct_init(memory_struct_t *mem);
void rpi_connect_memory_struct_free(memory_struct_t *mem);
size_t rpi_connect_request_write_memory_fn(void *contents, size_t size, size_t nmemb, void *userp);

// Configure the logging level
void rpi_connect_request_set_verbose(int v);

// HTTP request method types
typedef enum {
    HTTP_GET,
    HTTP_POST,
    HTTP_DELETE
} http_method_t;

// X-Connect-Signature helper, implemented in rpi_connect.c: appends the
// signature header to a request.
void rpi_connect_add_signature(http_method_t method, const char *url, struct curl_slist *headers,
                          const void *data, size_t data_size, const char *serial_number);

// Internal state for rpi_connect_request_perform_http
typedef struct request_context request_context_t;

//
// Async context for the request API.
//
// process_data is invoked from the async worker whenever new response data is
// available; the caller drains the buffered data into its own callbacks.
//
// process_complete is invoked exactly once from the async worker after the
// HTTP request has finished (success or error), with the resulting http_code
// (or -1 on connection failure). Set both arg fields to opaque caller data.
//
// This structure is allocated and populated by the caller of rpi_connect_request_perform_http().
typedef struct request_async_context {
    request_context_t *request_context;
    void *process_data_arg;
    void (*process_data)(void *process_data_arg);
    void *process_complete_arg;
    void (*process_complete)(long http_code, void *process_complete_arg);
} request_async_context_t;

/**
 * Common function to perform HTTP requests
 *
 * @param method HTTP method (GET, POST, DELETE)
 * @param hostname The hostname to connect to
 * @param endpoint The endpoint/path to request
 * @param token Authentication token (NULL if not needed)
 * @param additional_headers Additional headers (NULL if not needed)
 * @param content_type Content type header (NULL if not needed)
 * @param data Data to send (NULL if not needed)
 * @param data_size Size of data to send (0 if not needed)
 * @param response Pointer to store response (NULL if not needed)
 * @param serial_number Device serial number for HMAC signature (NULL if not needed)
 * @param async_context Async context for non-blocking operations (required for Pico platform)
 * @param request_async_context Optional pointer to receive async request context (NULL for synchronous requests)
 * @param tls_no_verify When true the server certificate is not verified: the
 * connection is encrypted but the peer is unauthenticated. Only for requests
 * whose response integrity is guaranteed by other means, e.g. a checksum.
 * @return HTTP status code (200, 204, etc.) on success, -1 on error, or 0 for async requests
 */
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
    bool tls_no_verify);

long rpi_connect_request_http_async_poll(request_async_context_t *request_async_context);

// Content-Length advertised by the server for the response, in bytes. Valid
// once the response headers have been received; returns 0 if the server did
// not send a Content-Length (e.g. chunked transfer encoding) or the headers
// have not arrived yet.
size_t rpi_connect_request_http_content_length(request_async_context_t *request_async_context);

// Cleanup function for async HTTP requests
void rpi_connect_request_http_async_cleanup(request_async_context_t *request_async_context);

#ifdef __cplusplus
}
#endif

#endif // _REQUEST_H
