/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CYBT_LOGGING_H
#define CYBT_LOGGING_H

// Error messages only enabled in debug by default
#ifndef NDEBUG
#define CYBT_ERROR_ENABLED 1
#else
#define CYBT_ERROR_ENABLED 0
#endif

// Info messages only enabled in debug by default
#ifndef NDEBUG
#define CYBT_INFO_ENABLED 1
#else
#define CYBT_INFO_ENABLED 0
#endif

// Debug messages disabled by default
#define CYBT_DEBUG_ENABLED 0

#if CYBT_DEBUG_ENABLED || CYBT_ERROR_ENABLED
#include <stdio.h>
#endif

#if CYBT_ERROR_ENABLED
#define cybt_error CYW43_WARN
#else
#define cybt_error(...)
#endif

#if CYBT_INFO_ENABLED
#define cybt_info CYW43_PRINTF
#else
#define cybt_info(...)
#endif

#if CYBT_DEBUG_ENABLED
#define cybt_debug CYW43_PRINTF
#else
#define cybt_debug(...)
#endif

#endif // CYBT_LOGGING_H