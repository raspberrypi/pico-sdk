/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "request.h"
#include "pico/rpi_connect.h"
#include "pico/rpi_connect_util.h"

#include "connect_time.h"
#include "request_internal.h"

int g_verbose = 0;
void rpi_connect_request_set_verbose(int v) {
    g_verbose = v;
}

// Learn wall-clock time from the response Date header. Only the plain
// HTTP time endpoint sends the epoch-seconds format this parses; RFC 9110
// dates from regular API responses are ignored.
static size_t header_fn(char *buffer, size_t size, size_t nitems, __unused void *userdata) {
    rpi_connect_time_observe_headers(buffer, size * nitems);
    return size * nitems;
}

long rpi_connect_request_http_async_poll(request_async_context_t *request_async_context) {
    request_context_t *request_context = request_async_context->request_context;
    long http_code = -1;

    if (!request_context || !request_context->multi_handle)
        goto fail;

    if (request_context->complete)
        http_code = request_context->http_code;

    int still_running;
    CURLMcode mc = curl_multi_perform(request_context->multi_handle, &still_running);
    if (mc != CURLM_OK)
        goto fail;

    // Check for completed transfers
    struct CURLMsg *msg;
    int msgs_left;
    while ((msg = curl_multi_info_read(request_context->multi_handle, &msgs_left))) {
        RPI_CONNECT_DEBUG("CURL message received (%p)\n", request_context);
        if (msg->msg == CURLMSG_DONE) {
            CURL *easy_handle = msg->easy_handle;
            CURLcode res = msg->data.result;

            if (res == CURLE_OK) {
                curl_easy_getinfo(easy_handle, CURLINFO_RESPONSE_CODE, &request_context->http_code);
                request_context->complete = true;
                http_code = request_context->http_code;
                goto end;
            } else {
                RPI_CONNECT_DEBUG("CURL async request failed: %s\n", curl_easy_strerror(res));
                goto fail;
            }
        }
    }

    // For streaming responses (like SSE), check if we've received headers with a success code
    // Once headers are received, consider the connection established.
    // Do NOT do this for regular downloads (streaming == false) - wait for full transfer.
    if (request_context->streaming && !request_context->complete && still_running) {
        long response_code = 0;
        curl_easy_getinfo(request_context->curl, CURLINFO_RESPONSE_CODE, &response_code);
        if (response_code > 0) {
            // We have response headers, mark as complete for streaming
            request_context->http_code = response_code;
            request_context->complete = true;
            http_code = response_code;
            RPI_CONNECT_DEBUG("Response headers received with code %ld, marking stream as connected\n", response_code);
            goto end;
        }
    }

    // Still in progress, return 0 to indicate caller should poll again
    http_code = 0;
end:
    if (request_async_context->process_data)
        request_async_context->process_data(request_async_context->process_data_arg);
    return http_code;
fail:
    RPI_CONNECT_DEBUG("HTTP code: %ld (%p)\n", http_code, request_context);
    return http_code;
}

size_t rpi_connect_request_http_content_length(request_async_context_t *request_async_context) {
    if (!request_async_context || !request_async_context->request_context)
        return 0;
    CURL *curl = request_async_context->request_context->curl;
    if (!curl)
        return 0;
    curl_off_t len = 0;
    if (curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &len) != CURLE_OK || len < 0)
        return 0;
    return (size_t)len;
}

static void request_http_cleanup(request_context_t *request_context) {
    if (!request_context)
        return;

    if (request_context->curl && request_context->multi_handle) {
        curl_multi_remove_handle(request_context->multi_handle, request_context->curl);
        curl_easy_cleanup(request_context->curl);
    }

    if (request_context->multi_handle)
        curl_multi_cleanup(request_context->multi_handle);

    if (request_context->headers)
        curl_slist_free_all(request_context->headers);

    if (request_context->url)
        free(request_context->url);

    free(request_context);
}

