/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico.h"
#include "hardware/address_mapped.h"
#include "hardware/regs/tbman.h"

// Note we leave the FPGA check in by default so that we can run bug repro
// binaries coming in from the wild on the FPGA platform. It takes up around
// 48 bytes if you include all the calls, so you can pass PICO_NO_FPGA_CHECK=1
// to remove it. The FPGA check is used to skip initialisation of hardware
// (mainly clock generators and oscillators) that aren't present on FPGA.

#if !PICO_NO_FPGA_CHECK
// Inline stub provided in header if this code is unused (so folding can be
// done in each TU instead of relying on LTO)
bool __attribute__((weak)) running_on_fpga(void) {
    return (*(io_ro_32 *)TBMAN_BASE) & TBMAN_PLATFORM_FPGA_BITS;
}
#endif

#if !PICO_NO_SIM_CHECK
bool __attribute__((weak)) running_in_sim(void) {
    return (*(io_ro_32 *)TBMAN_BASE) & TBMAN_PLATFORM_HDLSIM_BITS;
}
#endif

