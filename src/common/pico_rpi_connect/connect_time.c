/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Wall-clock time for X-Connect-Timestamp.
//
// There is no battery-backed RTC and the TLS handshake carries no time
// (TLS 1.3 removed gmt_unix_time from the server random). Connect's /up
// endpoint is reachable over plain HTTP and carries an X-Timestamp
// header with the current time in seconds since the Unix epoch; remember
// the offset between that and the boot clock. Wall time is then the boot
// clock plus the offset.

#include "connect_time.h"

#include "pico/rpi_connect.h"
#include "request.h"

// Microseconds from the boot clock to the Unix epoch. Written from network
// callbacks (possibly in IRQ context on the other core) and read from the
// application; 64-bit accesses are not atomic on RP2 so readers re-read
// until stable. 0 = not yet known.
static volatile int64_t g_epoch_offset_us;

void rpi_connect_set_time(int64_t unix_seconds) {
    if (unix_seconds <= 0) {
        return;
    }
    g_epoch_offset_us = unix_seconds * 1000000 -
        (int64_t)to_us_since_boot(get_absolute_time());
}

int64_t rpi_connect_time(void) {
    int64_t offset;
    do {
        offset = g_epoch_offset_us;
    } while (offset != g_epoch_offset_us);
    if (offset == 0) {
        return 0;
    }
    return (offset + (int64_t)to_us_since_boot(get_absolute_time())) / 1000000;
}

// Parse an X-Timestamp value: seconds since the Unix epoch as a decimal
// string. Returns -1 on anything else.
static int64_t connect_time_parse_timestamp(const char *s) {
    char *end;
    long long v = strtoll(s, &end, 10);
    if (end == s || v <= 0) {
        return -1;
    }
    while (*end == ' ') {
        end++;
    }
    if (*end != '\0') {
        return -1;
    }
    return v;
}

void rpi_connect_time_observe_headers(const char *headers, size_t len) {
    size_t i = 0;
    while (i < len) {
        // Field names are case-insensitive; HTTP/2-style proxies commonly
        // lowercase them. Match only at the start of a line.
        if (i + 12 <= len && strncasecmp(&headers[i], "x-timestamp:", 12) == 0) {
            i += 12;
            while (i < len && (headers[i] == ' ' || headers[i] == '\t')) {
                i++;
            }
            char stamp[24];
            size_t n = 0;
            while (i < len && n < sizeof(stamp) - 1 &&
                   headers[i] != '\r' && headers[i] != '\n') {
                stamp[n++] = headers[i++];
            }
            stamp[n] = '\0';
            int64_t t = connect_time_parse_timestamp(stamp);
            if (t > 0) {
                rpi_connect_set_time(t);
            }
            return;
        }
        while (i < len && headers[i] != '\n') {
            i++;
        }
        i++;
    }
}

int64_t rpi_connect_update_time(void) {
    char hostname[128];
    snprintf(hostname, sizeof(hostname), "http://%s", rpi_connect_api_host());

    memory_struct_t chunk;
    rpi_connect_memory_struct_init(&chunk);
    rpi_connect_request_perform_http(
        HTTP_GET,
        hostname,
        "/up",
        NULL,
        NULL,
        NULL,
        NULL,
        0,
        &chunk,
        NULL,
        rpi_connect_async_context(),
        NULL,
        false
    );
    rpi_connect_memory_struct_free(&chunk);

    // The X-Timestamp header was captured by the response-header callback.
    return rpi_connect_time();
}
