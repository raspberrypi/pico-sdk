/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include "pico.h"
#include "pico/malloc.h"

#if PICO_USE_MALLOC_MUTEX
#include "pico/mutex.h"
auto_init_mutex(malloc_mutex);
#endif

#if PICO_DEBUG_MALLOC
#include <stdio.h>
#endif

extern void *REAL_FUNC(malloc)(size_t size);
extern void *REAL_FUNC(calloc)(size_t count, size_t size);
extern void *REAL_FUNC(realloc)(void *mem, size_t size);
extern void REAL_FUNC(free)(void *mem);

extern char __StackLimit; /* Set by linker.  */

#if !PICO_USE_MALLOC_MUTEX
#define MALLOC_ENTER(outer) ((void)0);
#define MALLOC_EXIT(outer) ((void)0);
#elif !__PICOLIBC__
#define MALLOC_ENTER(outer) mutex_enter_blocking(&malloc_mutex);
#define MALLOC_EXIT(outer) mutex_exit(&malloc_mutex);
#else
static uint8_t mutex_exception_level_plus_one[NUM_CORES];
// PICOLIBC implementations of calloc and realloc may call malloc and free,
// so we need to cope with re-entrant calls. We don't want to use a recursive
// mutex as that won't catch usage within an ISR; instead we record the exception
// nesting as of acquiring the mutex
#define MALLOC_ENTER(outer) \
    uint exception = __get_current_exception(); \
    uint core_num = get_core_num(); \
    /* we skip the locking on outer == false if we are in the same irq nesting */ \
    /* note: the `+ 1` is to distinguish no malloc nesting vs no-exception/irq level */ \
    bool do_lock = outer || exception + 1 != mutex_exception_level_plus_one[core_num]; \
    if (do_lock) { \
        mutex_enter_blocking(&malloc_mutex); \
        if (outer) mutex_exception_level_plus_one[core_num] = (uint8_t)(exception + 1); \
    }

#define MALLOC_EXIT(outer) \
    if (outer) { \
        mutex_exception_level_plus_one[core_num] = 0; \
    } \
    if (do_lock) { \
        mutex_exit(&malloc_mutex); \
    }

#endif

static inline void check_alloc(__unused void *mem, __unused uint size) {
#if PICO_MALLOC_PANIC
    if (!mem || (((char *)mem) + size) > &__StackLimit) {
        panic("Out of memory");
    }
#endif
}

void *WRAPPER_FUNC(malloc)(size_t size) {
    MALLOC_ENTER(false)
    void *rc = REAL_FUNC(malloc)(size);
    MALLOC_EXIT(false)
#if PICO_DEBUG_MALLOC
    if (!rc) {
        printf("malloc %d failed to allocate memory\n", (uint) size);
    } else if (((uint8_t *)rc) + size > (uint8_t*)PICO_DEBUG_MALLOC_LOW_WATER) {
        printf("malloc %d %p->%p\n", (uint) size, rc, ((uint8_t *) rc) + size);
    }
#endif
    check_alloc(rc, size);
    return rc;
}

void *WRAPPER_FUNC(calloc)(size_t count, size_t size) {
    MALLOC_ENTER(true)
    void *rc = REAL_FUNC(calloc)(count, size);
    MALLOC_EXIT(true)
#if PICO_DEBUG_MALLOC
    if (!rc) {
        printf("calloc %d failed to allocate memory\n", (uint) (count * size));
    } else if (((uint8_t *)rc) + count * size > (uint8_t*)PICO_DEBUG_MALLOC_LOW_WATER) {
        printf("calloc %d %p->%p\n", (uint) (count * size), rc, ((uint8_t *) rc) + size);
    }
#endif
    check_alloc(rc, count * size);
    return rc;
}

void *WRAPPER_FUNC(realloc)(void *mem, size_t size) {
    MALLOC_ENTER(true)
    void *rc = REAL_FUNC(realloc)(mem, size);
    MALLOC_EXIT(true)
#if PICO_DEBUG_MALLOC
    if (!rc) {
        printf("realloc %d failed to allocate memory\n", (uint) size);
    } else if (((uint8_t *)rc) + size > (uint8_t*)PICO_DEBUG_MALLOC_LOW_WATER) {
        printf("realloc %p %d->%p\n", mem, (uint) size, rc);
    }
#endif
    check_alloc(rc, size);
    return rc;
}

void WRAPPER_FUNC(free)(void *mem) {
    MALLOC_ENTER(false)
    REAL_FUNC(free)(mem);
    MALLOC_EXIT(false)
}
