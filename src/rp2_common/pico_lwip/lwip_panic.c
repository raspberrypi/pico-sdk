/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdarg.h>
#include <stdio.h>

#include "pico.h"

// Borrowed off mbedtls
#if defined(__has_attribute)
#if __has_attribute(format)
#if defined(__MINGW32__) && __USE_MINGW_ANSI_STDIO == 1
#define PRINTF_ATTRIBUTE(string_index, first_to_check)    \
    __attribute__((__format__ (gnu_printf, string_index, first_to_check)))
#else /* defined(__MINGW32__) && __USE_MINGW_ANSI_STDIO == 1 */
#define PRINTF_ATTRIBUTE(string_index, first_to_check)    \
    __attribute__((format(printf, string_index, first_to_check)))
#endif
#else /* __has_attribute(format) */
#define PRINTF_ATTRIBUTE(string_index, first_to_check)
#endif /* __has_attribute(format) */
#else /* defined(__has_attribute) */
#define PRINTF_ATTRIBUTE(string_index, first_to_check)
#endif

// todo: panic defined in platform.h causes problems for lwip as we define LWIP_PLATFORM_ASSERT in cc.h
PRINTF_ATTRIBUTE(1, 2) void pico_lwip_panic(const char *fmt, ...) {
    va_list args;

    puts("*** LWIP PANIC ***\n");
    if (fmt) {
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
    }

    puts("\n");

    __breakpoint();
}
