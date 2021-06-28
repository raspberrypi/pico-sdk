/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/mutex.h"
#include "pico/time.h"

static void mutex_init_internal(mutex_t *mtx, bool recursive) {
    lock_init(&mtx->core, next_striped_spin_lock_num());
    mtx->owner = LOCK_INVALID_OWNER_ID;
    mtx->recursive = recursive;
    __mem_fence_release();
}

void mutex_init(mutex_t *mtx) {
    mutex_init_internal(mtx, false);
}

void recursive_mutex_init(mutex_t *mtx) {
    mutex_init_internal(mtx, true);
}

static void __time_critical_func(mutex_enter_blocking_nr_guts)(mutex_t *mtx) {
    lock_owner_id_t caller = lock_get_caller_owner_id();
    do {
        uint32_t save = spin_lock_blocking(mtx->core.spin_lock);
        if (!lock_is_owner_id_valid(mtx->owner)) {
            mtx->owner = caller;
            spin_unlock(mtx->core.spin_lock, save);
            break;
        }
        lock_internal_spin_unlock_with_wait(&mtx->core, save);
    } while (true);
}

static void __time_critical_func(mutex_enter_blocking_r_guts)(mutex_t *mtx) {
    lock_owner_id_t caller = lock_get_caller_owner_id();
    do {
        uint32_t save = spin_lock_blocking(mtx->core.spin_lock);
        if (mtx->owner == caller || !lock_is_owner_id_valid(mtx->owner)) {
            mtx->owner = caller;
            uint __unused total = mtx->enter_count++;
            spin_unlock(mtx->core.spin_lock, save);
            assert(!total); // check for overflow
            return;
        } else {
            lock_internal_spin_unlock_with_wait(&mtx->core, save);
        }
    } while (true);
}

void __time_critical_func(mutex_enter_blocking_nr)(mutex_t *mtx) {
    assert(!mtx->recursive);
    mutex_enter_blocking_nr_guts(mtx);
}

void __time_critical_func(mutex_enter_blocking_r)(mutex_t *mtx) {
    assert(mtx->recursive);
    mutex_enter_blocking_r_guts(mtx);
}

void __time_critical_func(mutex_enter_blocking)(mutex_t *mtx) {
    assert(mtx->core.spin_lock);
    if (!mtx->recursive) {
        mutex_enter_blocking_nr_guts(mtx);
    } else {
        mutex_enter_blocking_r_guts(mtx);
    }
}

static bool __time_critical_func(mutex_try_enter_nr_guts)(mutex_t *mtx, uint32_t *owner_out) {
    bool entered;
    uint32_t save = spin_lock_blocking(mtx->core.spin_lock);
    if (!lock_is_owner_id_valid(mtx->owner)) {
        mtx->owner = lock_get_caller_owner_id();
        entered = true;
    } else {
        if (owner_out) *owner_out = (uint32_t) mtx->owner;
        entered = false;
    }
    spin_unlock(mtx->core.spin_lock, save);
    return entered;
}

static bool __time_critical_func(mutex_try_enter_r_guts)(mutex_t *mtx, uint32_t *owner_out) {
    bool entered;
    lock_owner_id_t caller = lock_get_caller_owner_id();
    uint32_t save = spin_lock_blocking(mtx->core.spin_lock);
    if (!lock_is_owner_id_valid(mtx->owner) || (mtx->owner == caller && mtx->recursive)) {
        mtx->owner = caller;
        uint __unused total = mtx->enter_count++;
        assert(!total); // check for overflow
        entered = true;
    } else {
        if (owner_out) *owner_out = (uint32_t) mtx->owner;
        entered = false;
    }
    spin_unlock(mtx->core.spin_lock, save);
    return entered;
}

bool __time_critical_func(mutex_try_enter_nr)(mutex_t *mtx, uint32_t *owner_out) {
    assert(!mtx->recursive);
    return mutex_try_enter_nr_guts(mtx, owner_out);
}

bool __time_critical_func(mutex_try_enter_r)(mutex_t *mtx, uint32_t *owner_out) {
    assert(mtx->recursive);
    return mutex_try_enter_r_guts(mtx, owner_out);
}

bool __time_critical_func(mutex_try_enter)(mutex_t *mtx, uint32_t *owner_out) {
    if (!mtx->recursive) {
        return mutex_try_enter_nr_guts(mtx, owner_out);
    } else {
        return mutex_try_enter_r_guts(mtx, owner_out);
    }
}

bool __time_critical_func(mutex_enter_timeout_ms)(mutex_t *mtx, uint32_t timeout_ms) {
    return mutex_enter_block_until(mtx, make_timeout_time_ms(timeout_ms));
}

bool __time_critical_func(mutex_enter_timeout_us)(mutex_t *mtx, uint32_t timeout_us) {
    return mutex_enter_block_until(mtx, make_timeout_time_us(timeout_us));
}

bool __time_critical_func(mutex_enter_block_until)(mutex_t *mtx, absolute_time_t until) {
    assert(mtx->core.spin_lock);
    lock_owner_id_t caller = lock_get_caller_owner_id();
    do {
        uint32_t save = spin_lock_blocking(mtx->core.spin_lock);
        if (!lock_is_owner_id_valid(mtx->owner) || (mtx->owner == caller && mtx->recursive)) {
            mtx->owner = caller;
            uint __unused total = mtx->enter_count++;
            spin_unlock(mtx->core.spin_lock, save);
            assert(!total); // check for overflow
            return true;
        } else {
            if (lock_internal_spin_unlock_with_best_effort_wait_or_timeout(&mtx->core, save, until)) {
                // timed out
                return false;
            }
            // not timed out; spin lock already unlocked, so loop again
        }
    } while (true);
}

static void __time_critical_func(mutex_exit_nr_guts)(mutex_t *mtx) {
    uint32_t save = spin_lock_blocking(mtx->core.spin_lock);
    assert(lock_is_owner_id_valid(mtx->owner));
    mtx->owner = LOCK_INVALID_OWNER_ID;
    lock_internal_spin_unlock_with_notify(&mtx->core, save);
}

static void __time_critical_func(mutex_exit_r_guts)(mutex_t *mtx) {
    uint32_t save = spin_lock_blocking(mtx->core.spin_lock);
    assert(mtx->enter_count);
    if (!--mtx->enter_count) {
        lock_internal_spin_unlock_with_notify(&mtx->core, save);
    } else {
        spin_unlock(mtx->core.spin_lock, save);
    }
}

void __time_critical_func(mutex_exit_nr)(mutex_t *mtx) {
    assert(!mtx->recursive);
    mutex_exit_nr_guts(mtx);
}

void __time_critical_func(mutex_exit_r)(mutex_t *mtx) {
    assert(mtx->recursive);
    mutex_exit_r_guts(mtx);
}

void __time_critical_func(mutex_exit)(mutex_t *mtx) {
    if (!mtx->recursive) {
        mutex_exit_nr_guts(mtx);
    } else {
        mutex_exit_r_guts(mtx);
    }
}