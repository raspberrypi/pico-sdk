/*
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Checks that PICO_UBSAN_ALIGNMENT_CHECKS catches a misaligned access that
// the compiler's static checks cannot see.
//
// RP2350's RISC-V cores fault on any misaligned load or store, where the Arm
// cores handle it in hardware. An unaligned access therefore hangs on RISC-V
// and passes silently on Arm.

#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"

static uint8_t buf[16] __attribute__((aligned(4)));

// volatile so the compiler cannot fold the accesses below into constants.
static volatile int offset;
static volatile uint32_t sink;

int main(void) {
    stdio_init_all();

    for (int i = 0; i < 16; i++) {
        buf[i] = (uint8_t)i;
    }

    printf("pico_ubsan_align_test\n");

    // A correctly aligned access through the same code path must not trip the
    // check
    offset = 4;
    void *aligned = &buf[offset];
    sink = *(uint32_t *)aligned;
    printf("aligned load at offset 4 returned 0x%08x\n", (unsigned)sink);

    // The real test. buf is 4-byte aligned, so offset 1 is not.
    offset = 1;
    void *misaligned = &buf[offset];
    printf("loading 4 bytes from %p -- expecting a ubsan panic now\n", misaligned);
    sink = *(uint32_t *)misaligned;

#if PICO_UBSAN_RECOVER
    // The recoverable handler reports and returns. On Arm the access then
    // succeeds and we reach here; on RISC-V it faults first, so this line is
    // only expected on Arm.
    printf("load returned 0x%08x and execution continued, as expected with"
           " PICO_UBSAN_RECOVER\n", (unsigned)sink);
#else
    printf("FAILED: the misaligned load was not detected (returned 0x%08x)\n", (unsigned)sink);
#endif
    while (true) {
        tight_loop_contents();
    }
}
