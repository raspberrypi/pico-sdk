/*
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Checks pico_printf's snprintf against the output the C standard requires, with an emphasis on
// precision, width and length edge cases. Must be built with -fno-builtin, or the compiler
// formats some of these itself.

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "pico/stdlib.h"

static int failures;

static void check(const char *expected, int ret, const char *buf, const char *desc) {
    if (strcmp(buf, expected) || ret != (int)strlen(expected)) {
        printf("FAIL %-16s got [%s] (%d), expected [%s] (%d)\n", desc, buf, ret, expected, (int)strlen(expected));
        failures++;
    }
}

#define CHECK(expected, ...) do { \
    char buf[64]; \
    memset(buf, 'X', sizeof(buf)); \
    int ret = snprintf(buf, sizeof(buf), __VA_ARGS__); \
    check(expected, ret, buf, #__VA_ARGS__); \
} while (0)

// volatile, so the arguments can't be checked at compile time
static const char *volatile null_str = NULL;
static volatile int minus_one = -1, minus_three = -3;
// no terminating NUL; only as many bytes as the precision may be read
static const char unterminated[3] = {'a', 'b', 'c'};

int main(void) {
    stdio_init_all();

    // %s with a precision
    CHECK("", "%.0s", null_str);
    CHECK("", "%.*s", 0, null_str);
    CHECK("   ", "%3.0s", null_str);
    CHECK("|  |", "|%-2.0s|", "hello");
    CHECK("ab", "%.2s", unterminated);
    CHECK("abc", "%.3s", unterminated);
    CHECK("hel", "%.3s", "hello");
    CHECK("  hel", "%5.3s", "hello");
    CHECK("hi", "%.5s", "hi");
    CHECK("", "%.s", "hello");

    // %s without a precision
    CHECK("hello", "%s", "hello");
    CHECK("   hi", "%5s", "hi");
    CHECK("hi   ", "%-5s", "hi");

    // a negative * precision is taken as if the precision were omitted
    CHECK("abc", "%.*s", minus_one, "abc");
    CHECK("  abc", "%*.*s", 5, minus_three, "abc");
    CHECK("5", "%.*d", minus_one, 5);
    CHECK("1.500000", "%.*f", minus_one, 1.5);
    // and a negative * width as the - flag
    CHECK("ab   |", "%*s|", -5, "ab");

    // integer precision
    CHECK("", "%.*d", 0, 0);
    CHECK("00005", "%.*d", 5, 5);
    CHECK("005|     005", "%.3d|%8.3d", 5, 5);
    CHECK("2", "%.*f", 0, 1.5);

    // integers
    CHECK("42|  -42|7    |", "%d|%5d|%-5d|", 42, -42, 7);
    CHECK("ff|0xff|010|0", "%x|%#x|%#o|%o", 255u, 255u, 8u, 0u);
    CHECK("+0| 5", "%+d|% d", 0, 5);
    CHECK("x|    y", "%c|%5c", 'x', 'y');
    CHECK("4000000000|-5", "%lu|%ld", 4000000000ul, -5l);

    // long long
    CHECK("-9223372036854775808", "%lld", (long long)INT64_MIN);
    CHECK("18446744073709551615", "%llu", (unsigned long long)UINT64_MAX);
    CHECK("4294967296|-4294967296", "%llu|%lld", 4294967296ull, -4294967296ll);
    CHECK("deadbeefcafe|100000000", "%llx|%llx", 0xdeadbeefcafeull, 0x100000000ull);

    // unsupported conversions print their letter, but still consume their argument
    int count = 7;
    CHECK("a|A|5", "%a|%A|%d", 1.5, 1.5, 5);
    CHECK("n|5", "%n|%d", &count, 5);
    if (count != 7) {
        printf("FAIL %%n wrote %d\n", count);
        failures++;
    }
    // long double is double on Arm; where it is wider (RISC-V) it is consumed but not printed
#if __SIZEOF_LONG_DOUBLE__ == __SIZEOF_DOUBLE__
    CHECK("1.500000|1.500000e+00|5", "%Lf|%Le|%d", 1.5L, 1.5L, 5);
#else
    CHECK("||5", "%Lf|%Le|%d", 1.5L, 1.5L, 5);
#endif
    CHECK("a|5", "%La|%d", 1.5L, 5);

    // truncation: the return value is the length that would have been written
    {
        char buf[8];
        memset(buf, 'X', sizeof(buf));
        int ret = snprintf(buf, 4, "%s", "hello");
        if (ret != 5 || strcmp(buf, "hel") || buf[4] != 'X') {
            printf("FAIL truncation: got [%.8s] (%d)\n", buf, ret);
            failures++;
        }
        ret = snprintf(NULL, 0, "%d", 12345);
        if (ret != 5) {
            printf("FAIL snprintf(NULL, 0): got %d\n", ret);
            failures++;
        }
    }

    if (failures) {
        printf("%d FAILURES\n", failures);
    } else {
        printf("PASSED\n");
    }
    return 0;
}