void rpi_connect_request_http_async_cleanup(request_async_context_t *request_async_context) {
    if (!request_async_context)
        return;

    // The caller of rpi_connect_request_perform_http is responsible for freeing the request_async_context
    // so just free the internal request_context here.
    if (request_async_context->request_context)
        request_http_cleanup(request_async_context->request_context);
    request_async_context->request_context = NULL;
}

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
    __unused async_context_t *async_context,
    request_async_context_t *request_async_context,
    bool tls_no_verify) {
    long http_code = -1;

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

    // Initialize CURL
    ctx->curl = curl_easy_init();
    if (!ctx->curl) {
        RPI_CONNECT_DEBUG("Failed to initialize CURL\n");
        free(ctx);
        goto end;
    }

    ctx->multi_handle = curl_multi_init();
    if (!ctx->multi_handle) {
        RPI_CONNECT_DEBUG("Failed to initialize CURL multi handle\n");
        curl_easy_cleanup(ctx->curl);
        free(ctx);
        goto end;
    }

    ctx->complete = false;
    ctx->http_code = -1;

    // Set up request
    curl_easy_setopt(ctx->curl, CURLOPT_URL, ctx->url);

    // Test hook: verify TLS against a local CA bundle instead of the system
    // trust store, so the test suite can run against a self-signed mock server.
    const char *ca_file = getenv("RPI_CONNECT_CA_FILE");
    if (ca_file)
        curl_easy_setopt(ctx->curl, CURLOPT_CAINFO, ca_file);

    if (tls_no_verify) {
        curl_easy_setopt(ctx->curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(ctx->curl, CURLOPT_SSL_VERIFYHOST, 0L);
    }

    // Set timeouts
    uint32_t timeout_ms = rpi_connect_http_timeout();
    curl_easy_setopt(ctx->curl, CURLOPT_CONNECTTIMEOUT_MS, timeout_ms);
    // Don't set CURLOPT_TIMEOUT for streaming responses as they stay open indefinitely

    // Set method
    if (method == HTTP_POST) {
        curl_easy_setopt(ctx->curl, CURLOPT_POST, 1L);
        if (data && data_size > 0) {
            curl_easy_setopt(ctx->curl, CURLOPT_POSTFIELDS, data);
            curl_easy_setopt(ctx->curl, CURLOPT_POSTFIELDSIZE, data_size);
            RPI_CONNECT_DEBUG("POST DATA: %s\n", (const char *)data);
        } else {
            // Empty POST body - explicitly set size to 0
            curl_easy_setopt(ctx->curl, CURLOPT_POSTFIELDSIZE, 0L);
        }
    } else if (method == HTTP_DELETE) {
        curl_easy_setopt(ctx->curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    }

    if (response) {
        curl_easy_setopt(ctx->curl, CURLOPT_WRITEFUNCTION, rpi_connect_request_write_memory_fn);
        curl_easy_setopt(ctx->curl, CURLOPT_WRITEDATA, (void *)response);
    }

    curl_easy_setopt(ctx->curl, CURLOPT_HEADERFUNCTION, header_fn);

    // Set verbose mode if requested
    if (g_verbose) {
        RPI_CONNECT_DEBUG("CURLOPT_VERBOSE\n");
        curl_easy_setopt(ctx->curl, CURLOPT_VERBOSE, 1L);
    }

    // Set headers
    if (ctx->headers) {
        curl_easy_setopt(ctx->curl, CURLOPT_HTTPHEADER, ctx->headers);
    }

    // Add the easy handle to the multi handle
    CURLMcode mc = curl_multi_add_handle(ctx->multi_handle, ctx->curl);
    if (mc != CURLM_OK) {
        RPI_CONNECT_DEBUG("Failed to add easy handle to multi handle\n");
        request_http_cleanup(ctx);
        goto end;
    }

    if (request_async_context) {
        // Async mode - return context to caller (SSE/streaming)
        ctx->streaming = true;
        request_async_context->request_context = ctx;
        http_code = 0;
    } else {
        // Sync mode - poll until completion with proper waiting
        long result;
        request_async_context_t sync_async_ctx = {.request_context = ctx};
        uint32_t timeout_ms = rpi_connect_http_timeout();

        do {
            result = rpi_connect_request_http_async_poll(&sync_async_ctx);
            // result == 0 means still in progress
            // result > 0 means completed with HTTP status code
            // result < 0 means connection error

            if (result == 0) {
                // Wait for network activity before next poll
                struct timeval tv;
                tv.tv_sec = timeout_ms / 1000;
                tv.tv_usec = (timeout_ms % 1000) * 1000;

                fd_set fdread, fdwrite, fdexcep;
                FD_ZERO(&fdread);
                FD_ZERO(&fdwrite);
                FD_ZERO(&fdexcep);

                int maxfd = -1;
                curl_multi_fdset(ctx->multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);

                if (maxfd == -1) {
                    // No file descriptors, sleep briefly
                    sleep_ms(1);
                } else {
                    select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &tv);
                }
            }
        } while (result == 0);

        http_code = result;
        if (g_verbose && response && response->memory && response->size > 0)
            RPI_CONNECT_DEBUG("< \n%.*s\n", (int)response->size, response->memory);
        request_http_cleanup(ctx);
    }

    if (http_code != 200 && http_code != 204)
        RPI_CONNECT_DEBUG("Request failed %zd\n", http_code);
end:
    return http_code;
}
