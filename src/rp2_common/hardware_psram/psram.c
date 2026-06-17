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


size_t __weak psram_eid_to_size(uint8_t kgd, uint8_t eid) {
    // Weak function to allow overriding if other PSRAM chips
    // have different EID to size mapping
    // Currently supports APS6404 and ISSI PSRAM
    size_t psram_size = 0;

    if (kgd == PICO_DEFAULT_PSRAM_ID) { // Known Good Die - expects 0x5D
        psram_size = 1024 * 1024; // 1 MiB
        uint8_t size_id = eid >> 5;
        if (size_id == 4) { // == 4 is for ISSI PSRAM
            psram_size *= 16; // 16 MiB
        } else if (eid == 0x26 || size_id == 2 || size_id == 3) { // == 3 is for ISSI PSRAM
            psram_size *= 8; // 8 MiB
        } else if (size_id == 1) {
            psram_size *= 4; // 4 MiB
        } else if (size_id == 0) {
            psram_size *= 2; // 2 MiB
        }
    }

    return psram_size;
}


size_t psram_detect_size(void) {
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

    uint8_t kgd = rxbuffer[5];
    uint8_t eid = rxbuffer[6];

    return psram_eid_to_size(kgd, eid);
}

size_t psram_detect_cs_and_size(uint8_t *cs_gpios, size_t num) {
    gpio_function_t prev_funcs[num];
    size_t psram_size = 0;
    for (size_t i=0; i < num; i++) {
        // Save and clear all CS GPIO functions
        uint8_t gpio = cs_gpios[i];
        prev_funcs[i] = gpio_get_function(gpio);
        gpio_set_function(gpio, GPIO_FUNC_NULL);
    }
    for (size_t i=0; i < num; i++) {
        uint8_t gpio = cs_gpios[i];
        flash_devinfo_set_cs_gpio(1, gpio);
        gpio_set_function(gpio, GPIO_FUNC_XIP_CS1);
        psram_size = psram_detect_size();
        if (psram_size > 0) {
            // CS GPIO found, so will be left configured in flash_devinfo
            break;
        }
        gpio_set_function(gpio, GPIO_FUNC_NULL);
    }
    for (size_t i=0; i < num; i++) {
        // Restore previous function to all CS GPIOs
        uint8_t gpio = cs_gpios[i];
        gpio_set_function(gpio, prev_funcs[i]);
    }

    return psram_size;
}

#if PICO_AUTO_DETECT_PSRAM_CS_SKIP_DEFAULTS
// Preprocessor hackery to make this simpler and have fewer checks
#if PICO_RP2350
#if PICO_RP2350A
#define is_cs_gpio(gpio) (gpio == 0) || (gpio == 8) || (gpio == 19)
#else
#define is_cs_gpio(gpio) (gpio == 0) || (gpio == 8) || (gpio == 19) || (gpio == 47)
#endif
#else
// Default to assume all GPIOs are CS GPIOs
#define is_cs_gpio(gpio) true
#endif

