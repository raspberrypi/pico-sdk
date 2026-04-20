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
#include "hardware/flash.h"
#include "hardware/structs/qmi.h"
#include "hardware/structs/xip_ctrl.h"
#include "pico/runtime_init.h"

// PSRAM SPI command codes
#define PSRAM_READ_ID_CMD 0x9F
#define PSRAM_QUAD_ENABLE_CMD 0x35
#define PSRAM_QUAD_READ_CMD 0xEB
#define PSRAM_QUAD_WRITE_CMD 0x38
#define PSRAM_NOOP_CMD 0xFF


size_t psram_detect_size(void) {
    int psram_size = 0;

    // Save size to restore later
    flash_devinfo_size_t prev_size = flash_devinfo_get_cs_size(1);
    // Setup with non-zero size, so the bootrom will issue the XIP exit sequence to CS1
    // The GPIO doesn't matter, as the caller must set the correct function for the CS pin
    flash_devinfo_set_cs_size(1, FLASH_DEVINFO_SIZE_8K);


    // Read ID command, followed by 7 NOOP commands to get the response
    uint8_t txbuffer[8] = { PSRAM_READ_ID_CMD, PSRAM_NOOP_CMD, PSRAM_NOOP_CMD, PSRAM_NOOP_CMD, PSRAM_NOOP_CMD, PSRAM_NOOP_CMD, PSRAM_NOOP_CMD, PSRAM_NOOP_CMD };
    uint8_t rxbuffer[8] = { 0 };
    flash_do_cmd_cs(txbuffer, rxbuffer, sizeof(txbuffer), 1);

    // Restore previous size
    flash_devinfo_set_cs_size(1, prev_size);

    uint8_t kgd = rxbuffer[5];  // Known Good Die - expects 0x5D on APS6404
    uint8_t eid = rxbuffer[6];

    if (kgd == PICO_DEFAULT_PSRAM_ID) {
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

size_t psram_detect_cs_and_size(uint8_t *cs_gpios, size_t num) {
    gpio_function_t prev_funcs[4] = {};
    for (size_t i=0; i < num; i++) {
        // Save and clear all CS GPIO functions
        uint8_t gpio = cs_gpios[i];
        prev_funcs[i] = gpio_get_function(gpio);
        gpio_set_function(gpio, GPIO_FUNC_NULL);
    }
    for (size_t i=0; i < num; i++) {
        uint8_t gpio = cs_gpios[i];
        flash_devinfo_set_cs_gpio(1, gpio);
    #if PICO_RP2350_A2_SUPPORTED
        // Workaround for RP2350-E14, where the bootrom only does this for GPIO 0 instead of the correct CS pin
        hw_clear_bits(&pads_bank0_hw->io[gpio], PADS_BANK0_GPIO0_ISO_BITS);
    #endif
        size_t psram_size = psram_detect_size();
        if (psram_size > 0) {
            // CS GPIO found
            return psram_size;
        }
        // Restore previous function to GPIO
        gpio_set_function(gpio, prev_funcs[i]);
    }

    return 0;
}

static size_t remove_defaults_from_cs_gpios(uint8_t *cs_gpios, size_t num) {
    // To prevent trying to use a pin that is already defined for something
    // else by the board header
    size_t new_num = num;
    for (size_t i=0; i < new_num; i++) {
        if (
        #ifdef PICO_DEFAULT_UART_TX_PIN
            cs_gpios[i] == PICO_DEFAULT_UART_TX_PIN ||
        #endif
        #ifdef PICO_DEFAULT_UART_RX_PIN
            cs_gpios[i] == PICO_DEFAULT_UART_RX_PIN ||
        #endif
        #ifdef PICO_DEFAULT_I2C_SDA_PIN
            cs_gpios[i] == PICO_DEFAULT_I2C_SDA_PIN ||
        #endif
        #ifdef PICO_DEFAULT_I2C_SCL_PIN
            cs_gpios[i] == PICO_DEFAULT_I2C_SCL_PIN ||
        #endif
        #ifdef PICO_DEFAULT_SPI_SCK_PIN
            cs_gpios[i] == PICO_DEFAULT_SPI_SCK_PIN ||
        #endif
        #ifdef PICO_DEFAULT_SPI_TX_PIN
            cs_gpios[i] == PICO_DEFAULT_SPI_TX_PIN ||
        #endif
        #ifdef PICO_DEFAULT_SPI_RX_PIN
            cs_gpios[i] == PICO_DEFAULT_SPI_RX_PIN ||
        #endif
        #ifdef PICO_DEFAULT_SPI_CSN_PIN
            cs_gpios[i] == PICO_DEFAULT_SPI_CSN_PIN ||
        #endif
        #ifdef PICO_DEFAULT_LED_PIN
            cs_gpios[i] == PICO_DEFAULT_LED_PIN ||
        #endif
        #ifdef PICO_DEFAULT_WS2812_PIN
            cs_gpios[i] == PICO_DEFAULT_WS2812_PIN ||
        #endif
        #ifdef PICO_DEFAULT_WS2812_POWER_PIN
            cs_gpios[i] == PICO_DEFAULT_WS2812_POWER_PIN ||
        #endif
        #ifdef CYW43_DEFAULT_PIN_WL_REG_ON
            cs_gpios[i] == CYW43_DEFAULT_PIN_WL_REG_ON ||
        #endif
        #ifdef CYW43_DEFAULT_PIN_WL_DATA_OUT
            cs_gpios[i] == CYW43_DEFAULT_PIN_WL_DATA_OUT ||
        #endif
        #ifdef CYW43_DEFAULT_PIN_WL_DATA_IN
            cs_gpios[i] == CYW43_DEFAULT_PIN_WL_DATA_IN ||
        #endif
        #ifdef CYW43_DEFAULT_PIN_WL_HOST_WAKE
            cs_gpios[i] == CYW43_DEFAULT_PIN_WL_HOST_WAKE ||
        #endif
        #ifdef CYW43_DEFAULT_PIN_WL_CLOCK
            cs_gpios[i] == CYW43_DEFAULT_PIN_WL_CLOCK ||
        #endif
        #ifdef CYW43_DEFAULT_PIN_WL_CS
            cs_gpios[i] == CYW43_DEFAULT_PIN_WL_CS ||
        #endif
            false // so the || works
        ) {
            // Replace with last valid GPIO in the array
            cs_gpios[i] = cs_gpios[new_num-1];
            new_num--;
            // Check the same index again, as it has a new value
            i--;
        }
    }
    return new_num;
}

static uint32_t psram_divisor = 0;
static uint32_t psram_rxdelay = 0;
static uint32_t psram_max_select = 0;
static uint32_t psram_min_deselect = 0;

void psram_configure_params(uint32_t max_psram_freq, uint32_t max_select_ns, uint32_t min_deselect_ns) {
    // Set PSRAM timing for APS6404
    //
    // Using an rxdelay equal to the divisor isn't enough when running the APS6404 close to 133MHz.
    // So: don't allow running at divisor 1 above 100MHz (because delay of 2 would be too late),
    // and add an extra 1 to the rxdelay if the divided clock is > 100MHz (i.e. sys clock > 200MHz).
    uint32_t clock_hz = clock_get_hz(clk_sys);
    psram_divisor = (clock_hz + max_psram_freq - 1) / max_psram_freq;
    if (psram_divisor == 1 && clock_hz > 100000000) {
        psram_divisor = 2;
    }
    psram_rxdelay = psram_divisor;
    if (clock_hz / psram_divisor > 100000000) {
        psram_rxdelay += 1;
    }

    // - Max select is given in multiples of 64 system clocks.
    // - Min deselect is given in system clock cycles - ceil(divisor / 2).
    // Requires 64-bit maths as we're working with femtoseconds
    uint32_t clock_period_fs = 1000000000000000ull / clock_hz;  // 1s = 1000000000000000fs
    psram_max_select = ((uint64_t)max_select_ns * 1000000ull) / (64ull * (uint64_t)clock_period_fs);  // 1ns = 1000000fs
    psram_min_deselect = (min_deselect_ns * 1000000 + (clock_period_fs - 1)) / clock_period_fs - (psram_divisor + 1) / 2;

#if PICO_RP2350_A2_SUPPORTED
    // Workaround for RP2350-E14, where the bootrom only does this for GPIO 0 instead of the correct CS pin
    hw_clear_bits(&pads_bank0_hw->io[flash_devinfo_get_cs_gpio(1)], PADS_BANK0_GPIO0_ISO_BITS);
#endif
}

static void __no_inline_not_in_flash_func(psram_initialise_internal)(void) {
    // Send QUAD_ENABLE command manually, as using flash_do_cmd_cs would call this function again,
    // if it has been registered using flash_set_qmi_cs1_setup_function
    // Enable direct mode, auto CS1, clkdiv of 10
    qmi_hw->direct_csr = 10 << QMI_DIRECT_CSR_CLKDIV_LSB | \
        QMI_DIRECT_CSR_EN_BITS | \
        QMI_DIRECT_CSR_AUTO_CS1N_BITS;
    while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS) {
        tight_loop_contents();
    }
    // Send QUAD_ENABLE command
    qmi_hw->direct_tx = PSRAM_QUAD_ENABLE_CMD;
    while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS) {
        tight_loop_contents();
    }
    // Disable direct mode
    qmi_hw->direct_csr = 0;

    qmi_hw->m[1].timing = 1 << QMI_M1_TIMING_COOLDOWN_LSB |
        QMI_M1_TIMING_PAGEBREAK_VALUE_1024 << QMI_M1_TIMING_PAGEBREAK_LSB | // Crossing page boundary would need lower clock speed
        psram_max_select << QMI_M1_TIMING_MAX_SELECT_LSB |
        psram_min_deselect << QMI_M1_TIMING_MIN_DESELECT_LSB |
        psram_rxdelay << QMI_M1_TIMING_RXDELAY_LSB |
        psram_divisor << QMI_M1_TIMING_CLKDIV_LSB;

    // Read is all quad, with prefix of PSRAM_CMD_QUAD_READ, 6 wait cycles (so 6*4=24 dummy bits), no suffix
    qmi_hw->m[1].rfmt = (QMI_M1_RFMT_PREFIX_WIDTH_VALUE_Q << QMI_M1_RFMT_PREFIX_WIDTH_LSB |
                        QMI_M1_RFMT_ADDR_WIDTH_VALUE_Q << QMI_M1_RFMT_ADDR_WIDTH_LSB |
                        QMI_M1_RFMT_SUFFIX_WIDTH_VALUE_Q << QMI_M1_RFMT_SUFFIX_WIDTH_LSB |
                        QMI_M1_RFMT_DUMMY_WIDTH_VALUE_Q << QMI_M1_RFMT_DUMMY_WIDTH_LSB |
                        QMI_M1_RFMT_DATA_WIDTH_VALUE_Q << QMI_M1_RFMT_DATA_WIDTH_LSB |
                        QMI_M1_RFMT_PREFIX_LEN_VALUE_8 << QMI_M1_RFMT_PREFIX_LEN_LSB |
                        QMI_M1_RFMT_DUMMY_LEN_VALUE_24 << QMI_M1_RFMT_DUMMY_LEN_LSB |
                        QMI_M1_RFMT_SUFFIX_LEN_VALUE_NONE << QMI_M1_RFMT_SUFFIX_LEN_LSB);
    qmi_hw->m[1].rcmd = PSRAM_QUAD_READ_CMD << QMI_M1_RCMD_PREFIX_LSB;

    // Write is all quad, with prefix of PSRAM_CMD_QUAD_WRITE, no dummy, no suffix
    qmi_hw->m[1].wfmt = (QMI_M1_WFMT_PREFIX_WIDTH_VALUE_Q << QMI_M1_WFMT_PREFIX_WIDTH_LSB |
                        QMI_M1_WFMT_ADDR_WIDTH_VALUE_Q << QMI_M1_WFMT_ADDR_WIDTH_LSB |
                        QMI_M1_WFMT_SUFFIX_WIDTH_VALUE_Q << QMI_M1_WFMT_SUFFIX_WIDTH_LSB |
                        QMI_M1_WFMT_DUMMY_WIDTH_VALUE_Q << QMI_M1_WFMT_DUMMY_WIDTH_LSB |
                        QMI_M1_WFMT_DATA_WIDTH_VALUE_Q << QMI_M1_WFMT_DATA_WIDTH_LSB |
                        QMI_M1_WFMT_PREFIX_LEN_VALUE_8 << QMI_M1_WFMT_PREFIX_LEN_LSB |
                        QMI_M1_WFMT_DUMMY_LEN_VALUE_NONE << QMI_M1_WFMT_DUMMY_LEN_LSB |
                        QMI_M1_WFMT_SUFFIX_LEN_VALUE_NONE << QMI_M1_WFMT_SUFFIX_LEN_LSB);
    qmi_hw->m[1].wcmd = PSRAM_QUAD_WRITE_CMD << QMI_M1_WCMD_PREFIX_LSB;

    // Enable writes to PSRAM
    hw_set_bits(&xip_ctrl_hw->ctrl, XIP_CTRL_WRITABLE_M1_BITS);
}

