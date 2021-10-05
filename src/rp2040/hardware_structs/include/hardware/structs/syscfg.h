// THIS HEADER FILE IS AUTOMATICALLY GENERATED -- DO NOT EDIT

/*
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _HARDWARE_STRUCTS_SYSCFG_H
#define _HARDWARE_STRUCTS_SYSCFG_H

#include "hardware/address_mapped.h"
#include "hardware/regs/syscfg.h"

// reference to datasheet: https://datasheets.raspberrypi.org/rp2040/rp2040-datasheet.pdf#tab-registerlist_syscfg

// BITMASK [BITRANGE]: FIELDNAME (RESETVALUE): DESCRIPTION
// The _REG_ macro is intended to help make the register navigable in your IDE (for example, using the "Go to Definition" feature)
// _REG_(x) will link to the corresponding register in hardware/regs/syscfg.h.

typedef struct {
    _REG_(SYSCFG_PROC0_NMI_MASK_OFFSET)
    // Processor core 0 NMI source mask
    io_rw_32 proc0_nmi_mask;

    _REG_(SYSCFG_PROC1_NMI_MASK_OFFSET)
    // Processor core 1 NMI source mask
    io_rw_32 proc1_nmi_mask;

    _REG_(SYSCFG_PROC_CONFIG_OFFSET)
    // Configuration for processors
    // 0xf0000000 [28-31] : PROC1_DAP_INSTID (1): Configure proc1 DAP instance ID
    // 0x0f000000 [24-27] : PROC0_DAP_INSTID (0): Configure proc0 DAP instance ID
    // 0x00000002 [1]     : PROC1_HALTED (0): Indication that proc1 has halted
    // 0x00000001 [0]     : PROC0_HALTED (0): Indication that proc0 has halted
    io_rw_32 proc_config;

    _REG_(SYSCFG_PROC_IN_SYNC_BYPASS_OFFSET)
    // For each bit, if 1, bypass the input synchronizer between that GPIO
    // 0x3fffffff [0-29]  : PROC_IN_SYNC_BYPASS (0)
    io_rw_32 proc_in_sync_bypass;

    _REG_(SYSCFG_PROC_IN_SYNC_BYPASS_HI_OFFSET)
    // For each bit, if 1, bypass the input synchronizer between that GPIO
    // 0x0000003f [0-5]   : PROC_IN_SYNC_BYPASS_HI (0)
    io_rw_32 proc_in_sync_bypass_hi;

    _REG_(SYSCFG_DBGFORCE_OFFSET)
    // Directly control the SWD debug port of either processor
    // 0x00000080 [7]     : PROC1_ATTACH (0): Attach processor 1 debug port to syscfg controls, and disconnect it from...
    // 0x00000040 [6]     : PROC1_SWCLK (1): Directly drive processor 1 SWCLK, if PROC1_ATTACH is set
    // 0x00000020 [5]     : PROC1_SWDI (1): Directly drive processor 1 SWDIO input, if PROC1_ATTACH is set
    // 0x00000010 [4]     : PROC1_SWDO (0): Observe the value of processor 1 SWDIO output
    // 0x00000008 [3]     : PROC0_ATTACH (0): Attach processor 0 debug port to syscfg controls, and disconnect it from...
    // 0x00000004 [2]     : PROC0_SWCLK (1): Directly drive processor 0 SWCLK, if PROC0_ATTACH is set
    // 0x00000002 [1]     : PROC0_SWDI (1): Directly drive processor 0 SWDIO input, if PROC0_ATTACH is set
    // 0x00000001 [0]     : PROC0_SWDO (0): Observe the value of processor 0 SWDIO output
    io_rw_32 dbgforce;

    _REG_(SYSCFG_MEMPOWERDOWN_OFFSET)
    // Control power downs to memories
    // 0x00000080 [7]     : ROM (0)
    // 0x00000040 [6]     : USB (0)
    // 0x00000020 [5]     : SRAM5 (0)
    // 0x00000010 [4]     : SRAM4 (0)
    // 0x00000008 [3]     : SRAM3 (0)
    // 0x00000004 [2]     : SRAM2 (0)
    // 0x00000002 [1]     : SRAM1 (0)
    // 0x00000001 [0]     : SRAM0 (0)
    io_rw_32 mempowerdown;
} syscfg_hw_t;

#define syscfg_hw ((syscfg_hw_t *const)SYSCFG_BASE)

#endif
