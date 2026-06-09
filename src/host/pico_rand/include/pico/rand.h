/*
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_RAND_H
#define _PICO_RAND_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** \file pico/rand.h
 *  \defgroup pico_rand pico_rand
 *
 * \brief Random Number Generator API
 *
 * This module generates random numbers at runtime utilizing a number of possible entropy
 * sources and uses those sources to modify the state of a 128-bit 'Pseudo
 * Random Number Generator' implemented in software.
 *
 * The random numbers (32 to 128 bit) to be supplied are read from the PRNG which is used
 * to help provide a large number space.
 *
 * The following (multiple) sources of entropy are available (of varying quality), each enabled by a \#define:
 *
 *  - Time (\ref PICO_RAND_ENTROPY_SRC_TIME == 1): The 64-bit microsecond timer is mixed in each time.
 *
 * \note All entropy sources are hashed before application to the PRNG state machine.
 *
 * The \em first time a random number is requested, the 128-bit PRNG state
 * must be seeded. Multiple entropy sources are also available for the seeding operation:
 *
 *  - Time (\ref PICO_RAND_SEED_ENTROPY_SRC_TIME == 1): The 64-bit microsecond timer is mixed into the seed.
 *
 */

// ---------------
// ENTROPY SOURCES
// ---------------

// PICO_CONFIG: PICO_RAND_ENTROPY_SRC_TIME, Enable/disable use of hardware timestamp as an entropy source, type=bool, default=1, group=pico_rand
#ifndef PICO_RAND_ENTROPY_SRC_TIME
#define PICO_RAND_ENTROPY_SRC_TIME 1
#endif

// --------------------
// SEED ENTROPY SOURCES
// --------------------

// PICO_CONFIG: PICO_RAND_SEED_ENTROPY_SRC_TIME, Enable/disable use of hardware timestamp as an entropy source for the random seed, type=bool, default=PICO_RAND_ENTROPY_SRC_TIME, group=pico_rand
#ifndef PICO_RAND_SEED_ENTROPY_SRC_TIME
#define PICO_RAND_SEED_ENTROPY_SRC_TIME PICO_RAND_ENTROPY_SRC_TIME
#endif

// We provide a maximum of 128 bits entropy in one go
typedef struct rng_128 {
    uint64_t r[2];
} rng_128_t;

/*! \brief Get 128-bit random number
 *  \ingroup pico_rand
 *
 * \param rand128 Pointer to storage to accept a 128-bit random number
 */
void get_rand_128(rng_128_t *rand128);

/*! \brief Get 64-bit random number
 *  \ingroup pico_rand
 *
 * \return 64-bit random number
 */
uint64_t get_rand_64(void);

/*! \brief Get 32-bit random number
 *  \ingroup pico_rand
 *
 * \return 32-bit random number
 */
uint32_t get_rand_32(void);

#ifdef __cplusplus
}
#endif

#endif
