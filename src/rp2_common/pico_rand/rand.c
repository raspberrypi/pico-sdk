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
#include "pico/unique_id.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"
#include "hardware/structs/rosc.h"
#include "hardware/sync.h"

// Hashing the selected area of RAM is done the first time
// a random number is requested. If there is any chance the first RN will be
// requested from interrupt context only a small amount of RAM should be hashed
// hence the 1k default here which will take ~100us on RP2040 at 125Mhz

// PICO_CONFIG: PICO_RAND_DISABLE_ROSC_CHECKS, Disable assertions that the ROSC is running when generating random numbers which will drastically reduce randomness, type=bool, default=0, group=pico_rand

// Note that by default we hash the last 1K of SRAM which usually has the core0 stack, so is not only as good as any place to hash on power
// up, it is quite unpredictable in contexts on warm resets.

// PICO_CONFIG: PICO_RAND_RAM_HASH_END, end of address in RAM (non-inclusive) to hash during pico_rand initialization, default=SRAM_END, group=pico_rand
#ifndef PICO_RAND_RAM_HASH_END
#define PICO_RAND_RAM_HASH_END     SRAM_END
#endif
// PICO_CONFIG: PICO_RAND_RAM_HASH_START, start of address in RAM (inclusive) to hash during pico_rand initialization, default=PICO_RAND_RAM_HASH_END-1024, group=pico_rand
#ifndef PICO_RAND_RAM_HASH_START
#define PICO_RAND_RAM_HASH_START   (PICO_RAND_RAM_HASH_END - 1024u)
#endif

// PICO_CONFIG: PICO_RAND_MIN_ROSC_SAMPLE_TIME_US, Define a default minimum time between sampling the ROSC random bit, min=5, max=20, group=pico_rand
#ifndef PICO_RAND_MIN_ROSC_SAMPLE_TIME_US
// (Arbitrary / tested) minimum time between sampling the ROSC random bit
#define PICO_RAND_MIN_ROSC_SAMPLE_TIME_US 10u
#endif

static_assert(PICO_UNIQUE_BOARD_ID_SIZE_BYTES == sizeof(uint64_t),
              "Code below requires that 'board_id' is 64-bits in size");

// Note! The safety of the length assumption here is protected by a 'static_assert' above
union unique_id_u {
    pico_unique_board_id_t board_id_native;
    uint64_t board_id_u64;
};

static bool rng_initialised = false;

// Note: By design, do not initialise any of the variables that hold entropy,
// they may have useful junk in them, either from power-up or a previous boot.
static uint64_t __uninitialized_ram(ram_hash);
static uint64_t __uninitialized_ram(rosc_samples);
static rng_128_t __uninitialized_ram(rand_storage);

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

static __noinline uint64_t xoroshiro128ss(void) {
    const uint64_t s0 = rand_storage.r[0];
    uint64_t s1 = rand_storage.r[1];

    // Because the state is *modified* outside of this function, there is a
    // 1 in 2^128 chance that it could be all zeroes (which is not allowed).
    while (s0 == 0 && s1 == 0) {
        s1 = time_us_64();   // should not be 0, but loop anyway
    }

    const uint64_t result = rotl(s0 * 5, 7) * 9;

    s1 ^= s0;
    rand_storage.r[0] = rotl(s0, 24) ^ s1 ^ (s1 << 16); // a, b
    rand_storage.r[1] = rotl(s1, 37); // c

    return result;
}

// Note: This can take many tens of milliseconds to execute
static uint64_t sdbm_hash64_sram(uint64_t hash) {
    // save some time by hashing a word at a time
    for (uint i = (PICO_RAND_RAM_HASH_START + 3) & ~3; i < PICO_RAND_RAM_HASH_END; i+=4) {
        uint32_t c = *(uint32_t *) i;
        hash = (uint64_t) c + (hash << 6) + (hash << 16) - hash;
    }
    return hash;
}