static const uint8_t default_pin_cs_gpios[] = {
#if defined(PICO_DEFAULT_UART_TX_PIN) && is_cs_gpio(PICO_DEFAULT_UART_TX_PIN)
    PICO_DEFAULT_UART_TX_PIN,
#endif
#if defined(PICO_DEFAULT_UART_RX_PIN) && is_cs_gpio(PICO_DEFAULT_UART_RX_PIN)
    PICO_DEFAULT_UART_RX_PIN,
#endif
#if defined(PICO_DEFAULT_I2C_SDA_PIN) && is_cs_gpio(PICO_DEFAULT_I2C_SDA_PIN)
    PICO_DEFAULT_I2C_SDA_PIN,
#endif
#if defined(PICO_DEFAULT_I2C_SCL_PIN) && is_cs_gpio(PICO_DEFAULT_I2C_SCL_PIN)
    PICO_DEFAULT_I2C_SCL_PIN,
#endif
#if defined(PICO_DEFAULT_SPI_SCK_PIN) && is_cs_gpio(PICO_DEFAULT_SPI_SCK_PIN)
    PICO_DEFAULT_SPI_SCK_PIN,
#endif
#if defined(PICO_DEFAULT_SPI_TX_PIN) && is_cs_gpio(PICO_DEFAULT_SPI_TX_PIN)
    PICO_DEFAULT_SPI_TX_PIN,
#endif
#if defined(PICO_DEFAULT_SPI_RX_PIN) && is_cs_gpio(PICO_DEFAULT_SPI_RX_PIN)
    PICO_DEFAULT_SPI_RX_PIN,
#endif
#if defined(PICO_DEFAULT_SPI_CSN_PIN) && is_cs_gpio(PICO_DEFAULT_SPI_CSN_PIN)
    PICO_DEFAULT_SPI_CSN_PIN,
#endif
#if defined(PICO_DEFAULT_LED_PIN) && is_cs_gpio(PICO_DEFAULT_LED_PIN)
    PICO_DEFAULT_LED_PIN,
#endif
#if defined(PICO_DEFAULT_WS2812_PIN) && is_cs_gpio(PICO_DEFAULT_WS2812_PIN)
    PICO_DEFAULT_WS2812_PIN,
#endif
#if defined(PICO_DEFAULT_WS2812_POWER_PIN) && is_cs_gpio(PICO_DEFAULT_WS2812_POWER_PIN)
    PICO_DEFAULT_WS2812_POWER_PIN,
#endif
#if defined(CYW43_DEFAULT_PIN_WL_REG_ON) && is_cs_gpio(CYW43_DEFAULT_PIN_WL_REG_ON)
    CYW43_DEFAULT_PIN_WL_REG_ON,
#endif
#if defined(CYW43_DEFAULT_PIN_WL_DATA_OUT) && is_cs_gpio(CYW43_DEFAULT_PIN_WL_DATA_OUT)
    CYW43_DEFAULT_PIN_WL_DATA_OUT,
#endif
#if defined(CYW43_DEFAULT_PIN_WL_DATA_IN) && is_cs_gpio(CYW43_DEFAULT_PIN_WL_DATA_IN)
    CYW43_DEFAULT_PIN_WL_DATA_IN,
#endif
#if defined(CYW43_DEFAULT_PIN_WL_HOST_WAKE) && is_cs_gpio(CYW43_DEFAULT_PIN_WL_HOST_WAKE)
    CYW43_DEFAULT_PIN_WL_HOST_WAKE,
#endif
#if defined(CYW43_DEFAULT_PIN_WL_CLOCK) && is_cs_gpio(CYW43_DEFAULT_PIN_WL_CLOCK)
    CYW43_DEFAULT_PIN_WL_CLOCK,
#endif
#if defined(CYW43_DEFAULT_PIN_WL_CS) && is_cs_gpio(CYW43_DEFAULT_PIN_WL_CS)
    CYW43_DEFAULT_PIN_WL_CS,
#endif
#ifdef PICO_AUTO_DETECT_PSRAM_CS_SKIP_PINS
    PICO_AUTO_DETECT_PSRAM_CS_SKIP_PINS
#endif
};

static size_t remove_defaults_from_cs_gpios(uint8_t *cs_gpios, size_t num) {
    // To prevent trying to use a pin that is already defined for something
    // else by the board header
    size_t new_num = num;
    for (size_t i=0; i < new_num; i++) {
        for (size_t j=0; j < sizeof(default_pin_cs_gpios); j++) {
            if (cs_gpios[i] == default_pin_cs_gpios[j]) {
                // Replace with last valid GPIO in the array
                cs_gpios[i] = cs_gpios[new_num-1];
                new_num--;
                // Check the same index again, as it has a new value
                i--;
                // Exit the inner loop
                break;
            }
        }
    }
    return new_num;
}
#endif

static uint32_t psram_divisor = 0;
static uint32_t psram_rxdelay = 0;
static uint32_t psram_max_select = 0;
static uint32_t psram_min_deselect = 0;

int psram_configure_params(uint32_t max_psram_freq, uint32_t max_select_ns, uint32_t min_deselect_ns) {
    // Set PSRAM timing for APS6404
    //
    // Using an rxdelay equal to the divisor isn't enough when running the APS6404 close to 133MHz.
    // So: don't allow running at divisor 1 above 100MHz (because delay of 2 would be too late),
    // and add an extra 1 to the rxdelay if the divided clock is > 100MHz (i.e. sys clock > 200MHz).
    uint32_t clock_hz = clock_get_hz(clk_sys);
    uint32_t divisor = (clock_hz + max_psram_freq - 1) / max_psram_freq;
    if (divisor == 1 && clock_hz > 100000000) {
        divisor = 2;
    }
    uint32_t rxdelay = divisor;
    if (clock_hz / divisor > 100000000) {
        rxdelay += 1;
    }

    // - Max select is given in multiples of 64 system clocks.
    // - Min deselect is given in system clock cycles - ceil(divisor / 2).
    // Requires 64-bit maths as we're working with femtoseconds
    uint32_t clock_period_fs = 1000000000000000ull / clock_hz;  // 1s = 1000000000000000fs
    uint32_t max_select = ((uint64_t)max_select_ns * 1000000ull) / (64ull * (uint64_t)clock_period_fs);  // 1ns = 1000000fs
    uint32_t min_deselect = (min_deselect_ns * 1000000 + (clock_period_fs - 1)) / clock_period_fs - (divisor + 1) / 2;

    return psram_set_params(divisor, rxdelay, max_select, min_deselect);
}

