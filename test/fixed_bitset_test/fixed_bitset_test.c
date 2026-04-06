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

typedef fixed_bitset_type(47) bitset47_t;

int main() {
    stdio_init_all();

    bitset47_t b47;
    fixed_bitset_init(&b47, bitset47_t, 47, 0);
    for (int i=0;i<47;i++) {
        CHECK(!fixed_bitset_get(&b47.bitset, i), "Bit %d should be clear", i);
    }

    fixed_bitset_init(&b47, bitset47_t, 47, 1);
    for (int i=0;i<47;i++) {
        CHECK(fixed_bitset_get(&b47.bitset, i), "Bit %d should be set", i);
    }
    for (int i=46;i>=0;i--) {
        CHECK(fixed_bitset_get(&b47.bitset, i), "Bit %d should be set", i);
        fixed_bitset_clear(&b47.bitset, i);
        CHECK(!fixed_bitset_get(&b47.bitset, i), "Bit %d should be clear", i);
        CHECK(!i == fixed_bitset_is_empty(&b47.bitset), "Bitset should be empty once last bit is cleared");
    }

    puts("PASSED");
}