/*
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Checks that PICO_UBSAN_NULL_CHECKS catches a null pointer read.

#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"

static volatile uint32_t value = 0x12345678;

// volatile, so the compiler cannot see that this is null and fold the access
// away or treat the path as unreachable.
static uint32_t *volatile ptr;

int main(void) {
    stdio_init_all();
    printf("pico_ubsan_null_test\n");

    // A non-null load through the same code path must not trip the check.
    ptr = (uint32_t *)&value;
    uint32_t ok = *ptr;
    printf("non-null load returned 0x%08x\n", (unsigned)ok);

    ptr = NULL;
    printf("loading through a null pointer -- expecting a ubsan panic now\n");
    stdio_flush();
    uint32_t bad = *ptr;

#if PICO_UBSAN_RECOVER
    // The recoverable handler reports and returns, so getting here is expected.
    printf("load returned 0x%08x and execution continued, as expected with"
           " PICO_UBSAN_RECOVER\n", (unsigned)bad);
#else
    printf("FAILED: the null load was not detected (returned 0x%08x)\n", (unsigned)bad);
#endif
    while (true) {
        tight_loop_contents();
    }
}
