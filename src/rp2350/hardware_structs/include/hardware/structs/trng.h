// THIS HEADER FILE IS AUTOMATICALLY GENERATED -- DO NOT EDIT

/**
 * Copyright (c) 2025 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _HARDWARE_STRUCTS_TRNG_H
#define _HARDWARE_STRUCTS_TRNG_H

/**
 * \file rp2350/trng.h
 */

#include "hardware/address_mapped.h"
#include "hardware/regs/trng.h"

// Reference to datasheet: https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf#tab-registerlist_trng
//
// The _REG_ macro is intended to help make the register navigable in your IDE (for example, using the "Go to Definition" feature)
// _REG_(x) will link to the corresponding register in hardware/regs/trng.h.
//
// Bit-field descriptions are of the form:
// BITMASK [BITRANGE] FIELDNAME (RESETVALUE) DESCRIPTION

typedef struct {
    _REG_(TRNG_RNG_IMR_OFFSET) // TRNG_RNG_IMR
    // Interrupt masking
    // 0x00000008 [3]     VN_ERR_INT_MASK (1) Set to 1 to mask (disable) this interrupt: no interrupt...
    // 0x00000004 [2]     CRNGT_ERR_INT_MASK (1) Set to 1 to mask (disable) this interrupt: no interrupt...
    // 0x00000002 [1]     AUTOCORR_ERR_INT_MASK (1) Set to 1 to mask (disable) this interrupt: no interrupt...
    // 0x00000001 [0]     EHR_VALID_INT_MASK (1) Set to 1 to mask (disable) this interrupt: no interrupt...
    io_rw_32 rng_imr;

    _REG_(TRNG_RNG_ISR_OFFSET) // TRNG_RNG_ISR
    // RNG status register
    // 0x00000008 [3]     VN_ERR       (0) 1 indicates von Neumann error
    // 0x00000004 [2]     CRNGT_ERR    (0) 1 indicates CRNGT in the RNG test failed
    // 0x00000002 [1]     AUTOCORR_ERR (0) 1 indicates Autocorrelation test failed four times in a row
    // 0x00000001 [0]     EHR_VALID    (0) 1 indicates that 192 bits have been collected in the...
    io_ro_32 rng_isr;

    _REG_(TRNG_RNG_ICR_OFFSET) // TRNG_RNG_ICR
    // Interrupt/status bit clear Register
    // 0x00000008 [3]     VN_ERR       (0) Write 1 to clear corresponding bit in RNG_ISR
    // 0x00000004 [2]     CRNGT_ERR    (0) Write 1 to clear corresponding bit in RNG_ISR
    // 0x00000002 [1]     AUTOCORR_ERR (0) Cannot be cleared by SW! Only RNG reset clears this bit
    // 0x00000001 [0]     EHR_VALID    (0) Write 1 - clear corresponding bit in RNG_ISR
    io_rw_32 rng_icr;

    _REG_(TRNG_TRNG_CONFIG_OFFSET) // TRNG_TRNG_CONFIG
    // Selecting the inverter-chain length
    // 0x00000003 [1:0]   RND_SRC_SEL  (0x0) Selects the number of inverters (out of four possible...
    io_rw_32 trng_config;

    _REG_(TRNG_TRNG_VALID_OFFSET) // TRNG_TRNG_VALID
    // 192 bit collection indication
    // 0x00000001 [0]     EHR_VALID    (0) 1 indicates that collection of bits in the RNG is...
    io_ro_32 trng_valid;

    // (Description copied from array index 0 register TRNG_EHR_DATA0 applies similarly to other array indexes)
    _REG_(TRNG_EHR_DATA0_OFFSET) // TRNG_EHR_DATA0
    // RNG collected bits
    // 0xffffffff [31:0]  EHR_DATA0    (0x00000000) Bits [31:0] of Entropy Holding Register (EHR) - RNG...
    io_ro_32 ehr_data[6];

    _REG_(TRNG_RND_SOURCE_ENABLE_OFFSET) // TRNG_RND_SOURCE_ENABLE
    // Enable signal for the random source
    // 0x00000001 [0]     RND_SRC_EN   (0) * 1 - entropy source is enabled
    io_rw_32 rnd_source_enable;

    _REG_(TRNG_SAMPLE_CNT1_OFFSET) // TRNG_SAMPLE_CNT1
    // Counts clocks between sampling of random bit
    // 0xffffffff [31:0]  SAMPLE_CNTR1 (0x0000ffff) Sets the number of rng_clk cycles between two...
    io_rw_32 sample_cnt1;

    _REG_(TRNG_AUTOCORR_STATISTIC_OFFSET) // TRNG_AUTOCORR_STATISTIC
    // Statistics about autocorrelation test activations
    // 0x003fc000 [21:14] AUTOCORR_FAILS (0x00) Count each time an autocorrelation test fails
    // 0x00003fff [13:0]  AUTOCORR_TRYS (0x0000) Count each time an autocorrelation test starts
    io_rw_32 autocorr_statistic;

    _REG_(TRNG_TRNG_DEBUG_CONTROL_OFFSET) // TRNG_TRNG_DEBUG_CONTROL
    // Debug register
    // 0x00000008 [3]     AUTO_CORRELATE_BYPASS (0) When set, the autocorrelation test in the TRNG module is bypassed
    // 0x00000004 [2]     TRNG_CRNGT_BYPASS (0) When set, the CRNGT test in the RNG is bypassed
    // 0x00000002 [1]     VNC_BYPASS   (0) When set, the Von-Neuman balancer is bypassed (including...
    io_rw_32 trng_debug_control;

    uint32_t _pad0;

    _REG_(TRNG_TRNG_SW_RESET_OFFSET) // TRNG_TRNG_SW_RESET
    // Generate internal SW reset within the RNG block
    // 0x00000001 [0]     TRNG_SW_RESET (0) Writing 1 to this register causes an internal RNG reset
    io_rw_32 trng_sw_reset;

    uint32_t _pad1[28];

    _REG_(TRNG_RNG_DEBUG_EN_INPUT_OFFSET) // TRNG_RNG_DEBUG_EN_INPUT
    // Enable the RNG debug mode
    // 0x00000001 [0]     RNG_DEBUG_EN (0) * 1 - debug mode is enabled
    io_rw_32 rng_debug_en_input;

    _REG_(TRNG_TRNG_BUSY_OFFSET) // TRNG_TRNG_BUSY
    // RNG Busy indication
    // 0x00000001 [0]     TRNG_BUSY    (0) Reflects rng_busy status
    io_ro_32 trng_busy;

    _REG_(TRNG_RST_BITS_COUNTER_OFFSET) // TRNG_RST_BITS_COUNTER
    // Reset the counter of collected bits in the RNG
    // 0x00000001 [0]     RST_BITS_COUNTER (0) Writing any value to this address will reset the bits...
    io_rw_32 rst_bits_counter;

    _REG_(TRNG_RNG_VERSION_OFFSET) // TRNG_RNG_VERSION
    // Displays the version settings of the TRNG
    // 0x00000080 [7]     RNG_USE_5_SBOXES (0) * 1 - 5 SBOX AES
    // 0x00000040 [6]     RESEEDING_EXISTS (0) * 1 - Exists
    // 0x00000020 [5]     KAT_EXISTS   (0) * 1 - Exists
    // 0x00000010 [4]     PRNG_EXISTS  (0) * 1 - Exists
    // 0x00000008 [3]     TRNG_TESTS_BYPASS_EN (0) * 1 - Exists
    // 0x00000004 [2]     AUTOCORR_EXISTS (0) * 1 - Exists
    // 0x00000002 [1]     CRNGT_EXISTS (0) * 1 - Exists
    // 0x00000001 [0]     EHR_WIDTH_192 (0) * 1 - 192-bit EHR
    io_ro_32 rng_version;

    uint32_t _pad2[7];

    // (Description copied from array index 0 register TRNG_RNG_BIST_CNTR_0 applies similarly to other array indexes)
    _REG_(TRNG_RNG_BIST_CNTR_0_OFFSET) // TRNG_RNG_BIST_CNTR_0
    // Collected BIST results
    // 0x003fffff [21:0]  ROSC_CNTR_VAL (0x000000) Reflects the results of RNG BIST counter
    io_ro_32 rng_bist_cntr[3];
} trng_hw_t;

#define trng_hw ((trng_hw_t *)(TRNG_BASE + TRNG_RNG_IMR_OFFSET))
static_assert(sizeof (trng_hw_t) == 0x00ec, "");

// provided as these older constants are used by the RP2350 bootrom, but there are now no "special" reserved bits
// this maintains the pre-existing property that TRNG_FOO_RESERVED_BITS ^ TRNG_FOO_BITS = actual valid TRNG_FOO bits
#define TRNG_TRNG_DEBUG_CONTROL_RESERVED_BITS 0
#define TRNG_RND_SOURCE_ENABLE_RESERVED_BITS 0
#define TRNG_RNG_ICR_RESERVED_BITS 0

#endif // _HARDWARE_STRUCTS_TRNG_H
