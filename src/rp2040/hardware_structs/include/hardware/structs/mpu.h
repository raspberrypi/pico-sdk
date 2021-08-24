// THIS HEADER FILE IS AUTOMATICALLY GENERATED -- DO NOT EDIT

/*
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _HARDWARE_STRUCTS_MPU_H
#define _HARDWARE_STRUCTS_MPU_H

#include "hardware/address_mapped.h"
#include "hardware/regs/m0plus.h"

// reference to datasheet: https://datasheets.raspberrypi.org/rp2040/rp2040-datasheet.pdf#tab-registerlist_m0plus

// BITMASK [BITRANGE]: FIELDNAME (RESETVALUE): DESCRIPTION
// The REG macro is intended to help make the register navigable in your IDE (for example, using the "Go to Definition" feature)
// _REG_(x) will link to the corresponding register in hardware/regs/m0plus.h.

typedef struct {
    _REG_(M0PLUS_MPU_TYPE_OFFSET)
    // Read the MPU Type Register to determine if the processor implements an MPU, and how many regions...
    // 0x00ff0000 [16-23] : IREGION (0): Instruction region
    // 0x0000ff00 [8-15]  : DREGION (0x8): Number of regions supported by the MPU
    // 0x00000001 [0]     : SEPARATE (0): Indicates support for separate instruction and data address maps
    io_ro_32 type;

    _REG_(M0PLUS_MPU_CTRL_OFFSET)
    // Use the MPU Control Register to enable and disable the MPU, and to control whether the default...
    // 0x00000004 [2]     : PRIVDEFENA (0): Controls whether the default memory map is enabled as a...
    // 0x00000002 [1]     : HFNMIENA (0): Controls the use of the MPU for HardFaults and NMIs
    // 0x00000001 [0]     : ENABLE (0): Enables the MPU
    io_rw_32 ctrl;

    _REG_(M0PLUS_MPU_RNR_OFFSET)
    // Use the MPU Region Number Register to select the region currently accessed by MPU_RBAR and MPU_RASR
    // 0x0000000f [0-3]   : REGION (0): Indicates the MPU region referenced by the MPU_RBAR and...
    io_rw_32 rnr;

    _REG_(M0PLUS_MPU_RBAR_OFFSET)
    // Read the MPU Region Base Address Register to determine the base address of the region identified...
    // 0xffffff00 [8-31]  : ADDR (0): Base address of the region
    // 0x00000010 [4]     : VALID (0): On writes, indicates whether the write must update the base...
    // 0x0000000f [0-3]   : REGION (0): On writes, specifies the number of the region whose base...
    io_rw_32 rbar;

    _REG_(M0PLUS_MPU_RASR_OFFSET)
    // Use the MPU Region Attribute and Size Register to define the size, access behaviour and memory...
    // 0xffff0000 [16-31] : ATTRS (0): The MPU Region Attribute field
    // 0x0000ff00 [8-15]  : SRD (0): Subregion Disable
    // 0x0000003e [1-5]   : SIZE (0): Indicates the region size
    // 0x00000001 [0]     : ENABLE (0): Enables the region
    io_rw_32 rasr;
} mpu_hw_t;

#define mpu_hw ((mpu_hw_t *const)(PPB_BASE + M0PLUS_MPU_TYPE_OFFSET))

#endif
