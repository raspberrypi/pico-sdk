/*
* Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/util/fixed_bitset.h"

#define CHECK(b, message, ...) ({ if (!(b)) { printf("FAILED: " message "\n", ##__VA_ARGS__); exit(1); } })

typedef fixed_bitset_type(4) bitset4_t;
typedef fixed_bitset_type(47) bitset47_t;
typedef fixed_bitset_type(77) bitset77_t;

int main() {
    stdio_init_all();

#define FIXED_BITSET_TESTS(name, type, n)\
    type name;\
    fixed_bitset_init(&name, type, n, 0);\
    for (int i=0;i<n;i++) {\
        CHECK(!fixed_bitset_get(&name.bitset, i), "Bit %d should be clear", i);\
    }\
    \
    fixed_bitset_init(&name, type, n, 1);\
    for (int i=0;i<n;i++) {\
        CHECK(fixed_bitset_get(&name.bitset, i), "Bit %d should be set", i);\
    }\
    \
    for (int i=n-1;i>=0;i--) {\
        CHECK(fixed_bitset_get(&name.bitset, i), "Bit %d should be set", i);\
        fixed_bitset_clear(&name.bitset, i);\
        CHECK(!fixed_bitset_get(&name.bitset, i), "Bit %d should be clear", i);\
        CHECK(!i == fixed_bitset_is_empty(&name.bitset), "Bitset should be empty once last bit is cleared");\
    }\
    \
    fixed_bitset_init(&name, type, n, 1);\
    for (int i=0;i<n;i++) {\
        CHECK(fixed_bitset_get(&name.bitset, i), "Bit %d should be set", i);\
    }\
    for (int i=n-1;i>=0;i--) {\
        CHECK(fixed_bitset_get(&name.bitset, i), "Bit %d should be set", i);\
        fixed_bitset_clear(&name.bitset, i);\
        CHECK(!fixed_bitset_get(&name.bitset, i), "Bit %d should be clear", i);\
        CHECK(!i == fixed_bitset_is_empty(&name.bitset), "Bitset should be empty once last bit is cleared");\
    }\

    FIXED_BITSET_TESTS(b4, bitset4_t, 4);
    FIXED_BITSET_TESTS(b47, bitset47_t, 47);
    FIXED_BITSET_TESTS(b77, bitset77_t, 77);

    puts("PASSED");
}