// ----------------------------------------------------------------------------
// Second stage boot code
// Copyright (c) 2019-2022 Raspberry Pi (Trading) Ltd.
// SPDX-License-Identifier: BSD-3-Clause
//
// Device:      Generic QSPI PSRAM
//
// Description: Configures PSRAM to run in Quad I/O continuous read & write XIP
//              modes
//
// Details:     * Check status register 2 to determine if QSPI mode is enabled,
//                and perform an SR2 programming cycle if necessary.
//              * Use SSI to perform a dummy 0xEB read command, with the mode
//                continuation bits set, so that the flash will not require
//                0xEB instruction prefix on subsequent reads.
//              * Configure SSI to write address, mode bits, but no instruction.
//                SSI + flash are now jointly in a state where continuous reads
//                can take place.
//              * Jump to exit pointer passed in via lr. Bootrom passes null,
//                in which case this code uses a default 256 byte flash offset
//
// Building:    * This code must be position-independent, and use stack only
//              * The code will be padded to a size of 256 bytes, including a
//                4-byte checksum. Therefore code size cannot exceed 252 bytes.
// ----------------------------------------------------------------------------

#include "hardware/regs/addressmap.h"
#include "hardware/regs/pads_qspi.h"
#include "hardware/regs/qmi.h"
#if PICO_RP2350_A2_SUPPORTED
#include "hardware/regs/pads_bank0.h"
#endif

// ----------------------------------------------------------------------------
// Config section
// ----------------------------------------------------------------------------
// It should be possible to support most flash devices by modifying this section

// The serial flash interface will run at clk_sys/PICO_PSRAM_SPI_CLKDIV.
// This must be a positive integer.
// The bootrom is very conservative with SPI frequency, but here we should be
// as aggressive as possible.

#ifndef PICO_PSRAM_SPI_CLKDIV
#define PICO_PSRAM_SPI_CLKDIV 2
#endif
#if (PICO_PSRAM_SPI_CLKDIV << QMI_M1_TIMING_CLKDIV_LSB) & ~QMI_M1_TIMING_CLKDIV_BITS
#error "CLKDIV greater than maximum"
#endif

// RX sampling delay is measured in units of one half clock cycle.

#ifndef PICO_PSRAM_SPI_RXDELAY
#define PICO_PSRAM_SPI_RXDELAY 2
#endif
#if (PICO_PSRAM_SPI_RXDELAY << QMI_M1_TIMING_RXDELAY_LSB) & ~QMI_M1_TIMING_RXDELAY_BITS
#error "RX delay greater than maximum"
#endif

#ifndef PICO_PSRAM_SPI_MAX_SELECT
#define PICO_PSRAM_SPI_MAX_SELECT 18
#endif

#ifndef PICO_PSRAM_SPI_MIN_DESELECT
#define PICO_PSRAM_SPI_MIN_DESELECT 3
#endif

// For W25Q080 this is the "Read data fast quad IO" instruction:
#define CMD_READ 0xeb
#define CMD_WRITE 0x38
#define CMD_QUAD_ENABLE 0x35

#define CMD_WRITE_ENABLE 0x06
#define CMD_READ_STATUS 0x05
#define CMD_READ_STATUS2 0x35
#define CMD_WRITE_STATUS 0x01

// ----------------------------------------------------------------------------
// Register initialisation values -- same in Arm/RISC-V code.
// ----------------------------------------------------------------------------

// SCLK: 8mA drive, no slew limiting, input buffer disabled
#define INIT_PAD_SCLK  (\
    2 << PADS_QSPI_GPIO_QSPI_SCLK_DRIVE_LSB | \
    PADS_QSPI_GPIO_QSPI_SCLK_SLEWFAST_BITS | \
0)

// Need to use direct serial mode to send SR commands. Choose a
// conservative direct-mode divisor (5 MHz at 150 MHz clk_sys)
// since the XIP-mode divisor may be unsafe without an RX delay.
#define INIT_DIRECT_CSR (\
    30 << QMI_DIRECT_CSR_CLKDIV_LSB | \
    QMI_DIRECT_CSR_EN_BITS | \
    QMI_DIRECT_CSR_AUTO_CS0N_BITS | \
