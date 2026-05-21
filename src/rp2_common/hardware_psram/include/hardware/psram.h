/*
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _HARDWARE_PSRAM_H
#define _HARDWARE_PSRAM_H

#include "pico.h"
#include "hardware/clocks.h"
#include "hardware/flash.h"

/** \file psram.h
 *  \defgroup hardware_psram hardware_psram
 *
 * \brief Low level PSRAM setup functions
 *
 * When using the runtime_init initialization, this can initialize PSRAM in 3 ways,
 * listed from highest to lowest priority:
 *   1. If flash_devinfo is setup (e.g. configured in OTP), it will initialize PSRAM with
 *      the flash_devinfo size and GPIO for CS1
 *   2. If `PICO_AUTO_DETECT_PSRAM` is set it will attempt to detect PSRAM size and CS GPIO
 *      on CS1. This will attempt to use all available QMI CS1n GPIOs as chip selects, so they
 *      will be wiggled. By default, it will skip over some which are defined in the board
 *      header (see `PICO_AUTO_DETECT_PSRAM_CS_SKIP_DEFAULTS`).
 *     - If the CS GPIO is known and set in `PICO_PSRAM_CS_PIN`, you can just enable
 *       `PICO_AUTO_DETECT_PSRAM_SIZE` to only detect the size. Some board headers use
 *       this behavior if they have variants both with and without PSRAM fitted
 *       (e.g. adafruit_feather_rp2350)
 *   3. If `PICO_PSRAM_SIZE_BYTES` and `PICO_PSRAM_CS_PIN` are set (e.g. configured in the
 *      board header, or with \ref pico_override_psram_size) it will initialize PSRAM with
 *      that size and CS GPIO
 * 
 * Only the `PICO_AUTO_DETECT_PSRAM` methods (including `PICO_AUTO_DETECT_PSRAM_SIZE`) will
 * verify that PSRAM is present before using it.
 * 
 * Variables can be placed in PSRAM using __in_psram or __uninitialized_psram macros, and
 * you can also read/write the memory addresses directly.
 * 
 * If there are variables placed in PSRAM, XIP will be set up to cause bus faults on any
 * access to PSRAM addresses greater than the size available. The \ref psram_check_address
 * function should be used before accessing variables in PSRAM when auto-detection is on,
 * to prevent these bus faults.
 *
 * Note some of these functions are *unsafe* if you are using both cores, and the other
 * is executing from flash or psram concurrently with the operation. In this case, you
 * must perform your own synchronization to make sure that no XIP accesses take
 * place while running these functions. One option is to use the
 * \ref flash_safe_execute functions in \ref pico_flash.
 *
 * Likewise they are *unsafe* if you have interrupt handlers or an interrupt
 * vector table in flash or psram, so you must disable interrupts before calling in
 * this case - \ref flash_safe_execute handles this case too.
 *
 * The unsafe functions are:
 * - \ref psram_reinitialize
 * - \ref psram_detect_cs_and_size
 * - \ref psram_detect_size
 */

// PICO_CONFIG: PARAM_ASSERTIONS_ENABLED_HARDWARE_PSRAM, Enable/disable assertions in the hardware_psram module, type=bool, default=0, group=hardware_psram
#ifndef PARAM_ASSERTIONS_ENABLED_HARDWARE_PSRAM
#define PARAM_ASSERTIONS_ENABLED_HARDWARE_PSRAM 0
#endif

// PICO_CONFIG: PICO_PSRAM_SIZE_BYTES, size of psram in bytes, type=int, default=Usually provided via board header if psram is present, group=hardware_psram
// PICO_CONFIG: PICO_PSRAM_CS_PIN, chip select pin for psram, type=int, default=Usually provided via board header if psram is present, group=hardware_psram

// PICO_CONFIG: PICO_AUTO_DETECT_PSRAM, automatically detect if psram is present, type=bool, default=0, group=hardware_psram
#ifndef PICO_AUTO_DETECT_PSRAM
#define PICO_AUTO_DETECT_PSRAM 0
#endif

// PICO_CONFIG: PICO_AUTO_DETECT_PSRAM_SIZE, automatically detect psram size, type=bool, default=PICO_AUTO_DETECT_PSRAM, group=hardware_psram
#ifndef PICO_AUTO_DETECT_PSRAM_SIZE
#define PICO_AUTO_DETECT_PSRAM_SIZE PICO_AUTO_DETECT_PSRAM
#endif

