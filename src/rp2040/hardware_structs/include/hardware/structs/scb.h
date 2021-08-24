// THIS HEADER FILE IS AUTOMATICALLY GENERATED -- DO NOT EDIT

/*
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _HARDWARE_STRUCTS_SCB_H
#define _HARDWARE_STRUCTS_SCB_H

#include "hardware/address_mapped.h"
#include "hardware/regs/m0plus.h"

// reference to datasheet: https://datasheets.raspberrypi.org/rp2040/rp2040-datasheet.pdf#tab-registerlist_m0plus

// BITMASK [BITRANGE]: FIELDNAME (RESETVALUE): DESCRIPTION
// The REG macro is intended to help make the register navigable in your IDE (for example, using the "Go to Definition" feature)
// _REG_(x) will link to the corresponding register in hardware/regs/m0plus.h.

typedef struct {
    _REG_(M0PLUS_CPUID_OFFSET)
    // Read the CPU ID Base Register to determine: the ID number of the processor core, the version...
    // 0xff000000 [24-31] : IMPLEMENTER (0x41): Implementor code: 0x41 = ARM
    // 0x00f00000 [20-23] : VARIANT (0): Major revision number n in the rnpm revision status:
    // 0x000f0000 [16-19] : ARCHITECTURE (0xc): Constant that defines the architecture of the processor:
    // 0x0000fff0 [4-15]  : PARTNO (0xc60): Number of processor within family: 0xC60 = Cortex-M0+
    // 0x0000000f [0-3]   : REVISION (1): Minor revision number m in the rnpm revision status:
    io_ro_32 cpuid;

    _REG_(M0PLUS_ICSR_OFFSET)
    // Use the Interrupt Control State Register to set a pending Non-Maskable Interrupt (NMI), set or...
    // 0x80000000 [31]    : NMIPENDSET (0): Setting this bit will activate an NMI
    // 0x10000000 [28]    : PENDSVSET (0): PendSV set-pending bit
    // 0x08000000 [27]    : PENDSVCLR (0): PendSV clear-pending bit
    // 0x04000000 [26]    : PENDSTSET (0): SysTick exception set-pending bit
    // 0x02000000 [25]    : PENDSTCLR (0): SysTick exception clear-pending bit
    // 0x00800000 [23]    : ISRPREEMPT (0): The system can only access this bit when the core is halted
    // 0x00400000 [22]    : ISRPENDING (0): External interrupt pending flag
    // 0x001ff000 [12-20] : VECTPENDING (0): Indicates the exception number for the highest priority...
    // 0x000001ff [0-8]   : VECTACTIVE (0): Active exception number field
    io_rw_32 icsr;

    _REG_(M0PLUS_VTOR_OFFSET)
    // The VTOR holds the vector table offset address
    // 0xffffff00 [8-31]  : TBLOFF (0): Bits [31:8] of the indicate the vector table offset address
    io_rw_32 vtor;

    _REG_(M0PLUS_AIRCR_OFFSET)
    // Use the Application Interrupt and Reset Control Register to: determine data endianness, clear...
    // 0xffff0000 [16-31] : VECTKEY (0): Register key:
    // 0x00008000 [15]    : ENDIANESS (0): Data endianness implemented:
    // 0x00000004 [2]     : SYSRESETREQ (0): Writing 1 to this bit causes the SYSRESETREQ signal to the...
    // 0x00000002 [1]     : VECTCLRACTIVE (0): Clears all active state information for fixed and...
    io_rw_32 aircr;

    _REG_(M0PLUS_SCR_OFFSET)
    // System Control Register
    // 0x00000010 [4]     : SEVONPEND (0): Send Event on Pending bit:
    // 0x00000004 [2]     : SLEEPDEEP (0): Controls whether the processor uses sleep or deep sleep as...
    // 0x00000002 [1]     : SLEEPONEXIT (0): Indicates sleep-on-exit when returning from Handler mode...
    io_rw_32 scr;
} armv6m_scb_t;

#define scb_hw ((armv6m_scb_t *const)(PPB_BASE + M0PLUS_CPUID_OFFSET))

#endif
