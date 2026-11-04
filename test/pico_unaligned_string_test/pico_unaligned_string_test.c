/*
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Calls every libc string/memory function that newlib has an alignment-sensitive
// implementation of, with every combination of source and destination alignment,
// and checks the results against byte-wise reference implementations.
//
// This exists because we shipped a toolchain whose newlib omitted the alignment
// checks in these functions. RP2350's RISC-V cores fault on a misaligned access,
// so calling strncmp() on an odd address hung the chip -- see
// raspberrypi/pico-sdk#3118.
//
// newlib decides whether the hardware can do misaligned access in two separate
// places, and a build can get one right and the other wrong:
//
//   * the generic C routines test _HAVE_HW_MISALIGNED_ACCESS, set by newlib's
//     configure;
//   * the RISC-V specific routines in newlib/libc/machine/riscv test
//     __riscv_misaligned_slow / _fast directly, and ignore configure entirely.
//
// So the list below deliberately spans both. If a future toolchain regresses,
// this test faults on the offending call
//
// A bad libc does not report anything, it simply stops: the core takes an
// unhandled exception and spins. So each function's name is printed, and
// flushed, BEFORE that function is exercised. If the target goes quiet, the
// last line of output names the function that killed it.
//
// This must be built with -fno-builtin: otherwise the compiler replaces most of
// these calls with inline expansions or constant-folds them away, and the test
// silently stops testing libc at all.
// The test is intended to be run on riscv - arm is not affected.

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"

#define MAX_LEN     33      // enough to cover several word-loop iterations
#define PAD         8       // canary either side, and room to slide alignment
#define BUF_SIZE    (PAD + MAX_LEN + PAD)
#define CANARY      0xa5

static uint8_t src_buf[BUF_SIZE] __attribute__((aligned(8)));
static uint8_t dst_buf[BUF_SIZE] __attribute__((aligned(8)));

static int failures;
static const char *current;

static void fail(unsigned sa, unsigned da, unsigned len, const char *what) {
    printf("\nFAILED: %s (src_align=%u dst_align=%u len=%u): %s\n",
           current, sa, da, len, what);
    failures++;
}

// Reference implementations. Deliberately byte at a time, and deliberately not
// using any of the functions under test.
static size_t ref_strlen(const char *s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

static int ref_memcmp(const void *a, const void *b, size_t n) {
    const uint8_t *x = a, *y = b;
    for (size_t i = 0; i < n; i++) {
        if (x[i] != y[i]) return x[i] < y[i] ? -1 : 1;
    }
    return 0;
}

static int sign(int v) {
    return v < 0 ? -1 : (v > 0 ? 1 : 0);
}

// Fill src with a NUL-terminated pattern of len bytes, and reset both canaries.
static void setup(unsigned len, char *src) {
    memset(src_buf, CANARY, sizeof(src_buf));
    memset(dst_buf, CANARY, sizeof(dst_buf));
    for (unsigned i = 0; i < len; i++) {
        src[i] = (char)('a' + (i % 26));
    }
    src[len] = '\0';
}

static bool canaries_intact(const uint8_t *buf, const uint8_t *from, size_t written) {
    for (const uint8_t *p = buf; p < from; p++) {
        if (*p != CANARY) return false;
    }
    for (const uint8_t *p = from + written; p < buf + BUF_SIZE; p++) {
        if (*p != CANARY) return false;
    }
    return true;
}

typedef void (*case_fn)(unsigned sa, unsigned da, unsigned len, char *src, char *dst);

static void t_strlen(unsigned sa, unsigned da, unsigned len, char *src, char *dst) {
    (void)dst;
    if (strlen(src) != ref_strlen(src)) fail(sa, da, len, "wrong length");
}

static void t_strcmp(unsigned sa, unsigned da, unsigned len, char *src, char *dst) {
    memcpy(dst, src, len + 1);
    if (strcmp(src, dst) != 0) fail(sa, da, len, "equal strings compared unequal");
    if (len) {
        dst[len - 1] ^= 0x20;
        if (strcmp(src, dst) == 0) fail(sa, da, len, "differing strings compared equal");
    }
}

static void t_strncmp(unsigned sa, unsigned da, unsigned len, char *src, char *dst) {
    memcpy(dst, src, len + 1);
    if (strncmp(src, dst, len + 1) != 0) fail(sa, da, len, "equal strings compared unequal");
    if (len) {
        dst[len - 1] ^= 0x20;
        if (strncmp(src, dst, len) == 0) fail(sa, da, len, "differing strings compared equal");
        if (strncmp(src, dst, len - 1) != 0) fail(sa, da, len, "prefix compared unequal");
    }
}

static void t_memcmp(unsigned sa, unsigned da, unsigned len, char *src, char *dst) {
    memcpy(dst, src, len);
    if (memcmp(src, dst, len) != 0) fail(sa, da, len, "equal blocks compared unequal");
    if (len) {
        dst[len - 1] ^= 0xff;
        if (sign(memcmp(src, dst, len)) != sign(ref_memcmp(src, dst, len))) {
            fail(sa, da, len, "wrong sign");
        }
    }
}

static void t_memcpy(unsigned sa, unsigned da, unsigned len, char *src, char *dst) {
    if (memcpy(dst, src, len) != dst) fail(sa, da, len, "wrong return");
    if (ref_memcmp(dst, src, len) != 0) fail(sa, da, len, "wrong contents");
    if (!canaries_intact(dst_buf, (uint8_t *)dst, len)) fail(sa, da, len, "overran");
}

static void t_memmove(unsigned sa, unsigned da, unsigned len, char *src, char *dst) {
    if (memmove(dst, src, len) != dst) fail(sa, da, len, "wrong return");
    if (ref_memcmp(dst, src, len) != 0) fail(sa, da, len, "wrong contents");
    if (!canaries_intact(dst_buf, (uint8_t *)dst, len)) fail(sa, da, len, "overran");
    // and overlapping, which takes a different path
    if (len && sa + len + 1 < BUF_SIZE) {
        memmove(src + 1, src, len);
        for (unsigned i = 0; i < len; i++) {
            if (src[1 + i] != (char)('a' + (i % 26))) {
                fail(sa, da, len, "overlapping move corrupted data");
                break;
            }
        }
    }
}

static void t_memset(unsigned sa, unsigned da, unsigned len, char *src, char *dst) {
    (void)src;
    if (memset(dst, 0x5c, len) != dst) fail(sa, da, len, "wrong return");
    for (unsigned i = 0; i < len; i++) {
        if ((uint8_t)dst[i] != 0x5c) { fail(sa, da, len, "wrong contents"); break; }
    }
    if (!canaries_intact(dst_buf, (uint8_t *)dst, len)) fail(sa, da, len, "overran");
}

static void t_strcpy(unsigned sa, unsigned da, unsigned len, char *src, char *dst) {
    if (strcpy(dst, src) != dst) fail(sa, da, len, "wrong return");
    if (ref_memcmp(dst, src, len + 1) != 0) fail(sa, da, len, "wrong contents");
    if (!canaries_intact(dst_buf, (uint8_t *)dst, len + 1)) fail(sa, da, len, "overran");
}

static void t_strncpy(unsigned sa, unsigned da, unsigned len, char *src, char *dst) {
    if (strncpy(dst, src, len) != dst) fail(sa, da, len, "wrong return");
    if (ref_memcmp(dst, src, len) != 0) fail(sa, da, len, "wrong contents");
}

static void t_stpcpy(unsigned sa, unsigned da, unsigned len, char *src, char *dst) {
    if (stpcpy(dst, src) != dst + len) fail(sa, da, len, "wrong return");
    if (ref_memcmp(dst, src, len + 1) != 0) fail(sa, da, len, "wrong contents");
}

static void t_stpncpy(unsigned sa, unsigned da, unsigned len, char *src, char *dst) {
    if (stpncpy(dst, src, len) != dst + len) fail(sa, da, len, "wrong return");
    if (ref_memcmp(dst, src, len) != 0) fail(sa, da, len, "wrong contents");
}

static void t_strcat(unsigned sa, unsigned da, unsigned len, char *src, char *dst) {
    dst[0] = '\0';
    if (strcat(dst, src) != dst) fail(sa, da, len, "wrong return");
    if (ref_memcmp(dst, src, len + 1) != 0) fail(sa, da, len, "wrong contents");
}

static void t_strncat(unsigned sa, unsigned da, unsigned len, char *src, char *dst) {
    dst[0] = '\0';
    if (strncat(dst, src, len) != dst) fail(sa, da, len, "wrong return");
    if (ref_memcmp(dst, src, len) != 0) fail(sa, da, len, "wrong contents");
}

static void t_memchr(unsigned sa, unsigned da, unsigned len, char *src, char *dst) {
    (void)dst;
    if (!len) return;
    if (memchr(src, src[len - 1], len) == NULL) fail(sa, da, len, "did not find byte");
    if (memchr(src, 0x01, len) != NULL) fail(sa, da, len, "found absent byte");
}

static void t_strchr(unsigned sa, unsigned da, unsigned len, char *src, char *dst) {
    (void)dst;
    if (len && strchr(src, src[len - 1]) == NULL) fail(sa, da, len, "did not find char");
    if (strchr(src, '\0') != src + len) fail(sa, da, len, "wrong NUL position");
}

static void t_memccpy(unsigned sa, unsigned da, unsigned len, char *src, char *dst) {
    if (!len) return;
    // The fill pattern repeats every 26 bytes, so src[len-1] can also appear
    // earlier, and memccpy stops at the *first* match. Find where that is
    // rather than assuming it is the last byte.
    uint8_t needle = (uint8_t)src[len - 1];
    unsigned first = 0;
    while (first < len && (uint8_t)src[first] != needle) first++;
    void *end = memccpy(dst, src, needle, len);
    if (end != dst + first + 1) fail(sa, da, len, "wrong return");
    if (ref_memcmp(dst, src, first + 1) != 0) fail(sa, da, len, "wrong contents");
    if (!canaries_intact(dst_buf, (uint8_t *)dst, first + 1)) fail(sa, da, len, "overran");
}

static const struct {
    const char *name;
    case_fn fn;
} cases[] = {
    // Order matters for diagnosis
    // strlen only takes one pointer, so it can align itself and is safe even
    // in a broken libc -- it confirms the test is running at all.
    // strncmp is a generic C routine and strcmp is a RISC-V specific one,
    // and the two are configured independently, so running strncmp first
    // distinguishes a libc where only the generic half was fixed
    {"strlen",   t_strlen},
    {"strncmp",  t_strncmp},
    {"strcmp",   t_strcmp},
    {"memcmp",   t_memcmp},
    {"memcpy",   t_memcpy},
    {"memmove",  t_memmove},
    {"memset",   t_memset},
    {"strcpy",   t_strcpy},
    {"strncpy",  t_strncpy},
    {"stpcpy",   t_stpcpy},
    {"stpncpy",  t_stpncpy},
    {"strcat",   t_strcat},
    {"strncat",  t_strncat},
    {"memchr",   t_memchr},
    {"strchr",   t_strchr},
    {"memccpy",  t_memccpy},
};

int main(void) {
    stdio_init_all();
    printf("pico_unaligned_string_test\n");

    for (unsigned c = 0; c < count_of(cases); c++) {
        // Printed and flushed before the function is called, so a target that
        // dies mid-test leaves the culprit as the last line of output.
        printf("testing %s\n", cases[c].name);
        stdio_flush();

        current = cases[c].name;
        int before = failures;
        for (unsigned sa = 0; sa < PAD; sa++) {
            for (unsigned da = 0; da < PAD; da++) {
                for (unsigned len = 0; len <= MAX_LEN; len++) {
                    char *src = (char *)src_buf + sa;
                    char *dst = (char *)dst_buf + da;
                    setup(len, src);
                    cases[c].fn(sa, da, len, src, dst);
                }
            }
        }
        if (failures != before) {
            printf("  %s FAILED\n", cases[c].name);
        }
    }

    if (failures) {
        printf("\n%d FAILURES\n", failures);
    } else {
        printf("all functions handled every alignment correctly\n");
        printf("PASSED\n");
    }
    return 0;
}
