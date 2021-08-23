// THIS HEADER FILE IS AUTOMATICALLY GENERATED -- DO NOT EDIT

/*
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _HARDWARE_STRUCTS_PADSBANK0_H
#define _HARDWARE_STRUCTS_PADSBANK0_H

#include "hardware/address_mapped.h"
#include "hardware/regs/pads_bank0.h"

// reference to datasheet: https://datasheets.raspberrypi.org/rp2040/rp2040-datasheet.pdf#tab-registerlist_pads_bank0
// BITMASK : FIELDNAME [BITRANGE] (RESETVALUE): DESCRIPTION

typedef struct {
    _REG_(PADS_BANK0_VOLTAGE_SELECT_OFFSET)
    // Voltage select
    // 0x00000001 [0]     : VOLTAGE_SELECT (0)
    io_rw_32 voltage_select;

    _REG_(PADS_BANK0_None_OFFSET)
    io_rw_32 io[30];

} padsbank0_hw_t;

#define padsbank0_hw ((padsbank0_hw_t *const)PADS_BANK0_BASE)

#endif
