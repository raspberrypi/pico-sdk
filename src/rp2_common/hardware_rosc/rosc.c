/*
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico.h"

// For MHZ definitions etc
#include "hardware/clocks.h"
#include "hardware/rosc.h"


uint rosc_measure_freq_khz(void) {
    return frequency_count_khz(CLOCKS_FC0_SRC_VALUE_ROSC_CLKSRC);
}

void rosc_disable(void) {
    uint32_t tmp = rosc_hw->ctrl;
    tmp &= (~ROSC_CTRL_ENABLE_BITS);
    tmp |= (ROSC_CTRL_ENABLE_VALUE_DISABLE << ROSC_CTRL_ENABLE_LSB);
    rosc_write(&rosc_hw->ctrl, tmp);
    // Wait for stable to go away
    while (rosc_hw->status & ROSC_STATUS_STABLE_BITS);
}

void rosc_set_dormant(void) {
    // WARNING: This stops the rosc until woken up by an irq
    rosc_write(&rosc_hw->dormant, ROSC_DORMANT_VALUE_DORMANT);
    // Wait for it to become stable once woken up
    while (!(rosc_hw->status & ROSC_STATUS_STABLE_BITS));
}

void rosc_restart(void) {
    //Re-enable the rosc
    rosc_write(&rosc_hw->ctrl, ROSC_CTRL_ENABLE_VALUE_ENABLE << ROSC_CTRL_ENABLE_LSB);

    //Wait for it to become stable once restarted
    while (!(rosc_hw->status & ROSC_STATUS_STABLE_BITS));
}