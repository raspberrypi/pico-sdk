/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "slist.h"

#include "pico/rpi_connect_util.h"

// Only compile slist functions if we're not using real libcurl
#if PICO_ON_DEVICE

struct curl_slist *curl_slist_append(struct curl_slist *list, const char *string) {
    struct curl_slist *new_item;
    struct curl_slist *last;
    size_t len;

    if (!string)
        return list;

    new_item = malloc(sizeof(struct curl_slist));
    if (!new_item)
        goto error;

    len = strlen(string);
    new_item->data = malloc(len + 1);
    if (!new_item->data)
        goto error_free_item;

    memcpy(new_item->data, string, len + 1);
    new_item->next = NULL;

    if (!list)
        return new_item;

    last = list;
    while (last->next)
        last = last->next;

    last->next = new_item;
    return list;

error_free_item:
    free(new_item);
error:
    return NULL;
}

void curl_slist_free_all(struct curl_slist *list) {
    struct curl_slist *next;
    struct curl_slist *item;

    if (!list)
        return;

    item = list;
    while (item) {
        next = item->next;
        if (item->data)
            free(item->data);
        free(item);
        item = next;
    }
}

char *curl_slist_to_string(struct curl_slist *list) {
    struct curl_slist *item;
    size_t total_len = 0;
    size_t offset = 0;
    char *result;

    if (!list)
        return NULL;

    // First pass: calculate total length needed
    item = list;
    while (item) {
        if (item->data) {
            total_len += strlen(item->data);
            total_len += 2; // Add space for CRLF (\r\n)
        }
        item = item->next;
    }

    total_len += 1; // Add space for null terminator

    // Allocate the result string
    result = malloc(total_len);
    if (!result)
        return NULL;

    // Second pass: concatenate all strings with CRLF
    item = list;
    while (item) {
        if (item->data) {
            size_t item_len = strlen(item->data);
            memcpy(result + offset, item->data, item_len);
            offset += item_len;
            result[offset++] = '\r';
            result[offset++] = '\n';
        }
        item = item->next;
    }

    result[offset] = '\0';
    return result;
}

#endif // PICO_ON_DEVICE