/*
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_PLATFORM_ASM_H_
#define _PICO_PLATFORM_ASM_H_

#include "hardware/platform_defs.h"

// This is included by both C code and ASM so should be preprocessor defines only.

// Marker for builds targeting the RP2040
#define PICO_RP2040 1

// PICO_CONFIG: PICO_STACK_SIZE, Stack Size, min=0x100, default=0x800, advanced=true, group=pico_standard_link
#ifndef PICO_STACK_SIZE
#define PICO_STACK_SIZE _u(0x800)
#endif

// PICO_CONFIG: PICO_HEAP_SIZE, Heap size to reserve, min=0x100, default=0x800, advanced=true, group=pico_standard_link
#ifndef PICO_HEAP_SIZE
#define PICO_HEAP_SIZE _u(0x800)
#endif

// PICO_CONFIG: PICO_NO_RAM_VECTOR_TABLE, Enable/disable the RAM vector table, type=bool, default=0, advanced=true, group=pico_runtime
#ifndef PICO_NO_RAM_VECTOR_TABLE
#define PICO_NO_RAM_VECTOR_TABLE 0
#endif

// PICO_CONFIG: PICO_RP2040_B0_SUPPORTED, Include support for RP2040 B0 revision, type=bool, default=1, advanced=true, group=pico_platform
#ifndef PICO_RP2040_B0_SUPPORTED
#define PICO_RP2040_B0_SUPPORTED 1
#endif

// PICO_CONFIG: PICO_RP2040_B1_SUPPORTED, Include support for RP2040 B1 revision, type=bool, default=1, advanced=true, group=pico_platform
#ifndef PICO_RP2040_B1_SUPPORTED
#define PICO_RP2040_B1_SUPPORTED 1
#endif

// PICO_CONFIG: PICO_RP2040_B2_SUPPORTED, Include support for RP2040 B2 revision, type=bool, default=1, advanced=true, group=pico_platform
#ifndef PICO_RP2040_B2_SUPPORTED
#define PICO_RP2040_B2_SUPPORTED 1
#endif

#endif