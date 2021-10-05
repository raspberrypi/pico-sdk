// THIS HEADER FILE IS AUTOMATICALLY GENERATED -- DO NOT EDIT

/*
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _HARDWARE_STRUCTS_PWM_H
#define _HARDWARE_STRUCTS_PWM_H

#include "hardware/address_mapped.h"
#include "hardware/regs/pwm.h"

// reference to datasheet: https://datasheets.raspberrypi.org/rp2040/rp2040-datasheet.pdf#tab-registerlist_pwm

// BITMASK [BITRANGE]: FIELDNAME (RESETVALUE): DESCRIPTION
// The _REG_ macro is intended to help make the register navigable in your IDE (for example, using the "Go to Definition" feature)
// _REG_(x) will link to the corresponding register in hardware/regs/pwm.h.

typedef struct pwm_slice_hw {
    _REG_(PWM_CH0_CSR_OFFSET)
    // Control and status register
    // 0x00000080 [7]     : PH_ADV (0): Advance the phase of the counter by 1 count, while it is running
    // 0x00000040 [6]     : PH_RET (0): Retard the phase of the counter by 1 count, while it is running
    // 0x00000030 [4-5]   : DIVMODE (0)
    // 0x00000008 [3]     : B_INV (0): Invert output B
    // 0x00000004 [2]     : A_INV (0): Invert output A
    // 0x00000002 [1]     : PH_CORRECT (0): 1: Enable phase-correct modulation
    // 0x00000001 [0]     : EN (0): Enable the PWM channel
    io_rw_32 csr;

    _REG_(PWM_CH0_DIV_OFFSET)
    // INT and FRAC form a fixed-point fractional number
    // 0x00000ff0 [4-11]  : INT (1)
    // 0x0000000f [0-3]   : FRAC (0)
    io_rw_32 div;

    _REG_(PWM_CH0_CTR_OFFSET)
    // Direct access to the PWM counter
    // 0x0000ffff [0-15]  : CH0_CTR (0)
    io_rw_32 ctr;

    _REG_(PWM_CH0_CC_OFFSET)
    // Counter compare values
    // 0xffff0000 [16-31] : B (0)
    // 0x0000ffff [0-15]  : A (0)
    io_rw_32 cc;

    _REG_(PWM_CH0_TOP_OFFSET)
    // Counter wrap value
    // 0x0000ffff [0-15]  : CH0_TOP (0xffff)
    io_rw_32 top;
} pwm_slice_hw_t;

typedef struct {
    pwm_slice_hw_t slice[NUM_PWM_SLICES]; // 8

    _REG_(PWM_EN_OFFSET)
    // This register aliases the CSR_EN bits for all channels
    // 0x00000080 [7]     : CH7 (0)
    // 0x00000040 [6]     : CH6 (0)
    // 0x00000020 [5]     : CH5 (0)
    // 0x00000010 [4]     : CH4 (0)
    // 0x00000008 [3]     : CH3 (0)
    // 0x00000004 [2]     : CH2 (0)
    // 0x00000002 [1]     : CH1 (0)
    // 0x00000001 [0]     : CH0 (0)
    io_rw_32 en;

    _REG_(PWM_INTR_OFFSET)
    // Raw Interrupts
    // 0x00000080 [7]     : CH7 (0)
    // 0x00000040 [6]     : CH6 (0)
    // 0x00000020 [5]     : CH5 (0)
    // 0x00000010 [4]     : CH4 (0)
    // 0x00000008 [3]     : CH3 (0)
    // 0x00000004 [2]     : CH2 (0)
    // 0x00000002 [1]     : CH1 (0)
    // 0x00000001 [0]     : CH0 (0)
    io_rw_32 intr;

    _REG_(PWM_INTE_OFFSET)
    // Interrupt Enable
    // 0x00000080 [7]     : CH7 (0)
    // 0x00000040 [6]     : CH6 (0)
    // 0x00000020 [5]     : CH5 (0)
    // 0x00000010 [4]     : CH4 (0)
    // 0x00000008 [3]     : CH3 (0)
    // 0x00000004 [2]     : CH2 (0)
    // 0x00000002 [1]     : CH1 (0)
    // 0x00000001 [0]     : CH0 (0)
    io_rw_32 inte;

    _REG_(PWM_INTF_OFFSET)
    // Interrupt Force
    // 0x00000080 [7]     : CH7 (0)
    // 0x00000040 [6]     : CH6 (0)
    // 0x00000020 [5]     : CH5 (0)
    // 0x00000010 [4]     : CH4 (0)
    // 0x00000008 [3]     : CH3 (0)
    // 0x00000004 [2]     : CH2 (0)
    // 0x00000002 [1]     : CH1 (0)
    // 0x00000001 [0]     : CH0 (0)
    io_rw_32 intf;

    _REG_(PWM_INTS_OFFSET)
    // Interrupt status after masking & forcing
    // 0x00000080 [7]     : CH7 (0)
    // 0x00000040 [6]     : CH6 (0)
    // 0x00000020 [5]     : CH5 (0)
    // 0x00000010 [4]     : CH4 (0)
    // 0x00000008 [3]     : CH3 (0)
    // 0x00000004 [2]     : CH2 (0)
    // 0x00000002 [1]     : CH1 (0)
    // 0x00000001 [0]     : CH0 (0)
    io_ro_32 ints;
} pwm_hw_t;

#define pwm_hw ((pwm_hw_t *const)PWM_BASE)

static_assert( NUM_PWM_SLICES == 8, "");

#endif
