/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* To support IAR's runtime library, which features multiple link-time
 * selectable heap implementations, this file is designed to be
 * multiply included with PREFIX set to the appropriate function name
 * prefix (if any) */

extern void *REAL_FUNC_EXP(__CONCAT(PREFIX,malloc))(size_t size);
extern void *REAL_FUNC_EXP(__CONCAT(PREFIX,calloc))(size_t count, size_t size);
extern void *REAL_FUNC_EXP(__CONCAT(PREFIX,realloc))(void *mem, size_t size);
extern void REAL_FUNC_EXP(__CONCAT(PREFIX,free))(void *mem);

void *WRAPPER_FUNC_EXP(__CONCAT(PREFIX,malloc))(size_t size) {
#if PICO_USE_MALLOC_MUTEX
    mutex_enter_blocking(&malloc_mutex);
#endif
    void *rc = REAL_FUNC_EXP(__CONCAT(PREFIX,malloc))(size);
#if PICO_USE_MALLOC_MUTEX
    mutex_exit(&malloc_mutex);
#endif
#if PICO_DEBUG_MALLOC
    if (!rc || ((uint8_t *)rc) + size > (uint8_t*)PICO_DEBUG_MALLOC_LOW_WATER) {
        printf("malloc %d %p->%p\n", (uint) size, rc, ((uint8_t *) rc) + size);
    }
#endif
    check_alloc(rc, size);
    return rc;
}

void *WRAPPER_FUNC_EXP(__CONCAT(PREFIX,calloc))(size_t count, size_t size) {
#if PICO_USE_MALLOC_MUTEX
    mutex_enter_blocking(&malloc_mutex);
#endif
    void *rc = REAL_FUNC_EXP(__CONCAT(PREFIX,calloc))(count, size);
#if PICO_USE_MALLOC_MUTEX
    mutex_exit(&malloc_mutex);
#endif
#if PICO_DEBUG_MALLOC
    if (!rc || ((uint8_t *)rc) + size > (uint8_t*)PICO_DEBUG_MALLOC_LOW_WATER) {
        printf("calloc %d %p->%p\n", (uint) (count * size), rc, ((uint8_t *) rc) + size);
    }
#endif
    check_alloc(rc, size);
    return rc;
}

void *WRAPPER_FUNC_EXP(__CONCAT(PREFIX,realloc))(void *mem, size_t size) {
#if PICO_USE_MALLOC_MUTEX
    mutex_enter_blocking(&malloc_mutex);
#endif
    void *rc = REAL_FUNC_EXP(__CONCAT(PREFIX,realloc))(mem, size);
#if PICO_USE_MALLOC_MUTEX
    mutex_exit(&malloc_mutex);
#endif
#if PICO_DEBUG_MALLOC
    if (!rc || ((uint8_t *)rc) + size > (uint8_t*)PICO_DEBUG_MALLOC_LOW_WATER) {
        printf("realloc %p %d->%p\n", mem, (uint) size, rc);
    }
#endif
    check_alloc(rc, size);
    return rc;
}

void WRAPPER_FUNC_EXP(__CONCAT(PREFIX,free))(void *mem) {
#if PICO_USE_MALLOC_MUTEX
    mutex_enter_blocking(&malloc_mutex);
#endif
    REAL_FUNC_EXP(__CONCAT(PREFIX,free))(mem);
#if PICO_USE_MALLOC_MUTEX
    mutex_exit(&malloc_mutex);
#endif
}
