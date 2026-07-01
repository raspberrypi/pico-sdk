// THIS HEADER FILE IS AUTOMATICALLY GENERATED -- DO NOT EDIT

/**
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _HARDWARE_STRUCTS_BUSCTRL_H
#define _HARDWARE_STRUCTS_BUSCTRL_H

/**
 * \file rp2350/busctrl.h
 */

#include "hardware/address_mapped.h"
#include "hardware/regs/busctrl.h"

// Reference to datasheet: https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf#tab-registerlist_busctrl
//
// The _REG_ macro is intended to help make the register navigable in your IDE (for example, using the "Go to Definition" feature)
// _REG_(x) will link to the corresponding register in hardware/regs/busctrl.h.
//
// Bit-field descriptions are of the form:
// BITMASK [BITRANGE] FIELDNAME (RESETVALUE) DESCRIPTION

/** \brief Bus fabric performance counters on RP2350 (used as typedef \ref bus_ctrl_perf_counter_t)
 */
typedef enum bus_ctrl_perf_counter_rp2350 {
    arbiter_rom_perf_event_access = 67,
    arbiter_rom_perf_event_access_contested = 66,
    arbiter_rom_perf_event_stall_downstream = 65,
    arbiter_rom_perf_event_stall_upstream = 64,
    arbiter_xip_main0_perf_event_access = 63,
    arbiter_xip_main0_perf_event_access_contested = 62,
    arbiter_xip_main0_perf_event_stall_downstream = 61,
    arbiter_xip_main0_perf_event_stall_upstream = 60,
    arbiter_xip_main1_perf_event_access = 59,
    arbiter_xip_main1_perf_event_access_contested = 58,
    arbiter_xip_main1_perf_event_stall_downstream = 57,
    arbiter_xip_main1_perf_event_stall_upstream = 56,
    arbiter_sram0_perf_event_access = 55,
    arbiter_sram0_perf_event_access_contested = 54,
    arbiter_sram0_perf_event_stall_downstream = 53,
    arbiter_sram0_perf_event_stall_upstream = 52,
    arbiter_sram1_perf_event_access = 51,
    arbiter_sram1_perf_event_access_contested = 50,
    arbiter_sram1_perf_event_stall_downstream = 49,
    arbiter_sram1_perf_event_stall_upstream = 48,
    arbiter_sram2_perf_event_access = 47,
    arbiter_sram2_perf_event_access_contested = 46,
    arbiter_sram2_perf_event_stall_downstream = 45,
    arbiter_sram2_perf_event_stall_upstream = 44,
    arbiter_sram3_perf_event_access = 43,
    arbiter_sram3_perf_event_access_contested = 42,
    arbiter_sram3_perf_event_stall_downstream = 41,
    arbiter_sram3_perf_event_stall_upstream = 40,
    arbiter_sram4_perf_event_access = 39,
    arbiter_sram4_perf_event_access_contested = 38,
    arbiter_sram4_perf_event_stall_downstream = 37,
    arbiter_sram4_perf_event_stall_upstream = 36,
    arbiter_sram5_perf_event_access = 35,
    arbiter_sram5_perf_event_access_contested = 34,
    arbiter_sram5_perf_event_stall_downstream = 33,
    arbiter_sram5_perf_event_stall_upstream = 32,
    arbiter_sram6_perf_event_access = 31,
    arbiter_sram6_perf_event_access_contested = 30,
    arbiter_sram6_perf_event_stall_downstream = 29,
    arbiter_sram6_perf_event_stall_upstream = 28,
    arbiter_sram7_perf_event_access = 27,
    arbiter_sram7_perf_event_access_contested = 26,
    arbiter_sram7_perf_event_stall_downstream = 25,
    arbiter_sram7_perf_event_stall_upstream = 24,
    arbiter_sram8_perf_event_access = 23,
    arbiter_sram8_perf_event_access_contested = 22,
    arbiter_sram8_perf_event_stall_downstream = 21,
    arbiter_sram8_perf_event_stall_upstream = 20,
    arbiter_sram9_perf_event_access = 19,
    arbiter_sram9_perf_event_access_contested = 18,
    arbiter_sram9_perf_event_stall_downstream = 17,
    arbiter_sram9_perf_event_stall_upstream = 16,
    arbiter_fastperi_perf_event_access = 15,
    arbiter_fastperi_perf_event_access_contested = 14,
    arbiter_fastperi_perf_event_stall_downstream = 13,
    arbiter_fastperi_perf_event_stall_upstream = 12,
    arbiter_apb_perf_event_access = 11,
    arbiter_apb_perf_event_access_contested = 10,
    arbiter_apb_perf_event_stall_downstream = 9,
    arbiter_apb_perf_event_stall_upstream = 8,
    arbiter_sio_proc0_perf_event_access = 7,
    arbiter_sio_proc0_perf_event_access_contested = 6,
    arbiter_sio_proc0_perf_event_stall_downstream = 5,
    arbiter_sio_proc0_perf_event_stall_upstream = 4,
    arbiter_sio_proc1_perf_event_access = 3,
    arbiter_sio_proc1_perf_event_access_contested = 2,
    arbiter_sio_proc1_perf_event_stall_downstream = 1,
    arbiter_sio_proc1_perf_event_stall_upstream = 0
} bus_ctrl_perf_counter_t;

typedef struct {
    _REG_(BUSCTRL_PERFCTR0_OFFSET) // BUSCTRL_PERFCTR0
    // Bus fabric performance counter 0
    // 0x00ffffff [23:0]  PERFCTR0     (0x000000) Busfabric saturating performance counter 0 +
    io_rw_32 value;

    _REG_(BUSCTRL_PERFSEL0_OFFSET) // BUSCTRL_PERFSEL0
    // Bus fabric performance event select for PERFCTR0
    // 0x0000007f [6:0]   PERFSEL0     (0x1f) Select an event for PERFCTR0
    io_rw_32 sel;
} bus_ctrl_perf_hw_t;

typedef struct {
    _REG_(BUSCTRL_BUS_PRIORITY_OFFSET) // BUSCTRL_BUS_PRIORITY
    // Set the priority of each master for bus arbitration
    // 0x00001000 [12]    DMA_W        (0) 0 - low priority, 1 - high priority
    // 0x00000100 [8]     DMA_R        (0) 0 - low priority, 1 - high priority
    // 0x00000010 [4]     PROC1        (0) 0 - low priority, 1 - high priority
    // 0x00000001 [0]     PROC0        (0) 0 - low priority, 1 - high priority
    io_rw_32 priority;

    _REG_(BUSCTRL_BUS_PRIORITY_ACK_OFFSET) // BUSCTRL_BUS_PRIORITY_ACK
    // Bus priority acknowledge
    // 0x00000001 [0]     BUS_PRIORITY_ACK (0) Goes to 1 once all arbiters have registered the new...
    io_ro_32 priority_ack;

    _REG_(BUSCTRL_PERFCTR_EN_OFFSET) // BUSCTRL_PERFCTR_EN
    // Enable the performance counters
    // 0x00000001 [0]     PERFCTR_EN   (0)
    io_rw_32 perfctr_en;

    bus_ctrl_perf_hw_t counter[4];
} busctrl_hw_t;

#define busctrl_hw ((busctrl_hw_t *)BUSCTRL_BASE)
static_assert(sizeof (busctrl_hw_t) == 0x002c, "");

#endif // _HARDWARE_STRUCTS_BUSCTRL_H
