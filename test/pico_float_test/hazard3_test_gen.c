/*
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <fenv.h>
#include <stdbool.h>
#include <stdint.h>

// xoroshiro256++ pseudorandom number generator.
// Adapted from: https://prng.di.unimi.it/xoshiro256plusplus.c
// Original copyright notice:

/*  Written in 2019 by David Blackman and Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide. This software is distributed without any warranty.

See <http://creativecommons.org/publicdomain/zero/1.0/>. */

/* This is xoshiro256++ 1.0, one of our all-purpose, rock-solid generators.
   It has excellent (sub-ns) speed, a state (256 bits) that is large
   enough for any parallel application, and it passes all tests we are
   aware of.

   For generating just floating-point numbers, xoshiro256+ is even faster.

   The state must be seeded so that it is not everywhere zero. If you have
   a 64-bit seed, we suggest to seed a splitmix64 generator and use its
   output to fill s. */

static inline uint64_t xr256_rotl(const uint64_t x, int k) {
	return (x << k) | (x >> (64 - k));
}

uint64_t xr256_next(uint64_t s[4]) {
	const uint64_t result = xr256_rotl(s[0] + s[3], 23) + s[0];

	const uint64_t t = s[1] << 17;

	s[2] ^= s[0];
	s[3] ^= s[1];
	s[1] ^= s[2];
	s[0] ^= s[3];

	s[2] ^= t;

	s[3] = xr256_rotl(s[3], 45);

	return result;
}
uint32_t bitcast_f2u(float x) {
	// This is UB but then so is every C program
	union {
		float f;
		uint32_t u;
	} un;
	un.f = x;
	return un.u;
}

float bitcast_u2f(uint32_t x) {
	union {
		float f;
		uint32_t u;
	} un;
	un.u = x;
	return un.f;
}

bool is_nan_u(uint32_t x) {
	return ((x >> 23) & 0xffu) == 0xffu && (x & ~(-1u << 23));
}

uint32_t flush_to_zero_u(uint32_t x) {
	if (!(x & (0xffu << 23))) {
		x &= -1u << 23;
	}
	return x;
}

uint32_t model_fadd(uint32_t x, uint32_t y) {
	x = flush_to_zero_u(x);
	y = flush_to_zero_u(y);
	// Use local hardware implementation to perform calculation
	uint32_t result = bitcast_f2u(bitcast_u2f(x) + bitcast_u2f(y));
	// Use correct canonical generated nan
	if (is_nan_u(result)) {
		result = -1u;
	}
	result = flush_to_zero_u(result);
	return result;
}

uint32_t model_fmul(uint32_t x, uint32_t y) {
	x = flush_to_zero_u(x);
	y = flush_to_zero_u(y);
	// Use local hardware implementation to perform calculation
	uint32_t result = bitcast_f2u(bitcast_u2f(x) * bitcast_u2f(y));
	// Use correct canonical generated nan
	if (is_nan_u(result)) {
		result = -1u;
	}
	result = flush_to_zero_u(result);
	return result;
}

int main() {
	// SHA-256 of a rude word
	uint64_t rand_state[4] = {
		0x5891b5b522d5df08u,
		0x6d0ff0b110fbd9d2u,
		0x1bb4fc7163af34d0u,
		0x8286a2e846f6be03u
	};
	for (int i = 0; i < 1000; ++i) {
		uint32_t x, y;
		x = xr256_next(rand_state) & 0xffffffffu;
		y = xr256_next(rand_state) & 0xffffffffu;
		// Map nan to +-inf (input nans should already be well-covered)
		if (is_nan_u(x)) {
			x &= -1u << 23;
		}
		if (is_nan_u(y)) {
			y &= -1u << 23;
		}
#if 1
		printf("{0x%08xu, 0x%08xu, 0x%08xu},\n", x, y, model_fadd(x, y));
#else
		printf("{0x%08xu, 0x%08xu, 0x%08xu},\n", x, y, model_fmul(x, y));
#endif
	}
}
