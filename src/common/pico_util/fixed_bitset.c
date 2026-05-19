/*
* Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "include/pico/util/fixed_bitset.h"

fixed_bitset_t *fixed_bitset_flip_all(fixed_bitset_t *bitset) {
    check_fixed_bitset(bitset);
    for (uint i=0;i<bitset->word_size;i++) {
        bitset->words[i] = ~bitset->words[i];
    }
    return bitset;
}

bool fixed_bitset_is_empty(fixed_bitset_t *bitset) {
    check_fixed_bitset(bitset);
    int i=0;
    for (i=0;i<bitset->word_size-1;i++) {
        if (bitset->words[i]) return false;
    }
    // we don't guarantee that bits above the size aren't set, so mask them off
    return !(bitset->words[i] << (32u -(bitset->size & 31u)));
}

