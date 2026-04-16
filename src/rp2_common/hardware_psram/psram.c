/*
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "hardware/psram.h"
#include <string.h>
#include "hardware/address_mapped.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/regs/addressmap.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "hardware/structs/qmi.h"
#include "hardware/structs/xip_ctrl.h"
#include "pico/runtime_init.h"

// PSRAM SPI command codes
const uint8_t PSRAM_CMD_QUAD_END = 0xF5;
const uint8_t PSRAM_CMD_READ_ID = 0x9F;
const uint8_t PSRAM_CMD_QUAD_ENABLE = 0x35;
const uint8_t PSRAM_CMD_QUAD_READ = 0xEB;
const uint8_t PSRAM_CMD_QUAD_WRITE = 0x38;
const uint8_t PSRAM_CMD_NOOP = 0xFF;


#if PICO_AUTO_DETECT_PSRAM
static size_t __no_inline_not_in_flash_func(detect_psram_size)(void) {
    int psram_size = 0;

    uint32_t flags = save_and_disable_interrupts();

    // Try and read the PSRAM ID via direct_csr.
    qmi_hw->direct_csr = 30 << QMI_DIRECT_CSR_CLKDIV_LSB | QMI_DIRECT_CSR_EN_BITS;

    // Need to poll for the cooldown on the last XIP transfer to expire
    // (via direct-mode BUSY flag) before it is safe to perform the first
    // direct-mode operation
    while ((qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS) != 0) {
        tight_loop_contents();
    }

    // Exit out of QMI in case we've inited already
    qmi_hw->direct_csr |= QMI_DIRECT_CSR_ASSERT_CS1N_BITS;

    // Transmit as quad.
    qmi_hw->direct_tx = PSRAM_CMD_QUAD_END | QMI_DIRECT_TX_OE_BITS | QMI_DIRECT_TX_IWIDTH_VALUE_Q << QMI_DIRECT_TX_IWIDTH_LSB;

    while ((qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS) != 0) {
        tight_loop_contents();
    }

    (void)qmi_hw->direct_rx;

    qmi_hw->direct_csr &= ~(QMI_DIRECT_CSR_ASSERT_CS1N_BITS);

    // Read the ID
    qmi_hw->direct_csr |= QMI_DIRECT_CSR_ASSERT_CS1N_BITS;
    uint8_t kgd = 0;    // Known Good Die - expects 0x5D on APS6404
    uint8_t eid = 0;

    for (size_t i = 0; i < 7; i++)
    {
        qmi_hw->direct_tx = i == 0 ? PSRAM_CMD_READ_ID : PSRAM_CMD_NOOP;

        while ((qmi_hw->direct_csr & QMI_DIRECT_CSR_TXEMPTY_BITS) == 0) {
            tight_loop_contents();
        }

        while ((qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS) != 0) {
            tight_loop_contents();
        }

        if (i == 5) {
            kgd = qmi_hw->direct_rx;
        } else if (i == 6) {
            eid = qmi_hw->direct_rx;
        } else {
            (void)qmi_hw->direct_rx;
        }
    }

    // Disable direct csr.
    qmi_hw->direct_csr = 0;

    restore_interrupts(flags);

    if (kgd == PICO_AUTO_DETECT_PSRAM_ID) {
        psram_size = 1024 * 1024; // 1 MiB
        uint8_t size_id = eid >> 5;
        if (eid == 0x26 || size_id == 2) {
            psram_size *= 8; // 8 MiB
        } else if (size_id == 0) {
            psram_size *= 2; // 2 MiB
        } else if (size_id == 1) {
            psram_size *= 4; // 4 MiB
        }
    }

    return psram_size;
}
#endif

#if PICO_SAVE_RESTORE_QMI_CS1
static void __no_inline_not_in_flash_func(reinitialise_psram)(void) {
    gpio_set_function(flash_devinfo_get_cs_gpio(1), GPIO_FUNC_XIP_CS1);

    // Enable direct mode, PSRAM CS, clkdiv of 10.
    qmi_hw->direct_csr = 10 << QMI_DIRECT_CSR_CLKDIV_LSB | \
        QMI_DIRECT_CSR_EN_BITS | \
        QMI_DIRECT_CSR_AUTO_CS1N_BITS;
    while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS) {
        tight_loop_contents();
    }

    // Enable quad mode
    qmi_hw->direct_tx = PSRAM_CMD_QUAD_ENABLE;
    while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS) {
        tight_loop_contents();
    }

    // Disable direct mode
    qmi_hw->direct_csr = 0;

    // Set PSRAM timing for APS6404
    //
    // Using an rxdelay equal to the divisor isn't enough when running the APS6404 close to 133MHz.
    // So: don't allow running at divisor 1 above 100MHz (because delay of 2 would be too late),
    // and add an extra 1 to the rxdelay if the divided clock is > 100MHz (i.e. sys clock > 200MHz).
    const int max_psram_freq = 133000000;
    const int clock_hz = clock_get_hz(clk_sys);
    int divisor = (clock_hz + max_psram_freq - 1) / max_psram_freq;
    if (divisor == 1 && clock_hz > 100000000) {
        divisor = 2;
    }
    int rxdelay = divisor;
    if (clock_hz / divisor > 100000000) {
        rxdelay += 1;
    }

    // - Max select must be <= 8us.  The value is given in multiples of 64 system clocks.
    // - Min deselect must be >= 18ns.  The value is given in system clock cycles - ceil(divisor / 2).
    const int clock_period_fs = 1000000000000000ll / clock_hz;
    const int max_select = (125 * 1000000) / clock_period_fs;  // 125 = 8000ns / 64
    const int min_deselect = (18 * 1000000 + (clock_period_fs - 1)) / clock_period_fs - (divisor + 1) / 2;

    qmi_hw->m[1].timing = 1 << QMI_M1_TIMING_COOLDOWN_LSB |
        QMI_M1_TIMING_PAGEBREAK_VALUE_1024 << QMI_M1_TIMING_PAGEBREAK_LSB |
        max_select << QMI_M1_TIMING_MAX_SELECT_LSB |
        min_deselect << QMI_M1_TIMING_MIN_DESELECT_LSB |
        rxdelay << QMI_M1_TIMING_RXDELAY_LSB |
        divisor << QMI_M1_TIMING_CLKDIV_LSB;

    // Set PSRAM commands and formats
    qmi_hw->m[1].rfmt = (QMI_M1_RFMT_PREFIX_WIDTH_VALUE_Q << QMI_M1_RFMT_PREFIX_WIDTH_LSB |
                        QMI_M1_RFMT_ADDR_WIDTH_VALUE_Q << QMI_M1_RFMT_ADDR_WIDTH_LSB |
                        QMI_M1_RFMT_SUFFIX_WIDTH_VALUE_Q << QMI_M1_RFMT_SUFFIX_WIDTH_LSB |
                        QMI_M1_RFMT_DUMMY_WIDTH_VALUE_Q << QMI_M1_RFMT_DUMMY_WIDTH_LSB |
                        QMI_M1_RFMT_DATA_WIDTH_VALUE_Q << QMI_M1_RFMT_DATA_WIDTH_LSB |
                        QMI_M1_RFMT_PREFIX_LEN_VALUE_8 << QMI_M1_RFMT_PREFIX_LEN_LSB |
                        QMI_M1_RFMT_DUMMY_LEN_VALUE_24 << QMI_M1_RFMT_DUMMY_LEN_LSB |
                        QMI_M1_RFMT_SUFFIX_LEN_VALUE_NONE << QMI_M1_RFMT_SUFFIX_LEN_LSB);

    qmi_hw->m[1].rcmd = PSRAM_CMD_QUAD_READ << QMI_M1_RCMD_PREFIX_LSB | 0 << QMI_M1_RCMD_SUFFIX_LSB;

    qmi_hw->m[1].wfmt = (QMI_M1_WFMT_PREFIX_WIDTH_VALUE_Q << QMI_M1_WFMT_PREFIX_WIDTH_LSB |
                        QMI_M1_WFMT_ADDR_WIDTH_VALUE_Q << QMI_M1_WFMT_ADDR_WIDTH_LSB |
                        QMI_M1_WFMT_SUFFIX_WIDTH_VALUE_Q << QMI_M1_WFMT_SUFFIX_WIDTH_LSB |
                        QMI_M1_WFMT_DUMMY_WIDTH_VALUE_Q << QMI_M1_WFMT_DUMMY_WIDTH_LSB |
                        QMI_M1_WFMT_DATA_WIDTH_VALUE_Q << QMI_M1_WFMT_DATA_WIDTH_LSB |
                        QMI_M1_WFMT_PREFIX_LEN_VALUE_8 << QMI_M1_WFMT_PREFIX_LEN_LSB |
                        QMI_M1_WFMT_DUMMY_LEN_VALUE_NONE << QMI_M1_WFMT_DUMMY_LEN_LSB |
                        QMI_M1_WFMT_SUFFIX_LEN_VALUE_NONE << QMI_M1_WFMT_SUFFIX_LEN_LSB);

    qmi_hw->m[1].wcmd = PSRAM_CMD_QUAD_WRITE << QMI_M1_WCMD_PREFIX_LSB | 0 << QMI_M1_WCMD_SUFFIX_LSB;
}
#endif

#if !PICO_RUNTIME_NO_INIT_PSRAM
void runtime_init_setup_psram(void) {
    // Setup flash_devinfo from compile definitions
    #ifdef PICO_PSRAM_CS_PIN
    flash_devinfo_set_cs_gpio(1, PICO_PSRAM_CS_PIN);
    #endif
    #ifdef PICO_PSRAM_SIZE_BYTES
    flash_devinfo_set_cs_size(1, flash_devinfo_bytes_to_size(PICO_PSRAM_SIZE_BYTES));
    #endif

    #if PICO_AUTO_DETECT_PSRAM
    #ifndef PICO_PSRAM_CS_PIN
    #error PICO_PSRAM_CS_PIN must be set to use PICO_AUTO_DETECT_PSRAM
    #endif
    if (flash_devinfo_get_cs_size(1) == FLASH_DEVINFO_SIZE_NONE) {
        // Attempt to auto-detect the PSRAM size
        gpio_set_function(PICO_PSRAM_CS_PIN, GPIO_FUNC_XIP_CS1);
        size_t psram_size = detect_psram_size();
        psram_present = psram_size > 0;
        // Set flash_devinfo size
        if (psram_present) flash_devinfo_set_cs_size(1, flash_devinfo_bytes_to_size(psram_size));
    }
    #endif

    if (flash_devinfo_get_cs_size(1) == FLASH_DEVINFO_SIZE_NONE) {
        // No PSRAM detected
        invalid_params_if(HARDWARE_PSRAM, true);
        return;
    }

    // Enable writes to PSRAM
    hw_set_bits(&xip_ctrl_hw->ctrl, XIP_CTRL_WRITABLE_M1_BITS);

    #if PICO_SAVE_RESTORE_QMI_CS1
    // Register the function to initialise the QMI CS1 configuration
    flash_set_qmi_cs1_setup_function(reinitialise_psram);
    #endif

    // Now flash_devinfo is correct, just start XIP
    flash_start_xip();

    // And load any initialised PSRAM data
    extern uint32_t __psram_load_source__;
    extern uint32_t __psram_load_start__;
    extern uint32_t __psram_load_end__;
    uint32_t stored_words = (uint32_t)(&__psram_load_end__ - &__psram_load_start__);
    if (stored_words > 0) {
        memcpy(&__psram_load_start__, &__psram_load_source__, stored_words * sizeof(uint32_t));
    }
}

#if defined(PICO_RUNTIME_INIT_PSRAM) && !PICO_RUNTIME_SKIP_INIT_PSRAM
PICO_RUNTIME_INIT_FUNC_RUNTIME(runtime_init_setup_psram, PICO_RUNTIME_INIT_PSRAM);
#endif
#endif