int psram_set_params(uint32_t divisor, uint32_t rxdelay, uint32_t max_select, uint32_t min_deselect)
{
    invalid_params_if_and_return(HARDWARE_PSRAM, divisor & ~(QMI_M1_TIMING_CLKDIV_BITS >> QMI_M1_TIMING_CLKDIV_LSB), PICO_ERROR_INVALID_ARG);
    invalid_params_if_and_return(HARDWARE_PSRAM, rxdelay & ~(QMI_M1_TIMING_RXDELAY_BITS >> QMI_M1_TIMING_RXDELAY_LSB), PICO_ERROR_INVALID_ARG);
    invalid_params_if_and_return(HARDWARE_PSRAM, max_select & ~(QMI_M1_TIMING_MAX_SELECT_BITS >> QMI_M1_TIMING_MAX_SELECT_LSB), PICO_ERROR_INVALID_ARG);
    invalid_params_if_and_return(HARDWARE_PSRAM, min_deselect & ~(QMI_M1_TIMING_MIN_DESELECT_BITS >> QMI_M1_TIMING_MIN_DESELECT_LSB), PICO_ERROR_INVALID_ARG);
    // Provide method for explicitly setting the PSRAM parameters
    psram_divisor = divisor;
    psram_rxdelay = rxdelay;
    psram_max_select = max_select;
    psram_min_deselect = min_deselect;

    return PICO_OK;
}

