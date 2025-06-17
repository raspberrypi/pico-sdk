/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_H
#define _PICO_H

/** \file pico.h
 *  \defgroup pico_base pico_base
 *
 * \brief Core types and macros for the Raspberry Pi Pico SDK.
 *
 * This header is intended to be included by all source code
 * as it includes configuration headers and overrides in the correct order
 *
 * This header may be included by assembly code
*/

// We may be included by assembly which can't include <cdefs.h>
#define	__PICO_STRING(x)	#x
#define	__PICO_XSTRING(x)	__PICO_STRING(x)
#define __PICO_CONCAT1(x, y) x ## y

#include "pico/types.h"
#include "pico/version.h"

/**
 * \brief A marker used in board headers to specify a CMake variable and value that should be set in the CMake build when the board header is used
 * \ingroup pico_base
 *
 * Based on the PICO_BOARD CMake variable, the build will scan the board header for `pico_board_cmake_set(var, value)` and set these variables
 * very early in the build configuration process. This allows setting CMake variables like `PICO_PLATFORM` from the board header, and thus
 * affecting, for example, the choice of compiler made by the build
 *
 * \note use of this macro will overwrite the CMake variable if it is already set
 *
 * \note this macro's definition is empty as it is not intended to have any effect on actual compilation
 */
#define pico_board_cmake_set(x, y)

/**
 * \brief A marker used in board headers to specify a CMake variable and value that should be set in the CMake build when the board header is used,
 * if that CMake variable has not already been set
 * \ingroup pico_base
 *
 * Based on the PICO_BOARD CMake variable, the build will scan the board header for `pico_board_cmake_set_default(var, value)` and set these variables
 * very early in the build configuration process. This allows setting CMake variables like `PICO_PLATFORM` from the board header, and thus
 * affecting, for example, the choice of compiler made by the build
 *
 * \note use of this macro will not overwrite the CMake variable if it is already set
 *
 * \note this macro's definition is empty as it is not intended to have any effect on actual compilation
 */
#define pico_board_cmake_set_default(x, y)

// PICO_CONFIG: PICO_CONFIG_HEADER, Unquoted path to header include in place of the default pico/config.h which may be desirable for build systems which can't easily generate the config_autogen header, group=pico_base
#ifdef PICO_CONFIG_HEADER
#include __PICO_XSTRING(PICO_CONFIG_HEADER)
#else
#include "pico/config.h"
#endif
#include "pico/platform.h"
#include "pico/error.h"

#endif
