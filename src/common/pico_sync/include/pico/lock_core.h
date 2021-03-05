/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_LOCK_CORE_H
#define _PICO_LOCK_CORE_H

#include "pico.h"
#include "hardware/sync.h"

// PICO_CONFIG: PARAM_ASSERTIONS_ENABLED_LOCK_CORE, Enable/disable assertions in the lock core, type=bool, default=0, group=pico_sync
#ifndef PARAM_ASSERTIONS_ENABLED_LOCK_CORE
#define PARAM_ASSERTIONS_ENABLED_LOCK_CORE 0
#endif

/** \file lock_core.h
 *  \ingroup pico_sync
 *
 * Base implementation for locking primitives protected by a spin lock
 */
typedef struct lock_core {
    // spin lock protecting this lock's state
    spin_lock_t *spin_lock;

    // note any lock members in containing structures need not be volatile;
    // they are protected by memory/compiler barriers when gaining and release spin locks
} lock_core_t;

void lock_init(lock_core_t *core, uint lock_num);

#endif
