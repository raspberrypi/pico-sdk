/*
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_RAND_H
#define _PICO_RAND_H

#include "pico.h"

#ifdef __cplusplus
extern "C" {
#endif

/** \file pico/rand.h
 *  \defgroup pico_rand pico_rand
 *
 * Random Number Generator API
 *
 * This module generates random numbers at runtime via a couple of entropy
 * sources and uses those sources to modify the state of a 128-bit 'Pseudo
 * Random Number Generator' implemented in software.
 *
 * The random numbers to be supplied are read from the PRNG which is used
 * to help provide a large number space.
 *
 * The main source of entropy at run time is a provided by ROSC 'random bit'
 * samples but system time is also used to add a little extra entropy.
 *
 * All entropy sources are hashed before application to the PRNG state machine.
 *
 * Note: The *first* time a random number is requested, the 128-bit PRNG state
 * must be seeded.  The seed generation takes approximately 1 millisecond with
 * subsequent random numbers taking between 10 and 20 microseconds to generate.
 */

// We provide a maximum of 128 bits entropy in one go
typedef struct rng_128 {
    uint64_t r[2];
} rng_128_t;

/*! \brief Get 128-bit random number
 *  \ingroup pico_rand
 *
 * \param rand128  Pointer to storage to accept a 128-bit random number
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
