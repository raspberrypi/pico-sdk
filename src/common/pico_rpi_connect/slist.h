/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// A minimal implementation of the libcurl slist.h interface, to avoid adding
// a dependency on curl to the SDK itself. It is provided for convenience so
// the same code also works with host builds that use libcurl.

#ifndef _SLIST_H
#define _SLIST_H

// Only define our own curl_slist if we're not using real libcurl
#if PICO_ON_DEVICE

#ifdef __cplusplus
extern "C" {
#endif

// Simple linked list structure matching curl_slist
struct curl_slist {
    char *data;
    struct curl_slist *next;
};

// Core functions matching curl API exactly
struct curl_slist *curl_slist_append(struct curl_slist *list, const char *string);
void curl_slist_free_all(struct curl_slist *list);

// Additional utility function to concatenate list to string
char *curl_slist_to_string(struct curl_slist *list);

#ifdef __cplusplus
}
#endif

#else
#include <curl/curl.h>
#endif // PICO_ON_DEVICE

#endif // _SLIST_H