static bool has_psram = false;

void psram_reinitialise(void) {
    // flash_devinfo must be configured correctly to use this function
    invalid_params_if(HARDWARE_PSRAM, flash_devinfo_get_cs_size(1) == FLASH_DEVINFO_SIZE_NONE);
    // psram_configure_params must have been called to set the parameters
    invalid_params_if(HARDWARE_PSRAM, psram_divisor == 0);

    // Register the function to initialise the QMI CS1 configuration
    flash_set_qmi_cs1_setup_function(psram_initialise_internal);

    // Call flash_start_xip, which calls psram_initialise_internal
    flash_start_xip();

    has_psram = true;
}

bool psram_is_available(void) {
    return has_psram;
}

size_t psram_get_size(void) {
    if (!has_psram) {
        return 0;
    }
    return flash_devinfo_size_to_bytes(flash_devinfo_get_cs_size(1));
}

#if !PICO_RUNTIME_NO_INIT_PSRAM
void runtime_init_setup_psram(void) {
    // Setup flash_devinfo from compile definitions
    #if defined(PICO_PSRAM_SIZE_BYTES) && !PICO_AUTO_DETECT_PSRAM_SIZE
    flash_devinfo_set_cs_size(1, flash_devinfo_bytes_to_size(PICO_PSRAM_SIZE_BYTES));
    #endif
    #if defined(PICO_PSRAM_CS_PIN) && !PICO_AUTO_DETECT_PSRAM_CS
    flash_devinfo_set_cs_gpio(1, PICO_PSRAM_CS_PIN);
    #endif

    #if PICO_AUTO_DETECT_PSRAM_SIZE
    #if !defined(PICO_PSRAM_CS_PIN) && !PICO_AUTO_DETECT_PSRAM_CS
    #error PICO_AUTO_DETECT_PSRAM_SIZE requires a specified PICO_PSRAM_CS_PIN or PICO_AUTO_DETECT_PSRAM_CS
    #elif defined(PICO_PSRAM_CS_PIN)
    gpio_set_function(PICO_PSRAM_CS_PIN, GPIO_FUNC_XIP_CS1);
    #endif
    if (flash_devinfo_get_cs_size(1) == FLASH_DEVINFO_SIZE_NONE) {  // Check if size is already set by OTP
        // Attempt to auto-detect the PSRAM size
        #if PICO_AUTO_DETECT_PSRAM_CS
        uint8_t cs_gpios[] = PICO_AVAILABLE_CS1_GPIOS;
        size_t num_cs_gpios = sizeof(cs_gpios);
        #if PICO_AUTO_DETECT_PSRAM_CS_SKIP_DEFAULTS
        num_cs_gpios = remove_defaults_from_cs_gpios(cs_gpios, sizeof(cs_gpios));
        #endif
        size_t psram_size = psram_detect_cs_and_size(cs_gpios, num_cs_gpios);
        #else
        size_t psram_size = psram_detect_size();
        #endif
        // Set flash_devinfo size
        if (psram_size > 0) flash_devinfo_set_cs_size(1, flash_devinfo_bytes_to_size(psram_size));
    }
    #endif

    if (flash_devinfo_get_cs_size(1) == FLASH_DEVINFO_SIZE_NONE) {
        // No PSRAM present, so setup PSRAM to bus fault if there are variables in it
        extern uint32_t __psram_start__;
        extern uint32_t __psram_end__;
        uint32_t psram_words = (uint32_t)(&__psram_end__ - &__psram_start__);
        if (psram_words > 0) {
            // Clear ATRANS to trigger bus faults on PSRAM accesses
            for (int i=4; i < 8; i++) {
                qmi_hw->atrans[i] = 0;
            }
        }
        has_psram = false;
        return;
    } else{
        has_psram = true;
    }

    // Configure the PSRAM parameters with the default values
    psram_configure_params(PICO_DEFAULT_PSRAM_MAX_FREQ, PICO_DEFAULT_PSRAM_MAX_SELECT, PICO_DEFAULT_PSRAM_MIN_DESELECT);

    // Register the function to initialise the QMI CS1 configuration
    flash_set_qmi_cs1_setup_function(psram_initialise_internal);

    // Now flash_devinfo is correct, just start XIP, which calls psram_initialise afterwards
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
