/*
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_CXX_OPTIONS_H
#define _PICO_CXX_OPTIONS_H

#include "pico.h"

// PICO_CONFIG: PICO_CXX_DISABLE_ALLOCATION_OVERRIDES, Disable SDK overrides of C++ new/delete operators, type=bool, default=0, advanced=true, group=pico_cxx_options

// PICO_CONFIG: PICO_CXX_THREAD_SAFE_STATIC_INIT, Enable thread/IRQ safe locks for C++ static initializers, type=bool, default=1 when using pico_multicore, advanced=true, group=pico_cxx_options
#ifndef PICO_CXX_THREAD_SAFE_STATIC_INIT
#define PICO_CXX_THREAD_SAFE_STATIC_INIT LIB_PICO_MULTICORE
#endif

#endif