// check that the ROSC is running but that the processors are NOT running from it
static inline void check_rosc_asserts(void) {
#if !PICO_RAND_DISABLE_ROSC_CHECKS
    hard_assert(rosc_hw->status & ROSC_STATUS_ENABLED_BITS);
    hard_assert((clocks_hw->clk[clk_sys].ctrl & CLOCKS_CLK_SYS_CTRL_AUXSRC_BITS) !=
                (CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_ROSC_CLKSRC << CLOCKS_CLK_SYS_CTRL_AUXSRC_LSB));
#endif
}

static void update_rosc_samples_under_lock(void) {
    static uint32_t previous_sample_time_us = 0;

    check_rosc_asserts();

    // Ensure that the ROSC random bit is not sampled too quickly,
    // ROSC may be ticking only a few times a microsecond.
    // Note: In general (i.e. sporadic) use, very often there will be no delay here.
    while ((time_us_32() - previous_sample_time_us) < PICO_RAND_MIN_ROSC_SAMPLE_TIME_US) {
        tight_loop_contents();
    }
    previous_sample_time_us = time_us_32();

    rosc_samples <<= 1;
    rosc_samples |= rosc_hw->randombit & 1u;
}

static void initialise_rand_under_lock(void) {
    union unique_id_u unique_id;

    ram_hash = sdbm_hash64_sram(ram_hash);
    rand_storage.r[0] ^= splitmix64(ram_hash);

    // Note! The safety of the length assumption here is protected by a 'static_assert' above
    pico_get_unique_board_id(&unique_id.board_id_native);
    rand_storage.r[0] ^= splitmix64(unique_id.board_id_u64);

    uint ref_khz = clock_get_hz(clk_ref) / 100;
    for (int i = 0; i < 5; i++) {
        // Apply hash of the rosc frequency, limited but still 'extra' entropy
        uint measurement = frequency_count_raw(CLOCKS_FC0_SRC_VALUE_ROSC_CLKSRC, ref_khz);
        rand_storage.r[0] ^= splitmix64(measurement);
        (void) xoroshiro128ss();  //churn to mix seed sources
    }

    // Completely fill local ROSC sample array with sample bits
    for (uint i = 0; i < (8 * sizeof(rosc_samples)); i++) {
        update_rosc_samples_under_lock();
    }
    // Apply hashed ROSC samples value
    rand_storage.r[1] ^= splitmix64(rosc_samples);

    // Mix in hashed time.  This is [possibly] predictable boot-to-boot
    // but will vary application-to-application.
    rand_storage.r[1] ^= splitmix64(time_us_64());

    (void) xoroshiro128ss();
}

uint64_t get_rand_64(void) {
    uint64_t rand64;

    spin_lock_t *lock = spin_lock_instance(PICO_SPINLOCK_ID_RAND);
    uint32_t save = spin_lock_blocking(lock);

    if (!rng_initialised) {
        // Do not provide 'RNs' until the system has been initialised.  Note:
        // The first initialisation can be quite time-consuming depending on
        // the amount of RAM hashed, see RAM_HASH_START and RAM_HASH_END
        initialise_rand_under_lock();
        rng_initialised = true;
    }

    update_rosc_samples_under_lock();

    // Modify PRNG state with the two run-time entropy sources,
    // hashed to reduce correlation with previous modifications.
    rand_storage.r[0] ^= splitmix64(rosc_samples);
    rand_storage.r[1] ^= splitmix64(time_us_64());

    // Generate a 64-bit RN from the modified PRNG state.
    // Note: This also "churns" the 128-bit state for next time.
    rand64 = xoroshiro128ss();

    spin_unlock(lock, save);

    return rand64;
}

void get_rand_128(rng_128_t *ptr128) {
    if (ptr128 != NULL) {
        ptr128->r[0] = get_rand_64();
        ptr128->r[1] = get_rand_64();
    }
}

uint32_t get_rand_32(void) {
    return (uint32_t) get_rand_64();
}
