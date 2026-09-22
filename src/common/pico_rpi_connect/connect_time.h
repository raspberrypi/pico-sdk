/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _CONNECT_TIME_H
#define _CONNECT_TIME_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Scan a block of HTTP response headers (CRLF-separated, need not be NUL
// terminated) for an X-Timestamp header and record the server time.
void rpi_connect_time_observe_headers(const char *headers, size_t len);

#ifdef __cplusplus
}
#endif

#endif // _CONNECT_TIME_H