0)

// Setup timing parameters: short sequential-access cooldown, configured
// CLKDIV and RXDELAY, and no constraints on CS max assertion, CS min
// deassertion, or page boundary burst breaks.
#define INIT_M1_TIMING (\
    1                           << QMI_M1_TIMING_COOLDOWN_LSB |\
    QMI_M1_TIMING_PAGEBREAK_VALUE_1024 \
                                << QMI_M1_TIMING_PAGEBREAK_LSB |\
    PICO_PSRAM_SPI_MAX_SELECT   << QMI_M1_TIMING_MAX_SELECT_LSB |\
    PICO_PSRAM_SPI_MIN_DESELECT << QMI_M1_TIMING_MIN_DESELECT_LSB |\
    PICO_PSRAM_SPI_RXDELAY      << QMI_M1_TIMING_RXDELAY_LSB |\
    PICO_PSRAM_SPI_CLKDIV       << QMI_M1_TIMING_CLKDIV_LSB |\
0)

// Set command constants (e.g. EBh + A0h)
#define INIT_M1_RCMD (\
    CMD_READ             << QMI_M1_RCMD_PREFIX_LSB |\
    0                    << QMI_M1_RCMD_SUFFIX_LSB |\
0)

#define INIT_M1_WCMD (\
    CMD_WRITE            << QMI_M1_WCMD_PREFIX_LSB |\
    0                    << QMI_M1_WCMD_SUFFIX_LSB |\
0)

// Set read format (serial command prefix, everything else quad)
// Initial dummy transfer has a serial EBh prefix, and has XIP mode bits
// set after the address. Subsequent transfers don't need the prefix, as long
// as the mode bits are set correctly.
#define INIT_M1_RFMT (\
    QMI_M1_RFMT_PREFIX_WIDTH_VALUE_Q    << QMI_M1_RFMT_PREFIX_WIDTH_LSB |\
    QMI_M1_RFMT_ADDR_WIDTH_VALUE_Q      << QMI_M1_RFMT_ADDR_WIDTH_LSB |\
    QMI_M1_RFMT_SUFFIX_WIDTH_VALUE_Q    << QMI_M1_RFMT_SUFFIX_WIDTH_LSB |\
    QMI_M1_RFMT_DUMMY_WIDTH_VALUE_Q     << QMI_M1_RFMT_DUMMY_WIDTH_LSB |\
    QMI_M1_RFMT_DATA_WIDTH_VALUE_Q      << QMI_M1_RFMT_DATA_WIDTH_LSB |\
    QMI_M1_RFMT_PREFIX_LEN_VALUE_8      << QMI_M1_RFMT_PREFIX_LEN_LSB |\
    QMI_M1_RFMT_SUFFIX_LEN_VALUE_NONE   << QMI_M1_RFMT_SUFFIX_LEN_LSB |\
    QMI_M1_RFMT_DUMMY_LEN_VALUE_24      << QMI_M1_RFMT_DUMMY_LEN_LSB |\
0)

#define INIT_M1_WFMT (\
    QMI_M1_WFMT_PREFIX_WIDTH_VALUE_Q    << QMI_M1_WFMT_PREFIX_WIDTH_LSB |\
    QMI_M1_WFMT_ADDR_WIDTH_VALUE_Q      << QMI_M1_WFMT_ADDR_WIDTH_LSB |\
    QMI_M1_WFMT_SUFFIX_WIDTH_VALUE_Q    << QMI_M1_WFMT_SUFFIX_WIDTH_LSB |\
    QMI_M1_WFMT_DUMMY_WIDTH_VALUE_Q     << QMI_M1_WFMT_DUMMY_WIDTH_LSB |\
    QMI_M1_WFMT_DATA_WIDTH_VALUE_Q      << QMI_M1_WFMT_DATA_WIDTH_LSB |\
    QMI_M1_WFMT_PREFIX_LEN_VALUE_8      << QMI_M1_WFMT_PREFIX_LEN_LSB |\
    QMI_M1_WFMT_SUFFIX_LEN_VALUE_NONE   << QMI_M1_WFMT_SUFFIX_LEN_LSB |\
    QMI_M1_WFMT_DUMMY_LEN_VALUE_NONE      << QMI_M1_WFMT_DUMMY_LEN_LSB |\
