// This is a modified version of xoroshiro128++ from:
//
//   https://prng.di.unimi.it/xoroshiro128plusplus.c
//
// The code is modified to accept a pointer to a state object rather than
// having a single global state. The algorithm itself is unmodified. Original
// licence header follows:

/*  Written in 2019 by David Blackman and Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide.

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */

#include <stdint.h>

/* This is xoroshiro128++ 1.0, one of our all-purpose, rock-solid,
   small-state generators. It is extremely (sub-ns) fast and it passes all
   tests we are aware of, but its state space is large enough only for
   mild parallelism.

   For generating just floating-point numbers, xoroshiro128+ is even
   faster (but it has a very mild bias, see notes in the comments).

   The state must be seeded so that it is not everywhere zero. If you have
   a 64-bit seed, we suggest to seed a splitmix64 generator and use its
   output to fill s. */


static inline uint64_t xrand_rotl(const uint64_t x, int k) {
  return (x << k) | (x >> (64 - k));
}

typedef struct xrand_state {
  uint64_t s0;
  uint64_t s1;
} xrand_state_t;

static inline uint64_t xrand_next(xrand_state_t *s) {
  const uint64_t s0 = s->s0;
  uint64_t s1 = s->s1;
  const uint64_t result = xrand_rotl(s0 + s1, 17) + s0;

  s1 ^= s0;
  s->s0 = xrand_rotl(s0, 49) ^ s1 ^ (s1 << 21); // a, b
  s->s1 = xrand_rotl(s1, 28); // c

  return result;
}


/* This is the jump function for the generator. It is equivalent
   to 2^64 calls to next(); it can be used to generate 2^64
   non-overlapping subsequences for parallel computations. */

static inline void xrand_jump(xrand_state_t *s) {
  static const uint64_t JUMP[] = { 0x2bd7a6a6e99c2ddc, 0x0992ccaf6a6fca05 };

  uint64_t s0 = 0;
  uint64_t s1 = 0;
  for(int i = 0; i < sizeof JUMP / sizeof *JUMP; i++)
    for(int b = 0; b < 64; b++) {
      if (JUMP[i] & UINT64_C(1) << b) {
        s0 ^= s->s0;
        s1 ^= s->s1;
      }
      xrand_next(s);
    }

  s->s0 = s0;
  s->s1 = s1;
}

// Default seed for reproducible test runs: just 128 bits of /dev/urandom
#define XRAND_DEFAULT_INIT (xrand_state_t){0xfb11ab871044f128ull,  0x365396cb1df2665dull}

