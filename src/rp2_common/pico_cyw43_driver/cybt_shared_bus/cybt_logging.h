/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CYBT_LOGGING_H
#define CYBT_LOGGING_H

// Error messages only enabled in debug by default
#ifndef CYBT_ERROR_ENABLED
#ifndef NDEBUG
#define CYBT_ERROR_ENABLED 1
#else
#define CYBT_ERROR_ENABLED 0
#endif
#endif

// Info messages only enabled in debug by default
#ifndef CYBT_INFO_ENABLED
#ifndef NDEBUG
#define CYBT_INFO_ENABLED 1
#else
#define CYBT_INFO_ENABLED 0
#endif
#endif

// Debug messages disabled by default
#ifndef CYBT_DEBUG_ENABLED
#define CYBT_DEBUG_ENABLED 0
#endif

#ifndef cybt_error
#if CYBT_ERROR_ENABLED
#define cybt_error CYW43_WARN
#else
#define cybt_error(...)
#endif
#endif

#ifndef cybt_info
#if CYBT_INFO_ENABLED
#define cybt_info CYW43_PRINTF
#else
#define cybt_info(...)
#endif
#endif

#ifndef cybt_debug
#if CYBT_DEBUG_ENABLED
#define cybt_debug CYW43_PRINTF
#else
#define cybt_debug(...)
#endif
#endif

#endif // CYBT_LOGGING_H