// PICO_CONFIG: PICO_AUTO_DETECT_PSRAM_CS, automatically detect psram chip select pin, type=bool, default=PICO_AUTO_DETECT_PSRAM, group=hardware_psram
#ifndef PICO_AUTO_DETECT_PSRAM_CS
#define PICO_AUTO_DETECT_PSRAM_CS PICO_AUTO_DETECT_PSRAM
#elif PICO_AUTO_DETECT_PSRAM_CS && !PICO_AUTO_DETECT_PSRAM_SIZE
#error "PICO_AUTO_DETECT_PSRAM_SIZE must be set to use PICO_AUTO_DETECT_PSRAM_CS"
#endif

// PICO_CONFIG: PICO_AUTO_DETECT_PSRAM_CS_SKIP_DEFAULTS, skip any _DEFAULT_ GPIOs defined in the board header when auto-detecting psram chip select pin, type=bool, default=PICO_AUTO_DETECT_PSRAM_CS, group=hardware_psram
// PICO_CONFIG: PICO_AUTO_DETECT_PSRAM_CS_SKIP_PINS, comma separated list of extra pins to not check when auto-detecting psram chip select pin, type=list, group=hardware_psram
#ifndef PICO_AUTO_DETECT_PSRAM_CS_SKIP_DEFAULTS
#define PICO_AUTO_DETECT_PSRAM_CS_SKIP_DEFAULTS PICO_AUTO_DETECT_PSRAM_CS
#endif

// PICO_CONFIG: PICO_DEFAULT_PSRAM_ID, Default ID of psram used for auto-detection, type=int, default=0x5D, group=hardware_psram
#ifndef PICO_DEFAULT_PSRAM_ID
#define PICO_DEFAULT_PSRAM_ID 0x5D
#endif

// PICO_CONFIG: PICO_DEFAULT_PSRAM_MAX_FREQ, Default max frequency of psram, type=int, default=133 * MHZ, group=hardware_psram
#ifndef PICO_DEFAULT_PSRAM_MAX_FREQ
#define PICO_DEFAULT_PSRAM_MAX_FREQ (133 * MHZ)
#endif

// PICO_CONFIG: PICO_DEFAULT_PSRAM_MAX_SELECT, Default max select time of psram in ns, type=int, default=8000, group=hardware_psram
#ifndef PICO_DEFAULT_PSRAM_MAX_SELECT
#define PICO_DEFAULT_PSRAM_MAX_SELECT 8000
#endif

// PICO_CONFIG: PICO_DEFAULT_PSRAM_MIN_DESELECT, Default min deselect time of psram in ns, type=int, default=18, group=hardware_psram
#ifndef PICO_DEFAULT_PSRAM_MIN_DESELECT
#define PICO_DEFAULT_PSRAM_MIN_DESELECT 18
#endif


// PICO_CONFIG: PICO_RUNTIME_SKIP_INIT_PSRAM, Skip calling of `runtime_init_setup_psram` function during runtime init, type=bool, default=0, group=pico_runtime_init
// PICO_CONFIG: PICO_RUNTIME_NO_INIT_PSRAM, Do not include SDK implementation of `runtime_init_setup_psram` function, type=bool, default=0, group=pico_runtime_init

#ifndef PICO_RUNTIME_INIT_PSRAM
#define PICO_RUNTIME_INIT_PSRAM                 "11080"
#endif

#ifndef PICO_RUNTIME_SKIP_INIT_PSRAM
#define PICO_RUNTIME_SKIP_INIT_PSRAM 0
#endif

#ifndef PICO_RUNTIME_NO_INIT_PSRAM
#define PICO_RUNTIME_NO_INIT_PSRAM 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*! \brief Check if PSRAM is available and initialized
 *  \ingroup hardware_psram
 *
 * \return true if PSRAM is available and initialized, false otherwise
 */
bool psram_is_available(void);

/*! \brief Get the size of the PSRAM
 *  \ingroup hardware_psram
 *
 * Retrieve the size of the PSRAM, either from PICO_PSRAM_SIZE_BYTES,
 * flash_devinfo, or auto-detection.
 *
 * \return size of PSRAM in bytes, or 0 if none
 */
size_t psram_get_size(void);

/*! \brief Check if an address is in available PSRAM
 *  \ingroup hardware_psram
 *
 * \return true if the address is in available PSRAM, false otherwise
 */
bool psram_check_address(void* addr);

/*! \brief Detect PSRAM size
 *  \ingroup hardware_psram
 *
 * This will read the ID of the PSRAM chip and return the size based on the ID.
 *
 * You must configure the GPIO function for the CS pin before calling this function,
 * and should also configure the CS GPIO in flash_devinfo to prevent toggling of the
 * previously configured GPIO (usually 0, so prints invalid characters to default UART).
 *
 * \return size of PSRAM, or 0 if none found
 */
