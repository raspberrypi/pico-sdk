// THIS HEADER FILE IS AUTOMATICALLY GENERATED -- DO NOT EDIT

/*
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _HARDWARE_STRUCTS_PADS_QSPI_H
#define _HARDWARE_STRUCTS_PADS_QSPI_H

#include "hardware/address_mapped.h"
#include "hardware/regs/pads_qspi.h"

// reference to datasheet: https://datasheets.raspberrypi.org/rp2040/rp2040-datasheet.pdf#tab-registerlist_pads_qspi
//
// The _REG_ macro is intended to help make the register navigable in your IDE (for example, using the "Go to Definition" feature)
// _REG_(x) will link to the corresponding register in hardware/regs/pads_qspi.h.
//
// Bit-field descriptions are of the form:
// BITMASK [BITRANGE]: FIELDNAME (RESETVALUE): DESCRIPTION

typedef struct {
    _REG_(PADS_QSPI_VOLTAGE_SELECT_OFFSET) // PADS_QSPI_VOLTAGE_SELECT
    // Voltage select
    // 0x00000001 [0]     : VOLTAGE_SELECT (0)
    io_rw_32 voltage_select;

    _REG_(PADS_QSPI_GPIO_QSPI_SCLK_OFFSET) // PADS_QSPI_GPIO_QSPI_SCLK
    io_rw_32 io[NUM_QSPI_GPIOS]; // 6
} pads_qspi_hw_t;

#define pads_qspi_hw ((pads_qspi_hw_t *const)PADS_QSPI_BASE)

static_assert( NUM_QSPI_GPIOS == 6, "");

#endif
