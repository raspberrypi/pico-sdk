/*
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
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

/*! \brief Macro used to define a bitset type
 * \ingroup pico_util
 * This macro is used to define a bitset type. It is used as follows:
 * ```
 * typedef bitset_type_t(32) my_bitset_t;
 * ```
 * will define a new bitset type called `my_bitset_t` that can hold 32 bits.
 * 
 * The type can be used as `my_bitset_t bitset;` to declare a new bitset.
 * 
 * \param N the number of bits in the bitset
 */
#define bitset_type_t(N) union { \
    generic_bitset_t bitset;    \
    struct {                    \
        uint16_t size;          \
        uint16_t word_size;     \
        uint32_t words[((N) + 31) / 32]; \
    } sized_bitset;             \
}
#define bitset_sizeof_for(N) ((((N) + 63u) / 32u) * 4u)

/*! \brief Macro used to create a bitset with all bits set to a value
 * \ingroup pico_util
 * \param type the type of the bitset
 * \param N the number of bits in the bitset
 * \param value the value to set the bits to (0 or 1)
 * \return the bitset
 */
#define bitset_with_value(type, N, value) ({ type bitset; bitset_init(&bitset, type, N, value); bitset; })

// Quick test that the bitset macros give the correct size
extern bitset_type_t(32) __not_real_bitset32;
extern bitset_type_t(33) __not_real_bitset33;
static_assert(sizeof(__not_real_bitset32) == bitset_sizeof_for(1),"");
static_assert(sizeof(__not_real_bitset33) == bitset_sizeof_for(37), "");

/*! \brief Initialize a bitset
 * \ingroup pico_util
 * \param ptr the bitset to initialize
 * \param type the type of the bitset
 * \param N the number of bits in the bitset
 * \param fill the value to fill the bitset with (0 or 1)
 */
#define bitset_init(ptr, type, N, fill) ({ \
    assert(sizeof(type) == bitset_sizeof_for(N)); \
    __unused type *type_check = ptr;       \
    __builtin_memset(ptr, (fill) ? 0xff : 0, sizeof(type));                  \
    (ptr)->bitset.size = N;                        \
    (ptr)->bitset.word_size = ((N) + 31u) / 32u;   \
})

/*! \brief Get the size of the bitset
 * \ingroup pico_util
 * \param bitset the bitset to get the size of
 * \return the size of the bitset
 */
static inline uint bitset_size(const generic_bitset_t *bitset) {
    return bitset->size;
}

/*! \brief Get the size of the bitset in words
 * \ingroup pico_util
 * \param bitset the bitset to get the size of
 * \return the size of the bitset in words
 */
static inline uint bitset_word_size(const generic_bitset_t *bitset) {
    return bitset->word_size;
}

/*! \brief Check that the bitset is valid
 * \ingroup pico_util
 * This function will assert if the bitset is not valid.
 * \param bitset the bitset to check
 */
static inline void check_bitset(const generic_bitset_t *bitset) {
    assert(bitset->word_size == (bitset->size + 31) / 32);
}

/*! \brief Write a word in the bitset
 * \ingroup pico_util
 * \param bitset the bitset to write to
 * \param word_num the word number to write to
 * \param value the value to write to the word
 * \return the bitset
 */
static inline generic_bitset_t *bitset_write_word(generic_bitset_t *bitset, uint word_num, uint32_t value) {
    check_bitset(bitset);
    if (word_num < bitset_word_size(bitset)) {
        bitset->words[word_num] = value;
    }
    return bitset;
}

/*! \brief Read a word in the bitset
 * \ingroup pico_util
 * \param bitset the bitset to read from
 * \param word_num the word number to read from
 * \return the value of the word
 */
static inline uint32_t bitset_read_word(const generic_bitset_t *bitset, uint word_num) {
    check_bitset(bitset);
    if (word_num < bitset_word_size(bitset)) {
        return bitset->words[word_num];
    }
    return 0;
}

/*! \brief Clear all bits in the bitset
 * \ingroup pico_util
 * \param bitset the bitset to clear
 * \return the bitset
 */
static inline generic_bitset_t *bitset_clear(generic_bitset_t *bitset) {
    check_bitset(bitset);
    __builtin_memset(bitset->words, 0, bitset->word_size * sizeof(uint32_t));
    return bitset;
}

/*! \brief Set all bits in the bitset
 * \ingroup pico_util
 * \param bitset the bitset to set
 * \return the bitset
 */
static inline generic_bitset_t *bitset_set_all(generic_bitset_t *bitset) {
    check_bitset(bitset);
    __builtin_memset(bitset->words, 0xff, bitset->word_size * sizeof(uint32_t));
    return bitset;
}

/*! \brief Set a single bit in the bitset
 * \ingroup pico_util
 * \param bitset the bitset to set
 * \param bit the bit to set
 * \return the bitset
 */
static inline generic_bitset_t *bitset_set_bit(generic_bitset_t *bitset, uint bit) {
    check_bitset(bitset);
    if (bit < bitset->size) {
        bitset->words[bit / 32u] |= 1u << (bit % 32u);
    }
    return bitset;
}

/*! \brief Clear a single bit in the bitset
 * \ingroup pico_util
 * \param bitset the bitset to clear
 * \param bit the bit to clear
 * \return the bitset
 */
static inline generic_bitset_t *bitset_clear_bit(generic_bitset_t *bitset, uint bit) {
    check_bitset(bitset);
    if (bit < bitset->size) {
        bitset->words[bit / 32u] &= ~(1u << (bit % 32u));
    }
    return bitset;
}

/*! \brief Get the value of a single bit in the bitset
 * \ingroup pico_util
 * \param bitset the bitset to get the value of
 * \param bit the bit to get the value of
 * \return the value of the bit
 */
static inline bool bitset_get_bit(generic_bitset_t *bitset, uint bit) {
    check_bitset(bitset);
    assert(bit < bitset->size);
//    if (bit < bitset->size) {
        return bitset->words[bit / 32u] & (1u << (bit % 32u));
//    }
    return false;
}

/*! \brief Check if two bitsets are equal
 * \ingroup pico_util
 * \param bitset1 the first bitset to check
 * \param bitset2 the second bitset to check
 * \return true if the bitsets are equal, false otherwise
 */
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
