/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2019 Keith Packard
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 */
#ifndef _PICO_THREAD_LOCAL_PICOTLS_H
#define _PICO_THREAD_LOCAL_PICOTLS_H

// Note the name of this file is slightly confusing - this is actually overriding <picotls.h> from picolibc (if present)!

/** \file pico/tls.h
 *  \defgroup pico_thread_local pico_thread_local
 *
 * \brief C/C++ __thread/thread_local support
 *
 * This library provides transparent runtime support for thread locals for the different types of thread local implementations used by the SDK supported compilers and C libraries.
 *
 * This means that by default, each core will get its own value for the thread local variable, and the core 1 value will be re-initialized if the core is restarted.
 *
 * Additionally, this library provides the "picolibc" style methods `_tls_size()`, `_init_tls()` and `_set_tls()` even when not using picolibc, which makes
 * it easy to enable thread locals per task on at least FreeRTOS (just set `#define configUSE_PICOLIBC_TLS 1` in your `FreeRTOSConfig.h`).
 *
 * The pico_thread_local library comes in three main flavors:
 *
 * 1. `pico_thread_local_per_thread` (default) - Full support for thread local variables. Each thread/core gets its own copy of the variable initialized when it starts executing. The library tries to minimize the overhead if there are no thread locals used, however particularly on RISC-V there is always a small overhead.
 * 2. `pico_thread_local_global` - Thread local variables are allowed in code, but each thread/core shares the same value (i.e. they aren't really thread local). There is very minimial overhead for this option.
 * 3. `pico_thread_local_none` - No support for thread locals is provided. Code using them may not compile or link, and if it does the values won't be shared and may not even be initialized correctly. This mode is however guaranteed to have basically no overhead when thread locals are known not to be used.
 *
 * The support provided by the `pico_thread_local` library may be set from CMake via `pico_set_thread_local_implementation(<TARGET> per_thread|global|none)` and the default for all targets may be set via CMake variable (e.g. `set(PICO_DEFAULT_THREAD_LOCAL_IMPL pico_thread_local_none)`).
 */

// PICO_CONFIG: PICO_THREAD_LOCAL_MODE_PER_THREAD, Enable proper thread local support with one value per thread, type=bool, default=1, group=pico_thread_local
// PICO_CONFIG: PICO_THREAD_LOCAL_MODE_GLOBAL, Support compiling code with thread local variables but only keep one global value, type=bool, default=0, group=pico_thread_local
// PICO_CONFIG: PICO_THREAD_LOCAL_MODE_NONE, No support for thread local variables. Code using __thread may or may not compile/link or work correctly, type=bool, default=0, group=pico_thread_local

// PICO_CONFIG: PICO_THREAD_LOCAL_SUPPORT_EMUTLS, Thread local support should work with compilers that use emutls, type=bool, default=1, advanced=true, group=pico_thread_local
// PICO_CONFIG: PICO_THREAD_LOCAL_SUPPORT_THREAD_POINTER, Thread local support should work with compilers that use a per thread pointer and provide .tdata and .tbss, type=bool, default=1, advanced=true, group=pico_thread_local

// PICO_CONFIG: PICO_THREAD_LOCAL_EMUTLS_CONFIRMED, Passed from build if the compiler is known to use Emutls and so other types of support are not needed, type=bool, default=0, advanced=true, group=pico_thread_local
// PICO_CONFIG: PICO_THREAD_LOCAL_THREAD_POINTER_ARM_AEABI_CONFIRMED, Passed from build if the compiler is known to use a thread pointer accessed via __areabi_read_tp and so other types of support are not needed, type=bool, default=0, advanced=true, group=pico_thread_local
// PICO_CONFIG: PICO_THREAD_LOCAL_THREAD_POINTER_RISCV_REG_CONFIRMED, Passed from build if the compiler is known to use a thread pointer stored in the TP register and so other types of support are not needed, type=bool, default=0, advanced=true, group=pico_thread_local

// Default is PICO_THREAD_LOCAL_MODE_PER_THREAD if not specified
#if !PICO_THREAD_LOCAL_MODE_PER_THREAD && !PICO_THREAD_LOCAL_MODE_GLOBAL && !PICO_THREAD_LOCAL_MODE_NONE
#undef PICO_THREAD_LOCAL_MODE_PER_THREAD
#define PICO_THREAD_LOCAL_MODE_PER_THREAD 1
#endif