static void __no_inline_not_in_flash_func(psram_initialize_internal)(void) {
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

static bool psram_initialized = false;

int psram_reinitialize(void) {
    // flash_devinfo must be configured correctly to use this function
    invalid_params_if_and_return(HARDWARE_PSRAM, flash_devinfo_get_cs_size(1) == FLASH_DEVINFO_SIZE_NONE, PICO_ERROR_PRECONDITION_NOT_MET);
    // psram_configure_params must have been called to set the parameters
    invalid_params_if_and_return(HARDWARE_PSRAM, psram_divisor == 0, PICO_ERROR_PRECONDITION_NOT_MET);

#if PICO_RP2350_A2_SUPPORTED
    // Workaround for RP2350-E14, where the bootrom only does this for GPIO 0 instead of the correct CS pin
    hw_clear_bits(&pads_bank0_hw->io[flash_devinfo_get_cs_gpio(1)], PADS_BANK0_GPIO0_ISO_BITS);
#endif

    // Register the function to initialize the QMI CS1 configuration
    flash_set_qmi_cs1_setup_function(psram_initialize_internal);

    // Call flash_start_xip, which calls psram_initialize_internal
    flash_start_xip();

    psram_initialized = true;

    return PICO_OK;
}

bool psram_is_available(void) {
    return psram_initialized;
}

size_t psram_get_size(void) {
    if (!psram_initialized) {
        return 0;
    }
    return flash_devinfo_size_to_bytes(flash_devinfo_get_cs_size(1));
}

bool psram_check_address(void* addr) {
    uint32_t offset = (uint32_t)addr - (XIP_BASE + flash_devinfo_size_to_bytes(FLASH_DEVINFO_SIZE_MAX));
    if (offset < psram_get_size()) {
        return true;
    } else {
        return false;
    }
}

#if !PICO_RUNTIME_NO_INIT_PSRAM
#if PICO_AUTO_DETECT_PSRAM_CS
#if PICO_RP2350
#if PICO_RP2350A
#define PICO_AVAILABLE_CS1_GPIOS {0, 8, 19}
#else
#define PICO_AVAILABLE_CS1_GPIOS {0, 8, 19, 47}
#endif
#else
#error "PICO_AVAILABLE_CS1_GPIOS must be defined for this platform to use PICO_AUTO_DETECT_PSRAM_CS"
#endif
#endif

void runtime_init_setup_psram(void) {
    // Check if flash_devinfo is already configured by OTP (or an earlier runtime_init function, e.g. in kitchen_sink_psram_tiny)
    bool __unused flash_devinfo_not_configured = flash_devinfo_get_cs_size(1) == FLASH_DEVINFO_SIZE_NONE;

    // Setup flash_devinfo from compile definitions
    #if defined(PICO_PSRAM_CS_PIN) && !PICO_AUTO_DETECT_PSRAM_CS
    if (flash_devinfo_not_configured) {
        flash_devinfo_set_cs_gpio(1, PICO_PSRAM_CS_PIN);
    }
    #endif
    #if defined(PICO_PSRAM_SIZE_BYTES) && !PICO_AUTO_DETECT_PSRAM_SIZE
    if (flash_devinfo_not_configured) {
        flash_devinfo_set_cs_size(1, flash_devinfo_bytes_to_size(PICO_PSRAM_SIZE_BYTES));
    }
    #endif

    // Setup flash_devinfo with auto-detected size
    #if PICO_AUTO_DETECT_PSRAM_SIZE
    #if !defined(PICO_PSRAM_CS_PIN) && !PICO_AUTO_DETECT_PSRAM_CS
    #error PICO_AUTO_DETECT_PSRAM_SIZE requires a specified PICO_PSRAM_CS_PIN or PICO_AUTO_DETECT_PSRAM_CS
    #elif defined(PICO_PSRAM_CS_PIN)
    gpio_set_function(PICO_PSRAM_CS_PIN, GPIO_FUNC_XIP_CS1);
    #endif
    if (flash_devinfo_not_configured) {
        // Attempt to auto-detect the PSRAM size
        #if PICO_AUTO_DETECT_PSRAM_CS
        uint8_t cs_gpios[] = PICO_AVAILABLE_CS1_GPIOS;
        size_t num_cs_gpios = count_of(cs_gpios);
        #if PICO_AUTO_DETECT_PSRAM_CS_SKIP_DEFAULTS
        num_cs_gpios = remove_defaults_from_cs_gpios(cs_gpios, num_cs_gpios);
        #endif
        size_t psram_size = psram_detect_cs_and_size(cs_gpios, num_cs_gpios);
        #else
        size_t psram_size = psram_detect_size();
        #endif
        // Set flash_devinfo size
        if (psram_size > 0) flash_devinfo_set_cs_size(1, flash_devinfo_bytes_to_size(psram_size));
    }
    #endif

    // The size has now been configured, so can cache the value
    flash_devinfo_size_t psram_flash_devinfo_size = flash_devinfo_get_cs_size(1);
    size_t psram_word_size = flash_devinfo_size_to_bytes(psram_flash_devinfo_size) >> 2; // >>2 is /4, for words
    psram_initialized = psram_flash_devinfo_size != FLASH_DEVINFO_SIZE_NONE;

    static_assert(FLASH_DEVINFO_SIZE_MAX == FLASH_DEVINFO_SIZE_16M, "expected max region size of 16M");
    extern uint32_t __psram_start__;
    extern uint32_t __psram_end__;
    uint32_t psram_words = ((uintptr_t)&__psram_end__ - (uintptr_t)&__psram_start__) / sizeof(uint32_t);
    if (psram_words > psram_word_size) {
        // Setup to bus fault for variables that don't fit in available PSRAM
        int clear_regions = 0; // Clear no regions by default
        if (psram_flash_devinfo_size == FLASH_DEVINFO_SIZE_NONE) {
            clear_regions = 4; // Clear all PSRAM regions
        } else if (psram_flash_devinfo_size < FLASH_DEVINFO_SIZE_4M) {
            clear_regions = 3; // Clear last 3x 4M regions
            // And reduce size of first PSRAM region
            qmi_hw->atrans[4] = (1u << psram_flash_devinfo_size) << QMI_ATRANS4_SIZE_LSB; // units are 4 kiB
        } else if (psram_flash_devinfo_size == FLASH_DEVINFO_SIZE_4M) {
            clear_regions = 3; // Clear last 3x 4M regions
        } else if (psram_flash_devinfo_size == FLASH_DEVINFO_SIZE_8M) {
            clear_regions = 2; // Clear last 2x 4M regions
        }

        for (int i = 8 - clear_regions; i < 8; i++) {
            qmi_hw->atrans[i] = 0;
        }
    }

    if (!psram_initialized) {
        return;
    }

    // Configure the PSRAM parameters with the default values
    int ret = psram_configure_params(PICO_DEFAULT_PSRAM_MAX_FREQ, PICO_DEFAULT_PSRAM_MAX_SELECT, PICO_DEFAULT_PSRAM_MIN_DESELECT);
    if (ret != PICO_OK) {
        psram_initialized = false;
        return;
    }

    // Initialize the PSRAM
    ret = psram_reinitialize();
    if (ret != PICO_OK) {
        psram_initialized = false;
        return;
    }

    // And load any initialized PSRAM data
    extern uint32_t __psram_load_source__;
    extern uint32_t __psram_load_start__;
    extern uint32_t __psram_load_end__;
    uint32_t stored_words = (uint32_t)(&__psram_load_end__ - &__psram_load_start__);
    if (stored_words > 0) {
        if (stored_words > psram_word_size) {
            // Only copy into available PSRAM, to avoid triggering bus faults here,
            // they will be triggered later when the variable is accessed
            stored_words = psram_word_size;
        }
        memcpy(&__psram_load_start__, &__psram_load_source__, stored_words * sizeof(uint32_t));
    }
}

#if defined(PICO_RUNTIME_INIT_PSRAM) && !PICO_RUNTIME_SKIP_INIT_PSRAM
PICO_RUNTIME_INIT_FUNC_RUNTIME(runtime_init_setup_psram, PICO_RUNTIME_INIT_PSRAM);
#endif
#endif
