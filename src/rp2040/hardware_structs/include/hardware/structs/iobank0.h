// THIS HEADER FILE IS AUTOMATICALLY GENERATED -- DO NOT EDIT

/*
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _HARDWARE_STRUCTS_IOBANK0_H
#define _HARDWARE_STRUCTS_IOBANK0_H

#include "hardware/address_mapped.h"
#include "hardware/regs/io_bank0.h"

// reference to datasheet: https://datasheets.raspberrypi.org/rp2040/rp2040-datasheet.pdf#tab-registerlist_io_bank0

// BITMASK [BITRANGE]: FIELDNAME (RESETVALUE): DESCRIPTION
// The REG macro is intended to help make the register navigable in your IDE (for example, using the "Go to Definition" feature)
// _REG_(x) will link to the corresponding register in hardware/regs/io_bank0.h.

typedef struct {
    _REG_(IO_BANK0_GPIO0_STATUS_OFFSET)
    // GPIO status
    // 0x04000000 [26]    : IRQTOPROC (0): interrupt to processors, after override is applied
    // 0x01000000 [24]    : IRQFROMPAD (0): interrupt from pad before override is applied
    // 0x00080000 [19]    : INTOPERI (0): input signal to peripheral, after override is applied
    // 0x00020000 [17]    : INFROMPAD (0): input signal from pad, before override is applied
    // 0x00002000 [13]    : OETOPAD (0): output enable to pad after register override is applied
    // 0x00001000 [12]    : OEFROMPERI (0): output enable from selected peripheral, before register...
    // 0x00000200 [9]     : OUTTOPAD (0): output signal to pad after register override is applied
    // 0x00000100 [8]     : OUTFROMPERI (0): output signal from selected peripheral, before register...
    io_ro_32 status;

    _REG_(IO_BANK0_GPIO0_CTRL_OFFSET)
    // GPIO control including function select and overrides
    // 0x30000000 [28-29] : IRQOVER (0)
    // 0x00030000 [16-17] : INOVER (0)
    // 0x00003000 [12-13] : OEOVER (0)
    // 0x00000300 [8-9]   : OUTOVER (0)
    // 0x0000001f [0-4]   : FUNCSEL (0x1f): 0-31 -> selects pin function according to the gpio table
    io_rw_32 ctrl;
} io_status_ctrl_hw_t;

typedef struct {
    _REG_(IO_BANK0_PROC0_INTE0_OFFSET)
    io_rw_32 inte[4];

    _REG_(IO_BANK0_PROC0_INTF0_OFFSET)
    io_rw_32 intf[4];

    _REG_(IO_BANK0_PROC0_INTS0_OFFSET)
    io_ro_32 ints[4];
} io_irq_ctrl_hw_t;

/// \tag::iobank0_hw[]
typedef struct {
    io_status_ctrl_hw_t io[30];

    _REG_(IO_BANK0_INTR0_OFFSET)
    io_rw_32 intr[4];

    io_irq_ctrl_hw_t proc0_irq_ctrl;

    io_irq_ctrl_hw_t proc1_irq_ctrl;

    io_irq_ctrl_hw_t dormant_wake_irq_ctrl;
} iobank0_hw_t;

#define iobank0_hw ((iobank0_hw_t *const)IO_BANK0_BASE)
/// \end::iobank0_hw[]

#endif
