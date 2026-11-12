/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico.h"
#include "hardware/adc.h"
#include "hardware/resets.h"

void adc_init(void) {
    // ADC is in an unknown state. We should start by resetting it
    reset_unreset_block_num_wait_blocking(RESET_ADC);

    // Now turn it back on. Staging of clock etc is handled internally
    adc_hw->cs = ADC_CS_EN_BITS;

    // Internal staging completes in a few cycles, but poll to be sure
    while (!(adc_hw->cs & ADC_CS_READY_BITS)) {
        tight_loop_contents();
    }
}

void adc_deinit(void) {
    // Power off the ADC and disable its clock. // TS also turned off 
    adc_hw->cs = 0x00;
}
