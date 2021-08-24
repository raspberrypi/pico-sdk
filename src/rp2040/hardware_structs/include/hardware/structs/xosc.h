// THIS HEADER FILE IS AUTOMATICALLY GENERATED -- DO NOT EDIT

/*
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _HARDWARE_STRUCTS_XOSC_H
#define _HARDWARE_STRUCTS_XOSC_H

#include "hardware/address_mapped.h"
#include "hardware/regs/xosc.h"

// reference to datasheet: https://datasheets.raspberrypi.org/rp2040/rp2040-datasheet.pdf#tab-registerlist_xosc

// BITMASK [BITRANGE]: FIELDNAME (RESETVALUE): DESCRIPTION
// The REG macro is intended to help make the register navigable in your IDE (for example, using the "Go to Definition" feature)
// _REG_(x) will link to the corresponding register in hardware/regs/xosc.h.

/// \tag::xosc_hw[]
typedef struct {
    _REG_(XOSC_CTRL_OFFSET)
    // Crystal Oscillator Control
    // 0x00fff000 [12-23] : ENABLE (0): On power-up this field is initialised to DISABLE and the chip...
    // 0x00000fff [0-11]  : FREQ_RANGE (0): Frequency range
    io_rw_32 ctrl;

    _REG_(XOSC_STATUS_OFFSET)
    // Crystal Oscillator Status
    // 0x80000000 [31]    : STABLE (0): Oscillator is running and stable
    // 0x01000000 [24]    : BADWRITE (0): An invalid value has been written to CTRL_ENABLE or...
    // 0x00001000 [12]    : ENABLED (0): Oscillator is enabled but not necessarily running and stable,...
    // 0x00000003 [0-1]   : FREQ_RANGE (0): The current frequency range setting, always reads 0
    io_rw_32 status;

    _REG_(XOSC_DORMANT_OFFSET)
    // Crystal Oscillator pause control
    io_rw_32 dormant;

    _REG_(XOSC_STARTUP_OFFSET)
    // Controls the startup delay
    // 0x00100000 [20]    : X4 (0): Multiplies the startup_delay by 4
    // 0x00003fff [0-13]  : DELAY (0): in multiples of 256*xtal_period
    io_rw_32 startup;

    uint32_t _pad0[3];

    _REG_(XOSC_COUNT_OFFSET)
    // A down counter running at the xosc frequency which counts to zero and stops
    // 0x000000ff [0-7]   : COUNT (0)
    io_rw_32 count;
} xosc_hw_t;

#define xosc_hw ((xosc_hw_t *const)XOSC_BASE)
/// \end::xosc_hw[]

#endif