// Rest of file is contingent on not being in PICO_THREAD_LOCAL_MODE_NONE...
#if !PICO_THREAD_LOCAL_MODE_NONE
#if !PICO_THREAD_LOCAL_SUPPORT_EMUTLS && !PICO_THREAD_LOCAL_SUPPORT_THREAD_POINTER
// PICO_THREAD_LOCAL_EMUTLS_CONFIRMED, PICO_THREAD_LOCAL_THREAD_POINTER_ARM_AEABI_CONFIRMED PICO_THREAD_LOCAL_THREAD_POINTER_RISCV_REG_CONFIRMED are
// optional defines provided by the build that allow us to improve code size slightly if only one is set

#if PICO_THREAD_LOCAL_EMUTLS_CONFIRMED + PICO_THREAD_LOCAL_THREAD_POINTER_ARM_AEABI_CONFIRMED + PICO_THREAD_LOCAL_THREAD_POINTER_RISCV_REG_CONFIRMED > 1
#error expected at most one of PICO_THREAD_LOCAL_EMUTLS_CONFIRMED, PICO_THREAD_LOCAL_THREAD_POINTER_ARM_AEABI_CONFIRMED and PICO_THREAD_LOCAL_THREAD_POINTER_RISCV_REG_CONFIRMED to be set
#endif

#if PICO_THREAD_LOCAL_EMUTLS_CONFIRMED
#define PICO_THREAD_LOCAL_SUPPORT_THREAD_POINTER 0
#define PICO_THREAD_LOCAL_SUPPORT_EMUTLS 1
#elif PICO_THREAD_LOCAL_THREAD_POINTER_ARM_AEABI_CONFIRMED || PICO_THREAD_LOCAL_THREAD_POINTER_RISCV_REG_CONFIRMED || defined(PICOLIBC_TLS) || __riscv
// we assume here and in thread_local.c that if PICOLIBC_TLS is set, or we're on RISC-V
// then we're not using emutls - we may want to re-visit if in practice this is not always the case
//
// particularly, with respect to RISC-V the fact that we have to set TP on thread init
// means that TLS code cannot be elided on RISC-V so we want it to be as small as possible
#define PICO_THREAD_LOCAL_SUPPORT_THREAD_POINTER 1
#define PICO_THREAD_LOCAL_SUPPORT_EMUTLS 0
#else
// we know nothing, so support both at potential extra runtime cost
#define PICO_THREAD_LOCAL_SUPPORT_THREAD_POINTER 1
#define PICO_THREAD_LOCAL_SUPPORT_EMUTLS 1
#endif
#endif

#if PICO_THREAD_LOCAL_SUPPORT_EMUTLS || PICO_THREAD_LOCAL_MODE_GLOBAL || PICO_THREAD_LOCAL_MODE_NONE
#define PICO_THREAD_LOCAL_REQUIRES_CUSTOM_TLS_SIZE 1
#endif

// we can't use the stock header if supporting emutls as the TLS size isn't known at link time
#if !PICO_THREAD_LOCAL_REQUIRES_CUSTOM_TLS_SIZE && defined(__has_include_next) && __has_include_next(<picotls.h>)
// just use the stock header
#include_next <picotls.h>
#else

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#if PICO_THREAD_LOCAL_MODE_GLOBAL || PICO_THREAD_LOCAL_MODE_NONE
static inline size_t _tls_size(void) { return 0; }
#elif PICO_THREAD_LOCAL_SUPPORT_EMUTLS
extern size_t _runtime_tls_size(void);
static inline size_t _tls_size(void) { return _runtime_tls_size(); }
#else
// standard definition for picolibc
extern char __tls_size[];
static inline size_t _tls_size(void) { return (size_t) (uintptr_t) __tls_size; }
#endif

/*
 * Initialize a TLS block, copying the data segment from flash and
 * zeroing the BSS segment.
 */
void  _init_tls(void *tls);

/* Set the TLS pointer to the specific block */
void _set_tls(void *tls);

#ifdef __cplusplus
}
#endif

#endif

#endif

#endif
