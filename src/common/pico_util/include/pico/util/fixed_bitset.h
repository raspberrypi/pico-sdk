/*
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_UTIL_FIXED_BITSET_H
#define _PICO_UTIL_FIXED_BITSET_H

#include "pico.h"

/** \file fixed_bitset.h
 * \defgroup fixed_bitset fixed_bitset
 * \brief Simple fixed-size bitset implementation
 *
 * \ingroup pico_util
 */

#ifdef __cplusplus
extern "C" {
#endif

/*! \brief Base type for a fixed-size bitset
 *  \ingroup fixed_bitset
 *
 * This struct holds the metadata and storage for a fixed-size bitset. It is
 * typically used via the \ref fixed_bitset_type macro rather than directly.
 */
typedef struct {
    uint16_t size;           ///< Number of bits in the bitset
    uint16_t word_size;      ///< Number of 32-bit words used to store the bits
    uint32_t words[];        ///< Storage array for the bitset words
} fixed_bitset_t;

/*! \brief Macro used to define a fixed-size bitset of a given size
 * \ingroup fixed_bitset
 *
 * This macro is used to declare the type of a fixed-size bitset. It is used as follows:
 * ```
 * typedef fixed_bitset_type(17) my_bitset_t;
 * ```
 * will define a new bitset type called `my_bitset_t` that can hold 17 boolean values.
 *
 * The type can be used as `my_bitset_t bitset;` to declare a new bitset.
 *
 * \param N the number of boolean values in the bitset
 */
#define fixed_bitset_type(N) union { \
    fixed_bitset_t bitset;      \
    struct {                    \
        uint16_t size;          \
        uint16_t word_size;     \
        uint32_t words[((N) + 31) / 32]; \
    } sized_bitset;             \
}
#define fixed_bitset_sizeof_for(N) ((((N) + 63u) / 32u) * 4u)

/*! \brief Macro used to create a bitset with all bits set to a value
 * \ingroup fixed_bitset
 * \param type the type of the bitset
 * \param N the number of bits in the bitset
 * \param fill the value to set the bits to (0 or 1)
 * \return the bitset
 */
#define fixed_bitset_with_fill(type, N, fill) ({ type bitset; fixed_bitset_init(&bitset, type, N, fill); bitset; })

// Quick test that the bitset macros give the correct size
extern fixed_bitset_type(32) __not_real_bitset32;
extern fixed_bitset_type(33) __not_real_bitset33;
static_assert(sizeof(__not_real_bitset32) == fixed_bitset_sizeof_for(1),"");
static_assert(sizeof(__not_real_bitset33) == fixed_bitset_sizeof_for(37), "");
static_assert(sizeof(__not_real_bitset33) != fixed_bitset_sizeof_for(1), "");

/*! \brief Initialize a bitset
 * \ingroup fixed_bitset
 * \param ptr the bitset to initialize
 * \param type the type of the bitset
 * \param N the number of bits in the bitset
 * \param fill the value to fill the bitset with (0 or 1)
 */
#define fixed_bitset_init(ptr, type, N, fill) ({ \
    assert(sizeof(type) == fixed_bitset_sizeof_for(N)); \
    __unused type *type_check = ptr; \
    (ptr)->bitset.size = N; \
    (ptr)->bitset.word_size = ((N) + 31u) / 32u; \
    __builtin_memset(&(ptr)->bitset.words, (fill) ? 0xff : 0, (ptr)->bitset.word_size * sizeof(uint32_t)); \
})

/*! \brief Get the size of the bitset
 * \ingroup fixed_bitset
 * \param bitset the bitset to get the size of
 * \return the size of the bitset
 */
static inline uint fixed_bitset_size(const fixed_bitset_t *bitset) {
    return bitset->size;
}

/*! \brief Get the size of the bitset in words
 * \ingroup fixed_bitset
 * \param bitset the bitset to get the size of
 * \return the size of the bitset in words
 */
static inline uint fixed_bitset_word_size(const fixed_bitset_t *bitset) {
    return bitset->word_size;
}

/*! \brief Check that the bitset is valid
 * \ingroup fixed_bitset
 *
 * This function will assert if the bitset is not valid.
 * \param bitset the bitset to check
 */
static inline void check_fixed_bitset(__unused const fixed_bitset_t *bitset) {
    assert(bitset->word_size == (bitset->size + 31) / 32);
}

/*! \brief Write a word in the bitset
 * \ingroup fixed_bitset
 * \param bitset the bitset to write to
 * \param word_num the word number to write to
 * \param value the value to write to the word
 * \return the bitset
 */
