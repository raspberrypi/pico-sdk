/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "request.h"

#include "pico/rpi_connect.h"

#include "request_internal.h"
#include "slist.h"

void rpi_connect_memory_struct_init(memory_struct_t *mem) {
    mem->memory = calloc(1,1);
    mem->size = 0;
}

void rpi_connect_memory_struct_free(memory_struct_t *mem) {
    if (mem->memory ) {
        free(mem->memory);
        mem->memory = NULL;
    }
    mem->size = 0;
}

// Callback function to handle response data
size_t rpi_connect_request_write_memory_fn(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    memory_struct_t *mem = (memory_struct_t *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) {
        RPI_CONNECT_ERROR("Failed to allocate memory\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

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
    const char *serial_number) {
    struct curl_slist *headers = NULL;

    // Add token header if provided
    if (token) {
        char token_header[256];
        snprintf(token_header, sizeof(token_header), "Authorization: Bearer %s", token);
        headers = curl_slist_append(headers, token_header);
    }

    // Add content type header if provided
    if (content_type) {
        char content_header[256];
        snprintf(content_header, sizeof(content_header), "Content-Type: %s", content_type);
        headers = curl_slist_append(headers, content_header);
    }

    // Add additional headers if provided
    if (additional_headers) {
        struct curl_slist *temp = additional_headers;
        while (temp) {
            headers = curl_slist_append(headers, temp->data);
            temp = temp->next;
        }
    }

    // An explicit http:// prefix in the hostname selects plain HTTP
    const char *protocol = strncmp(hostname, "http://", 7) == 0 ? "" : "https://";
    char *url = malloc(strlen(protocol) + strlen(hostname) + strlen(endpoint) + 1);
    sprintf((char *)url, "%s%s%s", protocol, hostname, endpoint);
    RPI_CONNECT_DEBUG("HTTP request Method=%s URL=%s\n", method == HTTP_GET ? "GET" : method == HTTP_POST ? "POST" : "DELETE", url);
    if (serial_number) {
        // Add signature headers
        rpi_connect_add_signature(method, url, headers, data, data_size, serial_number);
    }

    // Set up response handling
    if (response) {
        // Initialize response chunk if not already done
        if (!response->memory) {
            response->memory = malloc(1);
            response->size = 0;
        }
    }

    request_context_t *ctx = calloc(1, sizeof(struct request_context));
    ctx->headers = headers;
    ctx->response = response;
    ctx->url = url;
    return ctx;
}
