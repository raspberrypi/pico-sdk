/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PICO_H_
#define PICO_H_

/** \file pico.h
 *  \defgroup pico_base pico_base
 *
 * Core types and macros for the Raspberry Pi Pico SDK. This header is intended to be included by all source code
 * as it includes configuration headers and overrides in the correct order
 *
 * This header may be included by assembly files.
*/

#ifndef __ASSEMBLER__
#include "pico/types.h"
#endif
#include "pico/version.h"
#include "pico/config.h"
#include "pico/platform_asm.h"
#ifndef __ASSEMBLER__
#include "pico/platform.h"
#include "pico/error.h"
#endif

#endif
