// THIS HEADER FILE IS AUTOMATICALLY GENERATED -- DO NOT EDIT

/*
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _HARDWARE_STRUCTS_IOQSPI_H
#define _HARDWARE_STRUCTS_IOQSPI_H

#include "hardware/address_mapped.h"
#include "hardware/regs/io_qspi.h"

// reference to datasheet: https://datasheets.raspberrypi.org/rp2040/rp2040-datasheet.pdf#tab-registerlist_io_qspi
// BITMASK : FIELDNAME [BITRANGE] (RESETVALUE): DESCRIPTION

typedef struct {
    _REG_(IO_QSPI_GPIO_QSPI_SCLK_STATUS_OFFSET)
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

    _REG_(IO_QSPI_GPIO_QSPI_SCLK_CTRL_OFFSET)
    // GPIO control including function select and overrides
    // 0x30000000 [28-29] : IRQOVER (0)
    // 0x00030000 [16-17] : INOVER (0)
    // 0x00003000 [12-13] : OEOVER (0)
    // 0x00000300 [8-9]   : OUTOVER (0)
    // 0x0000001f [0-4]   : FUNCSEL (0x1f): 0-31 -> selects pin function according to the gpio table
    io_rw_32 ctrl;

} io_status_ctrl_hw_t;

typedef struct {
    _REG_(IO_QSPI_PROC0_INTE_OFFSET)
    // Interrupt Enable for proc0
    // 0x00800000 [23]    : GPIO_QSPI_SD3_EDGE_HIGH (0)
    // 0x00400000 [22]    : GPIO_QSPI_SD3_EDGE_LOW (0)
    // 0x00200000 [21]    : GPIO_QSPI_SD3_LEVEL_HIGH (0)
    // 0x00100000 [20]    : GPIO_QSPI_SD3_LEVEL_LOW (0)
    // 0x00080000 [19]    : GPIO_QSPI_SD2_EDGE_HIGH (0)
    // 0x00040000 [18]    : GPIO_QSPI_SD2_EDGE_LOW (0)
    // 0x00020000 [17]    : GPIO_QSPI_SD2_LEVEL_HIGH (0)
    // 0x00010000 [16]    : GPIO_QSPI_SD2_LEVEL_LOW (0)
    // 0x00008000 [15]    : GPIO_QSPI_SD1_EDGE_HIGH (0)
    // 0x00004000 [14]    : GPIO_QSPI_SD1_EDGE_LOW (0)
    // 0x00002000 [13]    : GPIO_QSPI_SD1_LEVEL_HIGH (0)
    // 0x00001000 [12]    : GPIO_QSPI_SD1_LEVEL_LOW (0)
    // 0x00000800 [11]    : GPIO_QSPI_SD0_EDGE_HIGH (0)
    // 0x00000400 [10]    : GPIO_QSPI_SD0_EDGE_LOW (0)
    // 0x00000200 [9]     : GPIO_QSPI_SD0_LEVEL_HIGH (0)
    // 0x00000100 [8]     : GPIO_QSPI_SD0_LEVEL_LOW (0)
    // 0x00000080 [7]     : GPIO_QSPI_SS_EDGE_HIGH (0)
    // 0x00000040 [6]     : GPIO_QSPI_SS_EDGE_LOW (0)
    // 0x00000020 [5]     : GPIO_QSPI_SS_LEVEL_HIGH (0)
    // 0x00000010 [4]     : GPIO_QSPI_SS_LEVEL_LOW (0)
    // 0x00000008 [3]     : GPIO_QSPI_SCLK_EDGE_HIGH (0)
    // 0x00000004 [2]     : GPIO_QSPI_SCLK_EDGE_LOW (0)
    // 0x00000002 [1]     : GPIO_QSPI_SCLK_LEVEL_HIGH (0)
    // 0x00000001 [0]     : GPIO_QSPI_SCLK_LEVEL_LOW (0)
    io_rw_32 inte;

    _REG_(IO_QSPI_PROC0_INTF_OFFSET)
    // Interrupt Force for proc0
    // 0x00800000 [23]    : GPIO_QSPI_SD3_EDGE_HIGH (0)
    // 0x00400000 [22]    : GPIO_QSPI_SD3_EDGE_LOW (0)
    // 0x00200000 [21]    : GPIO_QSPI_SD3_LEVEL_HIGH (0)
    // 0x00100000 [20]    : GPIO_QSPI_SD3_LEVEL_LOW (0)
    // 0x00080000 [19]    : GPIO_QSPI_SD2_EDGE_HIGH (0)
    // 0x00040000 [18]    : GPIO_QSPI_SD2_EDGE_LOW (0)
    // 0x00020000 [17]    : GPIO_QSPI_SD2_LEVEL_HIGH (0)
    // 0x00010000 [16]    : GPIO_QSPI_SD2_LEVEL_LOW (0)
    // 0x00008000 [15]    : GPIO_QSPI_SD1_EDGE_HIGH (0)
    // 0x00004000 [14]    : GPIO_QSPI_SD1_EDGE_LOW (0)
    // 0x00002000 [13]    : GPIO_QSPI_SD1_LEVEL_HIGH (0)
    // 0x00001000 [12]    : GPIO_QSPI_SD1_LEVEL_LOW (0)
    // 0x00000800 [11]    : GPIO_QSPI_SD0_EDGE_HIGH (0)
    // 0x00000400 [10]    : GPIO_QSPI_SD0_EDGE_LOW (0)
    // 0x00000200 [9]     : GPIO_QSPI_SD0_LEVEL_HIGH (0)
    // 0x00000100 [8]     : GPIO_QSPI_SD0_LEVEL_LOW (0)
    // 0x00000080 [7]     : GPIO_QSPI_SS_EDGE_HIGH (0)
    // 0x00000040 [6]     : GPIO_QSPI_SS_EDGE_LOW (0)
    // 0x00000020 [5]     : GPIO_QSPI_SS_LEVEL_HIGH (0)
    // 0x00000010 [4]     : GPIO_QSPI_SS_LEVEL_LOW (0)
    // 0x00000008 [3]     : GPIO_QSPI_SCLK_EDGE_HIGH (0)
    // 0x00000004 [2]     : GPIO_QSPI_SCLK_EDGE_LOW (0)
    // 0x00000002 [1]     : GPIO_QSPI_SCLK_LEVEL_HIGH (0)
    // 0x00000001 [0]     : GPIO_QSPI_SCLK_LEVEL_LOW (0)
    io_rw_32 intf;

    _REG_(IO_QSPI_PROC0_INTS_OFFSET)
    // Interrupt status after masking & forcing for proc0
    // 0x00800000 [23]    : GPIO_QSPI_SD3_EDGE_HIGH (0)
    // 0x00400000 [22]    : GPIO_QSPI_SD3_EDGE_LOW (0)
    // 0x00200000 [21]    : GPIO_QSPI_SD3_LEVEL_HIGH (0)
    // 0x00100000 [20]    : GPIO_QSPI_SD3_LEVEL_LOW (0)
    // 0x00080000 [19]    : GPIO_QSPI_SD2_EDGE_HIGH (0)
    // 0x00040000 [18]    : GPIO_QSPI_SD2_EDGE_LOW (0)
    // 0x00020000 [17]    : GPIO_QSPI_SD2_LEVEL_HIGH (0)
    // 0x00010000 [16]    : GPIO_QSPI_SD2_LEVEL_LOW (0)
    // 0x00008000 [15]    : GPIO_QSPI_SD1_EDGE_HIGH (0)
    // 0x00004000 [14]    : GPIO_QSPI_SD1_EDGE_LOW (0)
    // 0x00002000 [13]    : GPIO_QSPI_SD1_LEVEL_HIGH (0)
    // 0x00001000 [12]    : GPIO_QSPI_SD1_LEVEL_LOW (0)
    // 0x00000800 [11]    : GPIO_QSPI_SD0_EDGE_HIGH (0)
    // 0x00000400 [10]    : GPIO_QSPI_SD0_EDGE_LOW (0)
    // 0x00000200 [9]     : GPIO_QSPI_SD0_LEVEL_HIGH (0)
    // 0x00000100 [8]     : GPIO_QSPI_SD0_LEVEL_LOW (0)
    // 0x00000080 [7]     : GPIO_QSPI_SS_EDGE_HIGH (0)
    // 0x00000040 [6]     : GPIO_QSPI_SS_EDGE_LOW (0)
    // 0x00000020 [5]     : GPIO_QSPI_SS_LEVEL_HIGH (0)
    // 0x00000010 [4]     : GPIO_QSPI_SS_LEVEL_LOW (0)
    // 0x00000008 [3]     : GPIO_QSPI_SCLK_EDGE_HIGH (0)
    // 0x00000004 [2]     : GPIO_QSPI_SCLK_EDGE_LOW (0)
    // 0x00000002 [1]     : GPIO_QSPI_SCLK_LEVEL_HIGH (0)
    // 0x00000001 [0]     : GPIO_QSPI_SCLK_LEVEL_LOW (0)
    io_ro_32 ints;

} io_qspi_ctrl_hw_t;

typedef struct {
    io_status_ctrl_hw_t io[6];

    _REG_(IO_QSPI_INTR_OFFSET)
    // Raw Interrupts
    // 0x00800000 [23]    : GPIO_QSPI_SD3_EDGE_HIGH (0)
    // 0x00400000 [22]    : GPIO_QSPI_SD3_EDGE_LOW (0)
    // 0x00200000 [21]    : GPIO_QSPI_SD3_LEVEL_HIGH (0)
    // 0x00100000 [20]    : GPIO_QSPI_SD3_LEVEL_LOW (0)
    // 0x00080000 [19]    : GPIO_QSPI_SD2_EDGE_HIGH (0)
    // 0x00040000 [18]    : GPIO_QSPI_SD2_EDGE_LOW (0)
    // 0x00020000 [17]    : GPIO_QSPI_SD2_LEVEL_HIGH (0)
    // 0x00010000 [16]    : GPIO_QSPI_SD2_LEVEL_LOW (0)
    // 0x00008000 [15]    : GPIO_QSPI_SD1_EDGE_HIGH (0)
    // 0x00004000 [14]    : GPIO_QSPI_SD1_EDGE_LOW (0)
    // 0x00002000 [13]    : GPIO_QSPI_SD1_LEVEL_HIGH (0)
    // 0x00001000 [12]    : GPIO_QSPI_SD1_LEVEL_LOW (0)
    // 0x00000800 [11]    : GPIO_QSPI_SD0_EDGE_HIGH (0)
    // 0x00000400 [10]    : GPIO_QSPI_SD0_EDGE_LOW (0)
    // 0x00000200 [9]     : GPIO_QSPI_SD0_LEVEL_HIGH (0)
    // 0x00000100 [8]     : GPIO_QSPI_SD0_LEVEL_LOW (0)
    // 0x00000080 [7]     : GPIO_QSPI_SS_EDGE_HIGH (0)
    // 0x00000040 [6]     : GPIO_QSPI_SS_EDGE_LOW (0)
    // 0x00000020 [5]     : GPIO_QSPI_SS_LEVEL_HIGH (0)
    // 0x00000010 [4]     : GPIO_QSPI_SS_LEVEL_LOW (0)
    // 0x00000008 [3]     : GPIO_QSPI_SCLK_EDGE_HIGH (0)
    // 0x00000004 [2]     : GPIO_QSPI_SCLK_EDGE_LOW (0)
    // 0x00000002 [1]     : GPIO_QSPI_SCLK_LEVEL_HIGH (0)
    // 0x00000001 [0]     : GPIO_QSPI_SCLK_LEVEL_LOW (0)
    io_rw_32 intr;

    io_qspi_ctrl_hw_t proc0_qspi_ctrl;

    io_qspi_ctrl_hw_t proc1_qspi_ctrl;

    io_qspi_ctrl_hw_t dormant_wake_qspi_ctrl;

} ioqspi_hw_t;

#define io_qspi_hw ((ioqspi_hw_t *const)IO_QSPI_BASE)

#endif