size_t psram_detect_size(void);

/*! \brief Detect PSRAM chip select pin and size
 *  \ingroup hardware_psram
 *
 * This runs \ref psram_detect_size() for each CS GPIO in the array in turn,
 * and returns the size as soon as a PSRAM chip is detected.
 *
 * This will setup the CS GPIO using flash_devinfo if PSRAM is found.
 *
 * \param cs_gpios Array of CS GPIOs to try
 * \param num Number of CS GPIOs in the array
 * \return size of PSRAM, or 0 if none found
 */
size_t psram_detect_cs_and_size(uint8_t *cs_gpios, size_t num);

/*! \brief Configure PSRAM timing parameters
 *  \ingroup hardware_psram
 *
 * This will calculate and set the PSRAM timing parameters based on the given values.
 *
 * Note: This will also implement the workaround for RP2350-E14 if PICO_RP2350_A2_SUPPORTED is set.
 *
 * \param max_psram_freq Maximum frequency of PSRAM
 * \param max_select_ns Maximum select time in ns
 * \param min_deselect_ns Minimum deselect time in ns
 * \return PICO_OK on success, PICO_ERROR_INVALID_ARG if unable to calculate valid parameters
 */
int psram_configure_params(uint32_t max_psram_freq, uint32_t max_select_ns, uint32_t min_deselect_ns);

/*! \brief Explicitly set PSRAM timing parameters
 *  \ingroup hardware_psram
 *
 * This will explicitly set the PSRAM timing parameters to the given values.
 *
 * This may be necessary if the parameters calculated by \ref psram_configure_params are not suitable.
 *
 * \param divisor Divisor for PSRAM clock
 * \param rxdelay RX delay for PSRAM clock
 * \param max_select Maximum select time in multiples of 64 system clocks
 * \param min_deselect Minimum deselect time in system clock cycles - ceil(divisor / 2)
 * \return PICO_OK on success, PICO_ERROR_INVALID_ARG if any of the parameters are invalid
 */
int psram_set_params(uint32_t divisor, uint32_t rxdelay, uint32_t max_select, uint32_t min_deselect);

/*! \brief Re-initialize PSRAM
 *  \ingroup hardware_psram
 *
 * This will re-initialize the PSRAM with the parameters set by \ref psram_configure_params.
 *
 * This calls \ref flash_start_xip internally, so will reset any QSPI pads changes you have made.
 *
 * \return PICO_OK on success, PICO_ERROR_PRECONDITION_NOT_MET if the PSRAM size is not set in
 * flash_devinfo or the PSRAM parameters are not set by \ref psram_configure_params or \ref psram_set_params
 */
int psram_reinitialize(void);

/*! \brief Convert PSRAM EID to size
 *  \ingroup hardware_psram
 *
 * This will convert the PSRAM EID to the size in bytes.
 *
 * This is not intended to be called by the user, but is provided as a weak function so it can
 * be overridden if other PSRAM chips are used that have different EID to size mapping.
 *
 * This is used by \ref psram_detect_size to check the KGD and convert the EID to the size.
 *
 * \param kgd Known Good Die
 * \param eid EID
 * \return size of PSRAM in bytes, or 0 if the KGD/EID is not recognised
 */
size_t psram_eid_to_size(uint8_t kgd, uint8_t eid);

/*! \brief Provide a static PSRAM allocation, or malloc if PSRAM is not available
 *  \ingroup hardware_psram
 *
 * This will allocate a static buffer in PSRAM and if available use that, otherwise it will use the heap.
 * 
 * This will fail to compile if PICO_PSRAM_SIZE_BYTES is not set
 */
#define psram_or_malloc(group, type, var, size) \
    static type __uninitialized_psram(group) var##_psram[size]; static type* var; \
    if (!var) { \
        if (psram_check_address(var##_psram + (size))) { \
           var = (type*)var##_psram; \
        } else { \
            var = (type*)malloc((size) * sizeof(type)); \
        } \
    }

/*! \brief Free a buffer if it is not in PSRAM
 *  \ingroup hardware_psram
 *
 * This will free the buffer from \ref psram_or_malloc if it was created by malloc
 */
#define psram_or_free(var) if (!psram_check_address(var##_psram)) { free(var); }

#ifdef __cplusplus
}
#endif

#endif // _HARDWARE_PSRAM_H
