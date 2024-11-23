/**
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"

// This test covers the single-precision functions in:
//
//    src/pico_float/float_hazard3_single.S
//
// It assumes the canonical generated-NaN value and NaN sign rules used by
// those functions (which are unspecified by IEEE 754). It does not cover
// libgcc/libm functions from outside of that source file.

typedef struct {
    uint32_t x;
    uint32_t y;
    uint32_t expect;
} test_t;

test_t add_directed_tests[] = {
    // 1 + 1 = 2
    {0x3f800000u, 0x3f800000u, 0x40000000u},
    // 2 + 1 = 3
    {0x40000000u, 0x3f800000u, 0x40400000u},
    // 1 + 2 = 3
    {0x3f800000u, 0x40000000u, 0x40400000u},
    // 1 + -1 = +0 (exact cancellation)
    {0x3f800000u, 0xbf800000u, 0x00000000u},
    // -1 + 1 = +0 (exact cancellation)
    {0xbf800000u, 0x3f800000u, 0x00000000u},
    // 1 + <<1 ulp = 1
    {0x3f800000u, 0x2f800000u, 0x3f800000u},
    // <<1 ulp + 1 = 1
    {0x2f800000u, 0x3f800000u, 0x3f800000u},
    // -1 + 1.25 = 0.25
    {0xbf800000u, 0x3fa00000u, 0x3e800000u},
    // max normal + 0.5 ulp = +inf
    {0x7f7fffffu, 0x73000000u, 0x7f800000u},
    // max normal + max normal = +inf
    {0x7f7fffffu, 0x7f7fffffu, 0x7f800000u},
    // min normal - 0.5 ulp = -inf
    {0xff7fffffu, 0xf3000000u, 0xff800000u},
    // min normal + min_normal = -inf
    {0xff7fffffu, 0xff7fffffu, 0xff800000u},
    // max normal + 0.499... ulp = max normal
    {0x7f7fffffu, 0x72ffffffu, 0x7f7fffffu},
    // min normal - 0.499... ulp = min normal
    {0xff7fffffu, 0xf2ffffffu, 0xff7fffffu},
    // nan + 0 = same nan
    {0xffff1234u, 0x00000000u, 0xffff1234u},
    // 0 + nan = same nan
    {0x00000000u, 0xffff1234u, 0xffff1234u},
    // nan + 1 = same nan
    {0xffff1234u, 0x3f800000u, 0xffff1234u},
    // 1 + nan = same nan
    {0x3f800000u, 0xffff1234u, 0xffff1234u},
    // nan + inf = same nan
    {0xffff1234u, 0x7f800000u, 0xffff1234u},
    // inf + nan = same nan
    {0x7f800000u, 0xffff1234u, 0xffff1234u},
    // inf + inf = inf
    {0x7f800000u, 0x7f800000u, 0x7f800000u},
    // -inf + -inf = -inf
    {0xff800000u, 0xff800000u, 0xff800000u},
    // inf + -inf = nan (all-ones is our canonical cheap nan)
    {0x7f800000u, 0xff800000u, 0xffffffffu},
    // -inf + inf = nan
    {0xff800000u, 0x7f800000u, 0xffffffffu},
    // subnormal + subnormal = exactly 0
    {0x007fffffu, 0x007fffffu, 0x00000000u},
    // -subnormal + -subnormal = exactly -0
    {0x807fffffu, 0x807fffffu, 0x80000000u},
    // Even + 0.5 ulp: round down
    {0x3f800002u, 0x33800000u, 0x3f800002u},
    // Even - 0.5 ulp: round up
    {0x3f800002u, 0xb3800000u, 0x3f800002u},
    // Odd + 0.5 ulp: round up
    {0x3f800001u, 0x33800000u, 0x3f800002u},
    // Odd - 0.5 ulp: round down
    {0x3f800001u, 0xb3800000u, 0x3f800000u},
    // All-zeroes significand - 0.5 ulp: no rounding (exact)
    {0x3f800000u, 0xb3800000u, 0x3f7fffffu},
    // Very subnormal difference of normals: flushed to zero
    {0x03800000u, 0x837fffffu, 0x00000000u},
    // Barely subnormal difference of normals: also flushed (unflushed result is 2^(emin-1))
    {0x03800000u, 0x837e0000u, 0x00000000u},
};

test_t mul_directed_tests[] = {
    // -- directed tests --
    // 1 * 1 = 1
    {0x3f800000u, 0x3f800000u, 0x3f800000u},
    // 1 * -1 = -1
    {0x3f800000u, 0xbf800000u, 0xbf800000u},
    // -1 * 1 = -1
    {0xbf800000u, 0x3f800000u, 0xbf800000u},
    // -1 * -1 = 1
    {0xbf800000u, 0xbf800000u, 0x3f800000u},
    // -0 * 0 = -0
    {0x80000000u, 0x00000000u, 0x80000000u},
    // 0 * -0 = - 0
    {0x00000000u, 0x80000000u, 0x80000000u},    
    // 1 * 2 = 2
    {0x3f800000u, 0x40000000u, 0x40000000u},
    // 2 * 1 = 2
    {0x40000000u, 0x3f800000u, 0x40000000u},
    // inf * inf = inf
    {0x7f800000u, 0x7f800000u, 0x7f800000u},
    // inf * -inf = -inf
    {0x7f800000u, 0xff800000u, 0xff800000u},
    // inf * 0 = nan
    {0x7f800000u, 0x00000000u, 0xffffffffu},
    // 0 * inf = nan
    {0x00000000u, 0x7f800000u, 0xffffffffu},
    // 1 * -inf = -inf
    {0x3f800000u, 0xff800000u, 0xff800000u},
    // -inf * 1 = -inf
    {0xff800000u, 0x3f800000u, 0xff800000u},
    // -1 * inf = -inf
    {0xbf800000u, 0x7f800000u, 0xff800000u},
    // inf * -1 = -inf
    {0x7f800000u, 0xbf800000u, 0xff800000u},
    // 1 * nonzero subnormal = exactly 0
    {0x3f800000u, 0x007fffffu, 0x00000000u},
    // nonzero subnormal * -1 = exactly -0
    {0x007fffffu, 0xbf800000u, 0x80000000u},
    // nan * 0 = same nan
    {0xffff1234u, 0x00000000u, 0xffff1234u},
    // 0 * nan = same nan
    {0x00000000u, 0xffff1234u, 0xffff1234u},
    // nan * 1 = same nan
    {0xffff1234u, 0x3f800000u, 0xffff1234u},
    // 1 * nan = same nan
    {0x3f800000u, 0xffff1234u, 0xffff1234u},
    // nan * inf = same nan
    {0xffff1234u, 0x7f800000u, 0xffff1234u},
    // inf * nan = same nan
    {0x7f800000u, 0xffff1234u, 0xffff1234u},
    // (2 - 0.5 ulp) x (2 - 0.5 ulp) = 4 - 0.5 ulp
    {0x3fffffffu, 0x3fffffffu, 0x407ffffeu},
    // (2 - 0.5 ulp) x (1 + 1 ulp) = 2 exactly
    {0xbfffffffu, 0x3f800001u, 0xc0000000u},
    // 1.666... * 1.333.. = 2.222...
    {0x3fd55555u, 0x3faaaaaau, 0x400e38e3u},
    // 1.25 x 2^-63 x 1.25 x 2^-64 = 0
    // (normal inputs with subnormal output, and we claim to be FTZ)
    {0x20200000u, 0x1fa00000u, 0x00000000u},
    // 1.333333 (rounded down) x 1.5 = 2 - 1 ulp
    {0x3faaaaaau, 0x3fc00000u, 0x3fffffffu},
    // 1.333333 (rounded down) x (1.5 + 1 ulp) = 2 exactly
    {0x3faaaaaau, 0x3fc00001u, 0x40000000u},
    // (1.333333 (rounded down) + 1 ulp) x 1.5 = 2 exactly
    {0x3faaaaabu, 0x3fc00000u, 0x40000000u},
    // (1.25 - 1 ulp) x (0.8 + 1 ulp) = 1 exactly (exponent increases after rounding)
    {0x3f9fffffu, 0x3f4cccceu, 0x3f800000u},
    // as above, but overflow on exponent increase -> +inf
    {0x3f9fffffu, 0x7f4cccceu, 0x7f800000u},
    // subtract 1 ulp from rhs -> largest normal
    {0x3f9fffffu, 0x7f4ccccdu, 0x7f7fffffu},
};

#define N_RANDOM_TESTS 1000
extern test_t add_random_tests[N_RANDOM_TESTS];
extern test_t mul_random_tests[N_RANDOM_TESTS];

uint32_t __addsf3(uint32_t x, uint32_t y);
uint32_t __mulsf3(uint32_t x, uint32_t y);

int run_tests(test_t *tests, int n_tests, const char *op_str, uint32_t (*func)(uint32_t, uint32_t)) {
    int failed = 0;
    for (int i = 0; i < n_tests; ++i) {
        uint32_t actual = func(tests[i].x, tests[i].y);
        if (tests[i].expect != actual) {
            printf("%08x %s %08x -> %08x", tests[i].x, op_str, tests[i].y, tests[i].expect);
            printf("  FAIL: got %08x\n", actual);
            ++failed;
        }
    }
    printf("Passed: %d / %d\n", n_tests - failed, n_tests);
    return failed;
}

int main() {
    stdio_init_all();
    int failed = 0;
    sleep_ms(3000);
    printf("Testing: __addsf3 (directed tests)\n");
    failed += run_tests(add_directed_tests, count_of(add_directed_tests), "+", __addsf3);
    printf("Testing: __mulsf3 (directed tests)\n");
    failed += run_tests(mul_directed_tests, count_of(mul_directed_tests), "*", __mulsf3);
    if (failed) {
        printf("Skipping random tests due to %d test failures\n", failed);
        goto done;
    }
    printf("Testing: __addsf3 (random tests)\n");
    failed += run_tests(add_random_tests, N_RANDOM_TESTS, "+", __addsf3);
    printf("Testing: __mulsf3 (random tests)\n");
    failed += run_tests(mul_random_tests, N_RANDOM_TESTS, "*", __mulsf3);

    printf("%d tests failed.\n", failed);
    if (failed == 0) {
        printf("Well done, you can relax now\n");
    }
done:
    while (true) {asm volatile ("wfi\n");} // keep USB stdout alive
    return 0;
}

// Generated using the FPU on my machine (Zen 4) plus FTZ on inputs/outputs
// See hazard3_test_gen.c
test_t add_random_tests[N_RANDOM_TESTS] = {
#include "vectors/hazard3_addsf.inc"
};

test_t mul_random_tests[N_RANDOM_TESTS] = {
#include "vectors/hazard3_mulsf.inc"
};
