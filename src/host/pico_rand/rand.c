/*
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*  xoroshiro128ss(), rotl():

    Written in 2018 by David Blackman and Sebastiano Vigna (vigna@acm.org)

    To the extent possible under law, the author has dedicated all copyright
    and related and neighboring rights to this software to the public domain
    worldwide. This software is distributed without any warranty.

    See <http://creativecommons.org/publicdomain/zero/1.0/>

    splitmix64() implementation:

    Written in 2015 by Sebastiano Vigna (vigna@acm.org)
    To the extent possible under law, the author has dedicated all copyright
    and related and neighboring rights to this software to the public domain
    worldwide. This software is distributed without any warranty.

    See <http://creativecommons.org/publicdomain/zero/1.0/>
*/

#include "pico/rand.h"
#if PICO_RAND_ENTROPY_SRC_TIME
#include "hardware/timer.h"
#endif
#include "hardware/sync.h"

static bool rng_initialised = false;

// Note: By design, do not initialise any of the variables that hold entropy,
// they may have useful junk in them, either from power-up or a previous boot.
static rng_128_t rng_state;

/* From the original source:

   This is a fixed-increment version of Java 8's SplittableRandom generator
   See http://dx.doi.org/10.1145/2714064.2660195 and
   http://docs.oracle.com/javase/8/docs/api/java/util/SplittableRandom.html

   It is a very fast generator passing BigCrush, and it can be useful if
   for some reason you absolutely want 64 bits of state; otherwise, we
   rather suggest to use a xoroshiro128+ (for moderately parallel
   computations) or xorshift1024* (for massively parallel computations)
   generator.

   Note:  This can be called with any value (i.e. including 0)
*/
static __noinline uint64_t splitmix64(uint64_t x) {
    uint64_t z = x + 0x9E3779B97F4A7C15ull;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

/* From the original source:

   This is xoroshiro128** 1.0, one of our all-purpose, rock-solid,
   small-state generators. It is extremely (sub-ns) fast and it passes all
   tests we are aware of, but its state space is large enough only for
   mild parallelism.

   For generating just floating-point numbers, xoroshiro128+ is even
   faster (but it has a very mild bias, see notes in the comments).

   The state must be seeded so that it is not everywhere zero. If you have
   a 64-bit seed, we suggest to seed a splitmix64 generator and use its
   output to fill s.
*/
static inline uint64_t rotl(const uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}

static __noinline uint64_t xoroshiro128ss(rng_128_t *local_rng_state) {
    const uint64_t s0 = local_rng_state->r[0];
    uint64_t s1 = local_rng_state->r[1];

    // Because the state is *modified* outside of this function, there is a
    // 1 in 2^128 chance that it could be all zeroes (which is not allowed).
    while (s0 == 0 && s1 == 0) {
        s1 = time_us_64();   // should not be 0, but loop anyway
    }

    const uint64_t result = rotl(s0 * 5, 7) * 9;

    s1 ^= s0;
    local_rng_state->r[0] = rotl(s0, 24) ^ s1 ^ (s1 << 16); // a, b
    local_rng_state->r[1] = rotl(s1, 37); // c

    return result;
}

static void initialise_rand(void) {
    rng_128_t local_rng_state = local_rng_state;
    uint which = 0;

#if PICO_RAND_SEED_ENTROPY_SRC_TIME
    // Mix in hashed time.  This is [possibly] predictable boot-to-boot
    // but will vary application-to-application.
    local_rng_state.r[which] ^= splitmix64(time_us_64());
    which ^= 1;
#endif

    spin_lock_t *lock = spin_lock_instance(PICO_SPINLOCK_ID_RAND);
    uint32_t save = spin_lock_blocking(lock);
    if (!rng_initialised) {
        (void) xoroshiro128ss(&local_rng_state);
        rng_state = local_rng_state;
        rng_initialised = true;
    }
    spin_unlock(lock, save);
}

uint64_t get_rand_64(void) {
    if (!rng_initialised) {
        // Do not provide 'RNs' until the system has been initialised.  Note:
        // The first initialisation can be quite time-consuming depending on
        // the amount of RAM hashed, see RAM_HASH_START and RAM_HASH_END
        initialise_rand();
    }

    static volatile uint8_t check_byte;
    rng_128_t local_rng_state = rng_state;
    uint8_t local_check_byte = check_byte;
    // Modify PRNG state with the run-time entropy sources,
    // hashed to reduce correlation with previous modifications.
    uint which = 0;
#if PICO_RAND_ENTROPY_SRC_TIME
    local_rng_state.r[which] ^= splitmix64(time_us_64());
    which ^= 1;
#endif

    spin_lock_t *lock = spin_lock_instance(PICO_SPINLOCK_ID_RAND);
    uint32_t save = spin_lock_blocking(lock);
    if (local_check_byte != check_byte) {
        // someone got a random number in the interim, so mix it in
        local_rng_state.r[0] ^= rng_state.r[0];
        local_rng_state.r[1] ^= rng_state.r[1];
    }
    // Generate a 64-bit RN from the modified PRNG state.
    // Note: This also "churns" the 128-bit state for next time.
    uint64_t rand64 = xoroshiro128ss(&local_rng_state);
    rng_state = local_rng_state;
    check_byte++;
    spin_unlock(lock, save);

    return rand64;
}

void get_rand_128(rng_128_t *ptr128) {
    ptr128->r[0] = get_rand_64();
    ptr128->r[1] = get_rand_64();
}

uint32_t get_rand_32(void) {
    return (uint32_t) get_rand_64();
}
