/*
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _HARDWARE_PSRAM_H
#define _HARDWARE_PSRAM_H

#include "pico.h"

/** \file psram.h
 *  \defgroup pico_psram pico_psram
 *
 * \brief Low level PSRAM setup functions
 *
 * \subsection psram_example Example
 * \include psram_program.c
 */

// PICO_CONFIG: PARAM_ASSERTIONS_ENABLED_HARDWARE_PSRAM, Enable/disable assertions in the hardware_psram module, type=bool, default=0, group=hardware_psram
#ifndef PARAM_ASSERTIONS_ENABLED_HARDWARE_PSRAM
#define PARAM_ASSERTIONS_ENABLED_HARDWARE_PSRAM 0
#endif

// PICO_CONFIG: PICO_PSRAM_SIZE_BYTES, size of psram in bytes, type=int, default=Usually provided via board header, group=hardware_psram
// PICO_CONFIG: PICO_PSRAM_CS_PIN, chip select pin for psram, type=int, default=Usually provided via board header, group=hardware_psram

// PICO_CONFIG: PICO_AUTO_DETECT_PSRAM, automatically detect psram presence, type=bool, default=0, group=hardware_psram
#ifndef PICO_AUTO_DETECT_PSRAM
#define PICO_AUTO_DETECT_PSRAM 0
#endif

// PICO_CONFIG: PICO_AUTO_DETECT_PSRAM_ID, ID of psram, type=int, default=0x5D, group=hardware_psram
#ifndef PICO_AUTO_DETECT_PSRAM_ID
#define PICO_AUTO_DETECT_PSRAM_ID 0x5D
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*! \brief Check if PSRAM is available
 *  \ingroup hardware_psram
 *
 * \return true if PSRAM is available, false otherwise
 */
bool psram_is_available(void);

#ifdef __cplusplus
}
#endif

#endif // _HARDWARE_PSRAM_H
