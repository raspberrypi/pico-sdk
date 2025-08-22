/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_UTIL_BITSET_H
#define _PICO_UTIL_BITSET_H

#include "pico.h"

/** \file bitset.h
 * \defgroup bitset bitset
 * \brief Simple bitset implementation
 *
 * \ingroup pico_util
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t size;           \
    uint16_t word_size;     \
    uint32_t words[];
} generic_bitset_t;

#define bitset_type_t(N) union { \
    generic_bitset_t bitset;    \
    struct {                    \
        uint16_t size;          \
        uint16_t word_size;     \
        uint32_t words[((N) + 31) / 32]; \
    } sized_bitset;             \
}
#define bitset_sizeof_for(N) ((((N) + 63u) / 32u) * 4u)

extern bitset_type_t(32) __not_real_bitset32;
extern bitset_type_t(33) __not_real_bitset33;
static_assert(sizeof(__not_real_bitset32) == bitset_sizeof_for(1),"");
static_assert(sizeof(__not_real_bitset33) == bitset_sizeof_for(37), "");

#define bitset_init(ptr, type, N, fill) ({ \
    assert(sizeof(type) == bitset_sizeof_for(N)); \
    __unused type *type_check = ptr;       \
    __builtin_memset(ptr, (fill) ? 0xff : 0, sizeof(type));                  \
    (ptr)->bitset.size = N;                        \
    (ptr)->bitset.word_size = ((N) + 31u) / 32u;   \
})

static inline uint bitset_size(const generic_bitset_t *bitset) {
    return bitset->size;
}

static inline uint bitset_word_size(const generic_bitset_t *bitset) {
    return bitset->word_size;
}

static inline void check_bitset(const generic_bitset_t *bitset) {
    assert(bitset->word_size == (bitset->size + 31) / 32);
}

static inline generic_bitset_t *bitset_write_word(generic_bitset_t *bitset, uint word_num, uint32_t value) {
    check_bitset(bitset);
    if (word_num < bitset_word_size(bitset)) {
        bitset->words[word_num] = value;
    }
}

static inline uint32_t bitset_read_word(const generic_bitset_t *bitset, uint word_num) {
    check_bitset(bitset);
    if (word_num < bitset_word_size(bitset)) {
        return bitset->words[word_num];
    }
    return 0;
}

static inline generic_bitset_t *bitset_clear(generic_bitset_t *bitset) {
    check_bitset(bitset);
    __builtin_memset(bitset->words, 0, bitset->word_size * sizeof(uint32_t));
    return bitset;
}

static inline generic_bitset_t *bitset_set_all(generic_bitset_t *bitset) {
    check_bitset(bitset);
    __builtin_memset(bitset->words, 0xff, bitset->word_size * sizeof(uint32_t));
    return bitset;
}

static inline generic_bitset_t *bitset_set_bit(generic_bitset_t *bitset, uint bit) {
    check_bitset(bitset);
    if (bit < bitset->size) {
        bitset->words[bit / 32u] |= 1u << (bit % 32u);
    }
    return bitset;
}

static inline generic_bitset_t *bitset_clear_bit(generic_bitset_t *bitset, uint bit) {
    check_bitset(bitset);
    if (bit < bitset->size) {
        bitset->words[bit / 32u] &= ~(1u << (bit % 32u));
    }
    return bitset;
}

static inline bool bitset_get_bit(generic_bitset_t *bitset, uint bit) {
    check_bitset(bitset);
    assert(bit < bitset->size);
//    if (bit < bitset->size) {
        return bitset->words[bit / 32u] & (1u << (bit % 32u));
//    }
    return false;
}

static inline bool bitset_equal(const generic_bitset_t *bitset1, const generic_bitset_t *bitset2) {
    check_bitset(bitset1);
    check_bitset(bitset2);
    assert(bitset1->size == bitset2->size);
    return __builtin_memcmp(bitset1->words, bitset2->words, bitset1->word_size * sizeof(uint32_t)) == 0;
}

typedef uint32_t tiny_encoded_bitset_t;
typedef uint64_t encoded_bitset_t;

#define encoded_bitset_empty() 0
#define encoded_bitset_of1(v) (1u | ((v) << 8))
#define encoded_bitset_of2(v1, v2) (2u | ((v1) << 8) | ((v2) << 16))
#define encoded_bitset_of3(v1, v2, v3) (3u | ((v1) << 8) | ((v2) << 16) | (((v3) << 24)))
#define encoded_bitset_of4(v1, v2, v3, v4) (4u | ((v1) << 8) | ((v2) << 16) | (((v3) << 24)) | (((uint64_t)(v4)) << 32))
#define encoded_bitset_of5(v1, v2, v3, v4, v5) (5u | ((v1) << 8) | ((v2) << 16) | (((v3) << 24)) | (((uint64_t)((v4) | ((v5)<<8u))) << 32))

#define encoded_bitset_foreach(bitset, x) ({ \
    for(uint _i=0;_i<((bitset)&0xffu);_i++) {  \
        uint bit = (uint8_t)((bitset) >> (8 * _i)); \
        x;                                   \
    }                                        \
})
#ifdef __cplusplus
}
#endif
#endif
