/*
* Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/cxx_options.h"

#define CXA_GUARD_UNINITIALIZED 0
#define CXA_GUARD_INITIALIZED 1
#define CXA_GUARD_INITIALIZING 2

#if PICO_CXX_THREAD_SAFE_STATIC_INIT
#include <stdatomic.h>
#include <hardware/sync.h>

// newer LLVM complains on RP2040 because there are no atomics (it doesn't know we have pico_atomic)
Clang_Pragma("clang diagnostic push")
Clang_Pragma("clang diagnostic ignored \"-Watomic-alignment\"")

int __cxa_guard_acquire(int32_t *guard_object) {
    // Cast the raw pointer to a standard C11 atomic pointer type
    _Atomic int32_t *atomic_guard = (_Atomic int32_t *)guard_object;

    while (true) {
        // read current state
        int32_t current = atomic_load_explicit(atomic_guard, memory_order_acquire);

        // bit 0 set -> Initialization is complete, skip constructor
        if (current & CXA_GUARD_INITIALIZED) {
            return 0;
        }

        // bit 1 set -> another thread/core is initializing it, so sleep
        if (current & CXA_GUARD_INITIALIZING) {
            __wfe();
            continue;
        }

        // attempt to atomically claim the lock by moving state from CXA_GUARD_UNINITIALIZED to CXA_GUARD_INITIALIZING.
        int32_t expected = CXA_GUARD_UNINITIALIZED;
        if (atomic_compare_exchange_weak_explicit(atomic_guard, &expected, CXA_GUARD_INITIALIZING,
                                                  memory_order_release,
                                                  memory_order_relaxed)) {
            return 1; // Success! This core must execute the constructor
        }
    }
}

void __cxa_guard_release(int32_t *guard_object) {
    _Atomic int32_t *atomic_guard = (_Atomic int32_t *)guard_object;

    // set Bit 0 (done) and clear Bit 1 (unlocked)
    atomic_store_explicit(atomic_guard, CXA_GUARD_INITIALIZED, memory_order_release);

    // wake up any waiters
    __sev();
}

void __cxa_guard_abort(int32_t *guard_object) {
    _Atomic int32_t *atomic_guard = (_Atomic int32_t *)guard_object;

    // reset back to uninitialized state
    atomic_store_explicit(atomic_guard, CXA_GUARD_UNINITIALIZED, memory_order_release);

    // wake up any waiters
    __sev();
}

Clang_Pragma("clang diagnostic pop")
#else

int __cxa_guard_acquire(volatile int32_t *guard_object) {
    int32_t guard_val = *guard_object;
    if (guard_val == CXA_GUARD_INITIALIZED) {
        return 0;
    }
    if (guard_val & CXA_GUARD_INITIALIZING) {
        panic("recursive CXX initializer");
    }
    *guard_object = CXA_GUARD_INITIALIZING;
    return 1;
}

void __cxa_guard_release(volatile int32_t *guard_object) {
    *guard_object = CXA_GUARD_INITIALIZED;
}

void __cxa_guard_abort(volatile int32_t *guard_object) {
    *guard_object = CXA_GUARD_UNINITIALIZED;
}
#endif