/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico.h"

// For MHZ definitions etc
#include "hardware/clocks.h"

#include "hardware/platform_defs.h"
#include "hardware/regs/xosc.h"
#include "hardware/structs/xosc.h"

// PICO_CONFIG: PICO_XOSC_STARTUP_DELAY_MULTIPLIER, Multiplier to lengthen xosc startup delay to accommodate slow-starting oscillators, type=int, min=1, default=1, group=hardware_xosc
#ifndef PICO_XOSC_STARTUP_DELAY_MULTIPLIER
#define PICO_XOSC_STARTUP_DELAY_MULTIPLIER 1
#endif

void xosc_init(void) {
    // Assumes 1-15 MHz input
    assert(XOSC_MHZ <= 15);
    xosc_hw->ctrl = XOSC_CTRL_FREQ_RANGE_VALUE_1_15MHZ;

    // Set xosc startup delay
    uint32_t startup_delay = (((12 * MHZ) / 1000) + 128) / 256;

    // Allow lengthening startup delay to accommodate slow-starting oscillators
    xosc_hw->startup = startup_delay * PICO_XOSC_STARTUP_DELAY_MULTIPLIER;

    // Set the enable bit now that we have set freq range and startup delay
    hw_set_bits(&xosc_hw->ctrl, XOSC_CTRL_ENABLE_VALUE_ENABLE << XOSC_CTRL_ENABLE_LSB);

    // Wait for XOSC to be stable
    while(!(xosc_hw->status & XOSC_STATUS_STABLE_BITS));
}

void xosc_disable(void) {
    uint32_t tmp = xosc_hw->ctrl;
    tmp &= (~XOSC_CTRL_ENABLE_BITS);
    tmp |= (XOSC_CTRL_ENABLE_VALUE_DISABLE << XOSC_CTRL_ENABLE_LSB);
    xosc_hw->ctrl = tmp;
    // Wait for stable to go away
    while(xosc_hw->status & XOSC_STATUS_STABLE_BITS);
}

void xosc_dormant(void) {
    // WARNING: This stops the xosc until woken up by an irq
    xosc_hw->dormant = XOSC_DORMANT_VALUE_DORMANT;
    // Wait for it to become stable once woken up
    while(!(xosc_hw->status & XOSC_STATUS_STABLE_BITS));
}
