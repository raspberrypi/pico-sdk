/**
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
// =============================================================================
// Register block : BUSCTRL
// Version        : 1
// Bus type       : apb
// Description    : Register block for busfabric control signals and performance
//                  counters
// =============================================================================
#ifndef HARDWARE_REGS_BUSCTRL_DEFINED
#define HARDWARE_REGS_BUSCTRL_DEFINED
// =============================================================================
// Register    : BUSCTRL_BUS_PRIORITY
// Description : Set the priority of each master for bus arbitration.
#define BUSCTRL_BUS_PRIORITY_OFFSET 0x00000000
#define BUSCTRL_BUS_PRIORITY_BITS   0x00001111
#define BUSCTRL_BUS_PRIORITY_RESET  0x00000000
// -----------------------------------------------------------------------------
// Field       : BUSCTRL_BUS_PRIORITY_DMA_W
// Description : 0 - low priority, 1 - high priority
#define BUSCTRL_BUS_PRIORITY_DMA_W_RESET  0x0
#define BUSCTRL_BUS_PRIORITY_DMA_W_BITS   0x00001000
#define BUSCTRL_BUS_PRIORITY_DMA_W_MSB    12
#define BUSCTRL_BUS_PRIORITY_DMA_W_LSB    12
#define BUSCTRL_BUS_PRIORITY_DMA_W_ACCESS "RW"
// -----------------------------------------------------------------------------
// Field       : BUSCTRL_BUS_PRIORITY_DMA_R
// Description : 0 - low priority, 1 - high priority
#define BUSCTRL_BUS_PRIORITY_DMA_R_RESET  0x0
#define BUSCTRL_BUS_PRIORITY_DMA_R_BITS   0x00000100
#define BUSCTRL_BUS_PRIORITY_DMA_R_MSB    8
#define BUSCTRL_BUS_PRIORITY_DMA_R_LSB    8
#define BUSCTRL_BUS_PRIORITY_DMA_R_ACCESS "RW"
// -----------------------------------------------------------------------------
// Field       : BUSCTRL_BUS_PRIORITY_PROC1
// Description : 0 - low priority, 1 - high priority
#define BUSCTRL_BUS_PRIORITY_PROC1_RESET  0x0
#define BUSCTRL_BUS_PRIORITY_PROC1_BITS   0x00000010
#define BUSCTRL_BUS_PRIORITY_PROC1_MSB    4
#define BUSCTRL_BUS_PRIORITY_PROC1_LSB    4
#define BUSCTRL_BUS_PRIORITY_PROC1_ACCESS "RW"
// -----------------------------------------------------------------------------
// Field       : BUSCTRL_BUS_PRIORITY_PROC0
// Description : 0 - low priority, 1 - high priority
#define BUSCTRL_BUS_PRIORITY_PROC0_RESET  0x0
#define BUSCTRL_BUS_PRIORITY_PROC0_BITS   0x00000001
#define BUSCTRL_BUS_PRIORITY_PROC0_MSB    0
#define BUSCTRL_BUS_PRIORITY_PROC0_LSB    0
#define BUSCTRL_BUS_PRIORITY_PROC0_ACCESS "RW"
// =============================================================================
// Register    : BUSCTRL_BUS_PRIORITY_ACK
// Description : Bus priority acknowledge
//               Goes to 1 once all arbiters have registered the new global
//               priority levels.
//               Arbiters update their local priority when servicing a new
//               nonsequential access.
//               In normal circumstances this will happen almost immediately.
#define BUSCTRL_BUS_PRIORITY_ACK_OFFSET 0x00000004
#define BUSCTRL_BUS_PRIORITY_ACK_BITS   0x00000001
#define BUSCTRL_BUS_PRIORITY_ACK_RESET  0x00000000
#define BUSCTRL_BUS_PRIORITY_ACK_MSB    0
#define BUSCTRL_BUS_PRIORITY_ACK_LSB    0
#define BUSCTRL_BUS_PRIORITY_ACK_ACCESS "RO"
// =============================================================================
// Register    : BUSCTRL_PERFCTR0
// Description : Bus fabric performance counter 0
//               Busfabric saturating performance counter 0
//               Count some event signal from the busfabric arbiters.
//               Write any value to clear. Select an event to count using
//               PERFSEL0
#define BUSCTRL_PERFCTR0_OFFSET 0x00000008
#define BUSCTRL_PERFCTR0_BITS   0x00ffffff
#define BUSCTRL_PERFCTR0_RESET  0x00000000
#define BUSCTRL_PERFCTR0_MSB    23
#define BUSCTRL_PERFCTR0_LSB    0
#define BUSCTRL_PERFCTR0_ACCESS "WC"
// =============================================================================
// Register    : BUSCTRL_PERFSEL0
// Description : Bus fabric performance event select for PERFCTR0
//               Select an event for PERFCTR0. Count either contested accesses,
//               or all accesses, on a downstream port of the main crossbar.
//               0x00 -> apb_contested
//               0x01 -> apb
//               0x02 -> fastperi_contested
//               0x03 -> fastperi
//               0x04 -> sram5_contested
//               0x05 -> sram5
//               0x06 -> sram4_contested
//               0x07 -> sram4
//               0x08 -> sram3_contested
//               0x09 -> sram3
//               0x0a -> sram2_contested
//               0x0b -> sram2
//               0x0c -> sram1_contested
//               0x0d -> sram1
//               0x0e -> sram0_contested
//               0x0f -> sram0
//               0x10 -> xip_main_contested
//               0x11 -> xip_main
//               0x12 -> rom_contested
//               0x13 -> rom
#define BUSCTRL_PERFSEL0_OFFSET                   0x0000000c
#define BUSCTRL_PERFSEL0_BITS                     0x0000001f
#define BUSCTRL_PERFSEL0_RESET                    0x0000001f
#define BUSCTRL_PERFSEL0_MSB                      4
#define BUSCTRL_PERFSEL0_LSB                      0
#define BUSCTRL_PERFSEL0_ACCESS                   "RW"
#define BUSCTRL_PERFSEL0_VALUE_APB_CONTESTED      0x00
#define BUSCTRL_PERFSEL0_VALUE_APB                0x01
#define BUSCTRL_PERFSEL0_VALUE_FASTPERI_CONTESTED 0x02
#define BUSCTRL_PERFSEL0_VALUE_FASTPERI           0x03
#define BUSCTRL_PERFSEL0_VALUE_SRAM5_CONTESTED    0x04
#define BUSCTRL_PERFSEL0_VALUE_SRAM5              0x05
#define BUSCTRL_PERFSEL0_VALUE_SRAM4_CONTESTED    0x06
#define BUSCTRL_PERFSEL0_VALUE_SRAM4              0x07
#define BUSCTRL_PERFSEL0_VALUE_SRAM3_CONTESTED    0x08
#define BUSCTRL_PERFSEL0_VALUE_SRAM3              0x09
#define BUSCTRL_PERFSEL0_VALUE_SRAM2_CONTESTED    0x0a
#define BUSCTRL_PERFSEL0_VALUE_SRAM2              0x0b
#define BUSCTRL_PERFSEL0_VALUE_SRAM1_CONTESTED    0x0c
#define BUSCTRL_PERFSEL0_VALUE_SRAM1              0x0d
#define BUSCTRL_PERFSEL0_VALUE_SRAM0_CONTESTED    0x0e
#define BUSCTRL_PERFSEL0_VALUE_SRAM0              0x0f
#define BUSCTRL_PERFSEL0_VALUE_XIP_MAIN_CONTESTED 0x10
#define BUSCTRL_PERFSEL0_VALUE_XIP_MAIN           0x11
#define BUSCTRL_PERFSEL0_VALUE_ROM_CONTESTED      0x12
#define BUSCTRL_PERFSEL0_VALUE_ROM                0x13
// =============================================================================
// Register    : BUSCTRL_PERFCTR1
// Description : Bus fabric performance counter 1
//               Busfabric saturating performance counter 1
//               Count some event signal from the busfabric arbiters.
//               Write any value to clear. Select an event to count using
//               PERFSEL1
#define BUSCTRL_PERFCTR1_OFFSET 0x00000010
#define BUSCTRL_PERFCTR1_BITS   0x00ffffff
#define BUSCTRL_PERFCTR1_RESET  0x00000000
#define BUSCTRL_PERFCTR1_MSB    23
#define BUSCTRL_PERFCTR1_LSB    0
#define BUSCTRL_PERFCTR1_ACCESS "WC"
// =============================================================================
// Register    : BUSCTRL_PERFSEL1
// Description : Bus fabric performance event select for PERFCTR1
//               Select an event for PERFCTR1. Count either contested accesses,
//               or all accesses, on a downstream port of the main crossbar.
//               0x00 -> apb_contested
//               0x01 -> apb
//               0x02 -> fastperi_contested
//               0x03 -> fastperi
//               0x04 -> sram5_contested
//               0x05 -> sram5
//               0x06 -> sram4_contested
//               0x07 -> sram4
//               0x08 -> sram3_contested
//               0x09 -> sram3
//               0x0a -> sram2_contested
//               0x0b -> sram2
//               0x0c -> sram1_contested
//               0x0d -> sram1
//               0x0e -> sram0_contested
//               0x0f -> sram0
//               0x10 -> xip_main_contested
//               0x11 -> xip_main
//               0x12 -> rom_contested
//               0x13 -> rom
#define BUSCTRL_PERFSEL1_OFFSET                   0x00000014
#define BUSCTRL_PERFSEL1_BITS                     0x0000001f
#define BUSCTRL_PERFSEL1_RESET                    0x0000001f
#define BUSCTRL_PERFSEL1_MSB                      4
#define BUSCTRL_PERFSEL1_LSB                      0
#define BUSCTRL_PERFSEL1_ACCESS                   "RW"
#define BUSCTRL_PERFSEL1_VALUE_APB_CONTESTED      0x00
#define BUSCTRL_PERFSEL1_VALUE_APB                0x01
#define BUSCTRL_PERFSEL1_VALUE_FASTPERI_CONTESTED 0x02
#define BUSCTRL_PERFSEL1_VALUE_FASTPERI           0x03
#define BUSCTRL_PERFSEL1_VALUE_SRAM5_CONTESTED    0x04
#define BUSCTRL_PERFSEL1_VALUE_SRAM5              0x05
#define BUSCTRL_PERFSEL1_VALUE_SRAM4_CONTESTED    0x06
#define BUSCTRL_PERFSEL1_VALUE_SRAM4              0x07
#define BUSCTRL_PERFSEL1_VALUE_SRAM3_CONTESTED    0x08
#define BUSCTRL_PERFSEL1_VALUE_SRAM3              0x09
#define BUSCTRL_PERFSEL1_VALUE_SRAM2_CONTESTED    0x0a
#define BUSCTRL_PERFSEL1_VALUE_SRAM2              0x0b
#define BUSCTRL_PERFSEL1_VALUE_SRAM1_CONTESTED    0x0c
#define BUSCTRL_PERFSEL1_VALUE_SRAM1              0x0d
#define BUSCTRL_PERFSEL1_VALUE_SRAM0_CONTESTED    0x0e
#define BUSCTRL_PERFSEL1_VALUE_SRAM0              0x0f
#define BUSCTRL_PERFSEL1_VALUE_XIP_MAIN_CONTESTED 0x10
#define BUSCTRL_PERFSEL1_VALUE_XIP_MAIN           0x11
#define BUSCTRL_PERFSEL1_VALUE_ROM_CONTESTED      0x12
#define BUSCTRL_PERFSEL1_VALUE_ROM                0x13
// =============================================================================
// Register    : BUSCTRL_PERFCTR2
// Description : Bus fabric performance counter 2
//               Busfabric saturating performance counter 2
//               Count some event signal from the busfabric arbiters.
//               Write any value to clear. Select an event to count using
//               PERFSEL2
#define BUSCTRL_PERFCTR2_OFFSET 0x00000018
#define BUSCTRL_PERFCTR2_BITS   0x00ffffff
#define BUSCTRL_PERFCTR2_RESET  0x00000000
#define BUSCTRL_PERFCTR2_MSB    23
#define BUSCTRL_PERFCTR2_LSB    0
#define BUSCTRL_PERFCTR2_ACCESS "WC"
// =============================================================================
// Register    : BUSCTRL_PERFSEL2
// Description : Bus fabric performance event select for PERFCTR2
//               Select an event for PERFCTR2. Count either contested accesses,
//               or all accesses, on a downstream port of the main crossbar.
//               0x00 -> apb_contested
//               0x01 -> apb
//               0x02 -> fastperi_contested
//               0x03 -> fastperi
//               0x04 -> sram5_contested
//               0x05 -> sram5
//               0x06 -> sram4_contested
//               0x07 -> sram4
//               0x08 -> sram3_contested
//               0x09 -> sram3
//               0x0a -> sram2_contested
//               0x0b -> sram2
//               0x0c -> sram1_contested
//               0x0d -> sram1
//               0x0e -> sram0_contested
//               0x0f -> sram0
//               0x10 -> xip_main_contested
//               0x11 -> xip_main
//               0x12 -> rom_contested
//               0x13 -> rom
#define BUSCTRL_PERFSEL2_OFFSET                   0x0000001c
#define BUSCTRL_PERFSEL2_BITS                     0x0000001f
#define BUSCTRL_PERFSEL2_RESET                    0x0000001f
#define BUSCTRL_PERFSEL2_MSB                      4
#define BUSCTRL_PERFSEL2_LSB                      0
#define BUSCTRL_PERFSEL2_ACCESS                   "RW"
#define BUSCTRL_PERFSEL2_VALUE_APB_CONTESTED      0x00
#define BUSCTRL_PERFSEL2_VALUE_APB                0x01
#define BUSCTRL_PERFSEL2_VALUE_FASTPERI_CONTESTED 0x02
#define BUSCTRL_PERFSEL2_VALUE_FASTPERI           0x03
#define BUSCTRL_PERFSEL2_VALUE_SRAM5_CONTESTED    0x04
#define BUSCTRL_PERFSEL2_VALUE_SRAM5              0x05
#define BUSCTRL_PERFSEL2_VALUE_SRAM4_CONTESTED    0x06
#define BUSCTRL_PERFSEL2_VALUE_SRAM4              0x07
#define BUSCTRL_PERFSEL2_VALUE_SRAM3_CONTESTED    0x08
#define BUSCTRL_PERFSEL2_VALUE_SRAM3              0x09
#define BUSCTRL_PERFSEL2_VALUE_SRAM2_CONTESTED    0x0a
#define BUSCTRL_PERFSEL2_VALUE_SRAM2              0x0b
#define BUSCTRL_PERFSEL2_VALUE_SRAM1_CONTESTED    0x0c
#define BUSCTRL_PERFSEL2_VALUE_SRAM1              0x0d
#define BUSCTRL_PERFSEL2_VALUE_SRAM0_CONTESTED    0x0e
#define BUSCTRL_PERFSEL2_VALUE_SRAM0              0x0f
#define BUSCTRL_PERFSEL2_VALUE_XIP_MAIN_CONTESTED 0x10
#define BUSCTRL_PERFSEL2_VALUE_XIP_MAIN           0x11
#define BUSCTRL_PERFSEL2_VALUE_ROM_CONTESTED      0x12
#define BUSCTRL_PERFSEL2_VALUE_ROM                0x13
// =============================================================================
// Register    : BUSCTRL_PERFCTR3
// Description : Bus fabric performance counter 3
//               Busfabric saturating performance counter 3
//               Count some event signal from the busfabric arbiters.
//               Write any value to clear. Select an event to count using
//               PERFSEL3
#define BUSCTRL_PERFCTR3_OFFSET 0x00000020
#define BUSCTRL_PERFCTR3_BITS   0x00ffffff
#define BUSCTRL_PERFCTR3_RESET  0x00000000
#define BUSCTRL_PERFCTR3_MSB    23
#define BUSCTRL_PERFCTR3_LSB    0
#define BUSCTRL_PERFCTR3_ACCESS "WC"
// =============================================================================
// Register    : BUSCTRL_PERFSEL3
// Description : Bus fabric performance event select for PERFCTR3
//               Select an event for PERFCTR3. Count either contested accesses,
//               or all accesses, on a downstream port of the main crossbar.
//               0x00 -> apb_contested
//               0x01 -> apb
//               0x02 -> fastperi_contested
//               0x03 -> fastperi
//               0x04 -> sram5_contested
//               0x05 -> sram5
//               0x06 -> sram4_contested
//               0x07 -> sram4
//               0x08 -> sram3_contested
//               0x09 -> sram3
//               0x0a -> sram2_contested
//               0x0b -> sram2
//               0x0c -> sram1_contested
//               0x0d -> sram1
//               0x0e -> sram0_contested
//               0x0f -> sram0
//               0x10 -> xip_main_contested
//               0x11 -> xip_main
//               0x12 -> rom_contested
//               0x13 -> rom
#define BUSCTRL_PERFSEL3_OFFSET                   0x00000024
#define BUSCTRL_PERFSEL3_BITS                     0x0000001f
#define BUSCTRL_PERFSEL3_RESET                    0x0000001f
#define BUSCTRL_PERFSEL3_MSB                      4
#define BUSCTRL_PERFSEL3_LSB                      0
#define BUSCTRL_PERFSEL3_ACCESS                   "RW"
#define BUSCTRL_PERFSEL3_VALUE_APB_CONTESTED      0x00
#define BUSCTRL_PERFSEL3_VALUE_APB                0x01
#define BUSCTRL_PERFSEL3_VALUE_FASTPERI_CONTESTED 0x02
#define BUSCTRL_PERFSEL3_VALUE_FASTPERI           0x03
#define BUSCTRL_PERFSEL3_VALUE_SRAM5_CONTESTED    0x04
#define BUSCTRL_PERFSEL3_VALUE_SRAM5              0x05
#define BUSCTRL_PERFSEL3_VALUE_SRAM4_CONTESTED    0x06
#define BUSCTRL_PERFSEL3_VALUE_SRAM4              0x07
#define BUSCTRL_PERFSEL3_VALUE_SRAM3_CONTESTED    0x08
#define BUSCTRL_PERFSEL3_VALUE_SRAM3              0x09
#define BUSCTRL_PERFSEL3_VALUE_SRAM2_CONTESTED    0x0a
#define BUSCTRL_PERFSEL3_VALUE_SRAM2              0x0b
#define BUSCTRL_PERFSEL3_VALUE_SRAM1_CONTESTED    0x0c
#define BUSCTRL_PERFSEL3_VALUE_SRAM1              0x0d
#define BUSCTRL_PERFSEL3_VALUE_SRAM0_CONTESTED    0x0e
#define BUSCTRL_PERFSEL3_VALUE_SRAM0              0x0f
#define BUSCTRL_PERFSEL3_VALUE_XIP_MAIN_CONTESTED 0x10
#define BUSCTRL_PERFSEL3_VALUE_XIP_MAIN           0x11
#define BUSCTRL_PERFSEL3_VALUE_ROM_CONTESTED      0x12
#define BUSCTRL_PERFSEL3_VALUE_ROM                0x13
// =============================================================================
#endif // HARDWARE_REGS_BUSCTRL_DEFINED