0)

// Setup PSRAM. Clobbers r0/a0 (Arm/RISC-V)

// ----------------------------------------------------------------------------
// RISC-V implementation

#ifdef __riscv

// On RP2350 boot stage2 is always called as a regular function, and should return normally
setup_psram:
    mv t1, ra
#if PICO_RP2350_A2_SUPPORTED
_psram_cs_iso_clear:
    // E14 Workaround
    li a3, PADS_BANK0_BASE
    ldr a0, =(PADS_BANK0_GPIO0_RESET & ~PADS_BANK0_GPIO0_ISO_BITS)
    str a0, (PADS_BANK0_GPIO0_OFFSET + (4 * PICO_PSRAM_CS_PIN))(a3)
    li a3, XIP_QMI_BASE
#endif
_psram_quad_setup:
    li a0, (10 << QMI_DIRECT_CSR_CLKDIV_LSB |\
            QMI_DIRECT_CSR_EN_BITS |\
            QMI_DIRECT_CSR_AUTO_CS1N_BITS)
    sw a0, QMI_DIRECT_CSR_OFFSET(a3)
    jal wait_qmi_ready
    li a0, CMD_QUAD_ENABLE
    sw a0, QMI_DIRECT_TX_OFFSET(a3)
    jal wait_qmi_ready
    sw zero, QMI_DIRECT_CSR_OFFSET(a3)


_psram_qmi_config:
    li a0, INIT_M1_TIMING
    sw a0, QMI_M1_TIMING_OFFSET(a3)
    li a0, INIT_M1_RCMD
    sw a0, QMI_M1_RCMD_OFFSET(a3)
    li a0, INIT_M1_RFMT
    sw a0, QMI_M1_RFMT_OFFSET(a3)
    li a0, INIT_M1_WCMD
    sw a0, QMI_M1_WCMD_OFFSET(a3)
    li a0, INIT_M1_WFMT
    sw a0, QMI_M1_WFMT_OFFSET(a3)

    jr t1

// ----------------------------------------------------------------------------
// Arm implementation

#else

// On RP2350 boot stage2 is always called as a regular function, and should return normally
setup_psram:
    push {lr}
#if PICO_RP2350_A2_SUPPORTED
_psram_cs_iso_clear:
    // E14 Workaround
    ldr r3, =PADS_BANK0_BASE
    ldr r0, =(PADS_BANK0_GPIO0_RESET & ~PADS_BANK0_GPIO0_ISO_BITS)
    str r0, [r3, #(PADS_BANK0_GPIO0_OFFSET + (4 * PICO_PSRAM_CS_PIN))]
    ldr r3, =XIP_QMI_BASE
#endif
_psram_quad_setup:
    ldr r0, =(10 << QMI_DIRECT_CSR_CLKDIV_LSB |\
            QMI_DIRECT_CSR_EN_BITS |\
            QMI_DIRECT_CSR_AUTO_CS1N_BITS)
    str r0, [r3, #QMI_DIRECT_CSR_OFFSET]
    bl wait_qmi_ready
    ldr r0, =CMD_QUAD_ENABLE
    str r0, [r3, #QMI_DIRECT_TX_OFFSET]
    bl wait_qmi_ready
    ldr r0, =0
    str r0, [r3, #QMI_DIRECT_CSR_OFFSET]

_psram_qmi_config:
    ldr r0, =INIT_M1_TIMING
    str r0, [r3, #QMI_M1_TIMING_OFFSET]
    ldr r0, =INIT_M1_RCMD
    str r0, [r3, #QMI_M1_RCMD_OFFSET]
    ldr r0, =INIT_M1_RFMT
    str r0, [r3, #QMI_M1_RFMT_OFFSET]
    ldr r0, =INIT_M1_WCMD
    str r0, [r3, #QMI_M1_WCMD_OFFSET]
    ldr r0, =INIT_M1_WFMT
    str r0, [r3, #QMI_M1_WFMT_OFFSET]

    pop {pc}

#endif

// ----------------------------------------------------------------------------

// Common functions
#include "boot2_helpers/wait_qmi_ready.S"