static inline fixed_bitset_t *fixed_bitset_write_word(fixed_bitset_t *bitset, uint word_num, uint32_t value) {
    check_fixed_bitset(bitset);
    if (word_num < fixed_bitset_word_size(bitset)) {
        bitset->words[word_num] = value;
    }
    return bitset;
}

/*! \brief Read a word in the bitset
 * \ingroup fixed_bitset
 * \param bitset the bitset
 * \param word_num the word number to read from
 * \return the value of the word
 */
static inline uint32_t fixed_bitset_read_word(const fixed_bitset_t *bitset, uint word_num) {
    check_fixed_bitset(bitset);
    if (word_num < fixed_bitset_word_size(bitset)) {
        return bitset->words[word_num];
    }
    return 0;
}

/*! \brief Clear all bits in the bitset
 * \ingroup fixed_bitset
 * \param bitset the bitset
 * \return the bitset
 */
static inline fixed_bitset_t *fixed_bitset_clear_all(fixed_bitset_t *bitset) {
    check_fixed_bitset(bitset);
    __builtin_memset(bitset->words, 0, bitset->word_size * sizeof(uint32_t));
    return bitset;
}

/*! \brief Set all bits in the bitset
 * \ingroup fixed_bitset
 * \param bitset the bitset
 * \return the bitset
 */
static inline fixed_bitset_t *fixed_bitset_set_all(fixed_bitset_t *bitset) {
    check_fixed_bitset(bitset);
    __builtin_memset(bitset->words, 0xff, bitset->word_size * sizeof(uint32_t));
    return bitset;
}

/*! \brief Flip all bits in the bitset
 * \ingroup fixed_bitset
 * \param bitset the bitset
 * \return the bitset
 */
fixed_bitset_t *fixed_bitset_flip_all(fixed_bitset_t *bitset);

/*! \brief Determine if bitset is empty
 * \ingroup fixed_bitset
 * \param bitset the bitset
 * \return true if not bits are set
 */
bool fixed_bitset_is_empty(fixed_bitset_t *bitset);

/*! \brief Set a single bit in the bitset
 * \ingroup fixed_bitset
 * \param bitset the bitset
 * \param bit_index the bit to set
 * \return the bitset
 */
static inline fixed_bitset_t *fixed_bitset_set(fixed_bitset_t *bitset, uint bit_index) {
    check_fixed_bitset(bitset);
    if (bit_index < bitset->size) {
        bitset->words[bit_index / 32u] |= 1u << (bit_index % 32u);
    }
    return bitset;
}

/*! \brief Clear a single bit in the bitset
 * \ingroup fixed_bitset
 * \param bitset the bitset
 * \param bit_index the bit to clear
 * \return the bitset
 */
static inline fixed_bitset_t *fixed_bitset_clear(fixed_bitset_t *bitset, uint bit_index) {
    check_fixed_bitset(bitset);
    if (bit_index < bitset->size) {
        bitset->words[bit_index / 32u] &= ~(1u << (bit_index % 32u));
    }
    return bitset;
}

/*! \brief Flip a single bit in the bitset
 * \ingroup fixed_bitset
 * \param bitset the bitset
 * \param bit_index the bit to flip
 * \return the bitset
 */
static inline fixed_bitset_t *fixed_bitset_flip(fixed_bitset_t *bitset, uint bit_index) {
    check_fixed_bitset(bitset);
    if (bit_index < bitset->size) {
        bitset->words[bit_index / 32u] ^= 1u << (bit_index % 32u);
    }
    return bitset;
}


/*! \brief Get the value of a single bit in the bitset
 * \ingroup fixed_bitset
 * \param bitset the bitset
 * \param bit_index the bit to get the value of
 * \return the value of the bit
 */
static inline bool fixed_bitset_get(const fixed_bitset_t *bitset, uint bit_index) {
    check_fixed_bitset(bitset);
    assert(bit_index < bitset->size);
//    if (bit < bitset->size) {
        return bitset->words[bit_index / 32u] & (1u << (bit_index % 32u));
//    }
    return false;
}

/*! \brief Check if two bitsets are equal
 * \ingroup fixed_bitset
 * \param bitset1 the first bitset to check
 * \param bitset2 the second bitset to check
 * \return true if the bitsets are equal, false otherwise
 */
static inline bool fixed_bitset_equal(const fixed_bitset_t *bitset1, const fixed_bitset_t *bitset2) {
    check_fixed_bitset(bitset1);
    check_fixed_bitset(bitset2);
    assert(bitset1->size == bitset2->size);
    return __builtin_memcmp(bitset1->words, bitset2->words, bitset1->word_size * sizeof(uint32_t)) == 0;
}

#ifdef __cplusplus
}
#endif
#endif
