/*
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _HARDWARE_PSRAM_H
#define _HARDWARE_PSRAM_H

#include "pico.h"
#include "hardware/flash.h"

/** \file psram.h
 *  \defgroup pico_psram pico_psram
 *
 * \brief Low level PSRAM setup functions
 *
 * Note some of these functions are *unsafe* if you are using both cores, and the other
 * is executing from flash or psram concurrently with the operation. In this case, you
 * must perform your own synchronisation to make sure that no XIP accesses take
 * place during flash programming. One option is to use the
 * \ref multicore_lockout functions.
 *
 * Likewise they are *unsafe* if you have interrupt handlers or an interrupt
 * vector table in flash or psram, so you must disable interrupts before calling in
 * this case.
 *
 * The unsafe functions are:
 * - \ref psram_reinitialise
 * - \ref psram_detect_cs_and_size
 * - \ref psram_detect_size
 *
 * \subsection psram_example Example
 * \include psram_program.c
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

// PICO_CONFIG: PICO_AUTO_DETECT_PSRAM_CS, automatically detect psram chip select pin, default=PICO_AUTO_DETECT_PSRAM, group=hardware_psram
#ifndef PICO_AUTO_DETECT_PSRAM_CS
#define PICO_AUTO_DETECT_PSRAM_CS PICO_AUTO_DETECT_PSRAM
#elif PICO_AUTO_DETECT_PSRAM_CS && !PICO_AUTO_DETECT_PSRAM_SIZE
#error "PICO_AUTO_DETECT_PSRAM_SIZE must be set to use PICO_AUTO_DETECT_PSRAM_CS"
#endif

// PICO_CONFIG: PICO_AUTO_DETECT_PSRAM_CS_SKIP_DEFAULTS, skip default GPIOs when auto-detecting psram chip select pin, type=bool, default=1, group=hardware_psram
#ifndef PICO_AUTO_DETECT_PSRAM_CS_SKIP_DEFAULTS
#define PICO_AUTO_DETECT_PSRAM_CS_SKIP_DEFAULTS 1
#endif

#if PICO_AUTO_DETECT_PSRAM_CS
#if PICO_RP2350
#define PICO_AVAILABLE_CS1_GPIOS {0, 8, 19, 47}
#else
#error "PICO_AVAILABLE_CS1_GPIOS must be defined for this platform to use PICO_AUTO_DETECT_PSRAM_CS"
#endif
#endif

// PICO_CONFIG: PICO_DEFAULT_PSRAM_ID, Default ID of psram used for auto-detection, type=int, default=0x5D, group=hardware_psram
#ifndef PICO_DEFAULT_PSRAM_ID
#define PICO_DEFAULT_PSRAM_ID 0x5D
#endif

// PICO_CONFIG: PICO_DEFAULT_PSRAM_MAX_FREQ, Default max frequency of psram, type=int, default=133000000, group=hardware_psram
#ifndef PICO_DEFAULT_PSRAM_MAX_FREQ
#define PICO_DEFAULT_PSRAM_MAX_FREQ 133000000
#endif

// PICO_CONFIG: PICO_DEFAULT_PSRAM_MAX_SELECT, Default max select time in ns of psram, type=int, default=8000, group=hardware_psram
#ifndef PICO_DEFAULT_PSRAM_MAX_SELECT
#define PICO_DEFAULT_PSRAM_MAX_SELECT 8000
#endif

// PICO_CONFIG: PICO_DEFAULT_PSRAM_MIN_DESELECT, Default min deselect time in ns of psram, type=int, default=18, group=hardware_psram
#ifndef PICO_DEFAULT_PSRAM_MIN_DESELECT
#define PICO_DEFAULT_PSRAM_MIN_DESELECT 18
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*! \brief Check if PSRAM is initialised
 *  \ingroup hardware_psram
 *
 * \return true if PSRAM is initialised, false otherwise
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

/*! \brief Detect PSRAM size
 *  \ingroup hardware_psram
 *
 * This will read the ID of the PSRAM chip and return the size based on the ID.
 *
 * You must configure the GPIO function for the CS pin before calling this function,
 * and should also configure the GPIO in flash_devinfo to prevent toggling of the
 * previously configured GPIO (usually 0, so prints invalid characters to UART).
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
 * This will set the CS pin in flash_devinfo if PSRAM is found, and configure the GPIO function.
 *
 * \param cs_gpios Array of CS GPIOs to try
 * \param num Number of CS GPIOs in the array
 * \return size of PSRAM, or 0 if none found
 */
size_t psram_detect_cs_and_size(uint8_t *cs_gpios, size_t num);

/*! \brief Configure PSRAM timing parameters
 *  \ingroup hardware_psram
 *
 * This will setup this library to use the given timing parameters.
 *
 * \param max_psram_freq Maximum frequency of PSRAM
 * \param max_select_ns Maximum select time in ns
 * \param min_deselect_ns Minimum deselect time in ns
 */
void psram_configure_params(uint32_t max_psram_freq, uint32_t max_select_ns, uint32_t min_deselect_ns);

/*! \brief Re-initialise PSRAM
 *  \ingroup hardware_psram
 *
 * This will re-initialise the PSRAM with the parameters set by \ref psram_configure_params.
 *
 * This calls \ref flash_start_xip internally, so will reset any QSPI pads changes you have made.
 */
void psram_reinitialise(void);

#ifdef __cplusplus
}
#endif

#endif // _HARDWARE_PSRAM_H
