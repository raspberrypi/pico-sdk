/*
* Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_TLS_H
#define _PICO_TLS_H

#include <picotls.h>

// PICO_CONFIG: PICO_TLS_CORE1_REINITIALIZE, Whether re-initializing core 1 should reset its thread local variables. A tiny space-saving can be achieved by disabling this support if not needed, type=bool, default=1, advanced=true, group=pico_tls
// PICO_CONFIG: PICO_TLS_PROVIDE_INIT_TLS, Whether pico_tls should provide its own implementation of _init_tls, type=bool, default=1 for non-picolibc and 0 for picolibc, advanced=true, group=pico_tls
// PICO_CONFIG: PICO_TLS_PROVIDE_SET_TLS, Whether pico_tls should provide its own implementation of _set_tls, type=bool, default=1, advanced=true, group=pico_tls

#if !defined(PICO_TLS_CORE1_REINITIALIZE) && LIB_PICO_MULTICORE
#define PICO_TLS_CORE1_REINITIALIZE 1
#endif

#if !PICO_TLS_MODE_NONE
#ifndef PICO_TLS_PROVIDE_INIT_TLS
#ifndef PICOLIBC_TLS // this provides the function, and frankly in that case we don't expect emutls - is that fair tho?
#define PICO_TLS_PROVIDE_INIT_TLS 1
#endif
#endif

#ifndef PICO_TLS_PROVIDE_SET_TLS
#define PICO_TLS_PROVIDE_SET_TLS 1
#endif
#endif

#if PICO_TLS_SUPPORT_THREAD_POINTER
// auto-select THREAD_POINTER type if not provided
#if !PICO_TLS_THREAD_POINTER_VIA_ARM_EABI && !PICO_TLS_THREAD_POINTER_VIA_RISCV_REG
#if __riscv || PICO_TLS_THREAD_POINTER_RISCV_REG_CONFIRMED
#define PICO_TLS_THREAD_POINTER_VIA_RISCV_REG 1
#else
#define PICO_TLS_THREAD_POINTER_VIA_ARM_EABI 1
#endif
#endif
#endif

#endif