/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/lock_core.h"

#if PICO_SYNC_RP2350_SPIN_LOCK_WORKAROUND
volatile uint8_t lock_internal_notify_count;
#endif

void lock_init(lock_core_t *core, uint lock_num) {
    valid_params_if(LOCK_CORE, lock_num < NUM_SPIN_LOCKS);
    core->spin_lock = spin_lock_instance(lock_num);
}

