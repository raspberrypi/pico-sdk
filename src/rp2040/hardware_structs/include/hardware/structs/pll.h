// THIS HEADER FILE IS AUTOMATICALLY GENERATED -- DO NOT EDIT

/*
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _HARDWARE_STRUCTS_PLL_H
#define _HARDWARE_STRUCTS_PLL_H

#include "hardware/address_mapped.h"
#include "hardware/regs/pll.h"

// reference to datasheet: https://datasheets.raspberrypi.org/rp2040/rp2040-datasheet.pdf#tab-registerlist_pll

// BITMASK [BITRANGE]: FIELDNAME (RESETVALUE): DESCRIPTION
// The _REG_ macro is intended to help make the register navigable in your IDE (for example, using the "Go to Definition" feature)
// _REG_(x) will link to the corresponding register in hardware/regs/pll.h.

/// \tag::pll_hw[]
typedef struct {
    _REG_(PLL_CS_OFFSET)
    // Control and Status
    // 0x80000000 [31]    : LOCK (0): PLL is locked
    // 0x00000100 [8]     : BYPASS (0): Passes the reference clock to the output instead of the divided VCO
    // 0x0000003f [0-5]   : REFDIV (1): Divides the PLL input reference clock
    io_rw_32 cs;

    _REG_(PLL_PWR_OFFSET)
    // Controls the PLL power modes
    // 0x00000020 [5]     : VCOPD (1): PLL VCO powerdown
    // 0x00000008 [3]     : POSTDIVPD (1): PLL post divider powerdown
    // 0x00000004 [2]     : DSMPD (1): PLL DSM powerdown
    // 0x00000001 [0]     : PD (1): PLL powerdown
    io_rw_32 pwr;

    _REG_(PLL_FBDIV_INT_OFFSET)
    // Feedback divisor
    // 0x00000fff [0-11]  : FBDIV_INT (0): see ctrl reg description for constraints
    io_rw_32 fbdiv_int;

    _REG_(PLL_PRIM_OFFSET)
    // Controls the PLL post dividers for the primary output
    // 0x00070000 [16-18] : POSTDIV1 (0x7): divide by 1-7
    // 0x00007000 [12-14] : POSTDIV2 (0x7): divide by 1-7
    io_rw_32 prim;
} pll_hw_t;

#define pll_sys_hw ((pll_hw_t *const)PLL_SYS_BASE)
#define pll_usb_hw ((pll_hw_t *const)PLL_USB_BASE)
/// \end::pll_hw[]

#endif
