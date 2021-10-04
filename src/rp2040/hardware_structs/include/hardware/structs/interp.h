// THIS HEADER FILE IS AUTOMATICALLY GENERATED -- DO NOT EDIT

/*
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _HARDWARE_STRUCTS_INTERP_H
#define _HARDWARE_STRUCTS_INTERP_H

#include "hardware/address_mapped.h"
#include "hardware/regs/sio.h"

// reference to datasheet: https://datasheets.raspberrypi.org/rp2040/rp2040-datasheet.pdf#tab-registerlist_sio

// BITMASK [BITRANGE]: FIELDNAME (RESETVALUE): DESCRIPTION
// The REG macro is intended to help make the register navigable in your IDE (for example, using the "Go to Definition" feature)
// _REG_(x) will link to the corresponding register in hardware/regs/sio.h.

typedef struct {
    _REG_(SIO_INTERP0_ACCUM0_OFFSET)
    io_rw_32 accum[2];

    _REG_(SIO_INTERP0_BASE0_OFFSET)
    io_rw_32 base[3];

    _REG_(SIO_INTERP0_POP_LANE0_OFFSET)
    io_ro_32 pop[3];

    _REG_(SIO_INTERP0_PEEK_LANE0_OFFSET)
    io_ro_32 peek[3];

    _REG_(SIO_INTERP0_CTRL_LANE0_OFFSET)
    io_rw_32 ctrl[2];

    _REG_(SIO_INTERP0_ACCUM0_ADD_OFFSET)
    io_rw_32 add_raw[2];

    _REG_(SIO_INTERP0_BASE_1AND0_OFFSET)
    // On write, the lower 16 bits go to BASE0, upper bits to BASE1 simultaneously
    io_wo_32 base01;
} interp_hw_t;

#define interp_hw_array ((interp_hw_t *)(SIO_BASE + SIO_INTERP0_ACCUM0_OFFSET))
#define interp0_hw (&interp_hw_array[0])
#define interp1_hw (&interp_hw_array[1])

#endif
