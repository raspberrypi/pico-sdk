/*
 * Copyright (c) 2022-2025 Paul Guyot <pguyot@kallisys.net>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/cond.h"

void cond_init(cond_t *cond) {
    lock_init(&cond->core, next_striped_spin_lock_num());
    cond->waiter = LOCK_INVALID_OWNER_ID;
    cond->broadcast_count = 0;
    cond->signaled = false;
    __mem_fence_release();
}

bool __time_critical_func(cond_wait_until)(cond_t *cond, mutex_t *mtx, absolute_time_t until) {
    bool success = true;
    bool broadcast = false;
    bool mutex_acquired = false;
    lock_owner_id_t caller = lock_get_caller_owner_id();

    // waiter and mtx_core are protected by the cv spin_lock
    uint32_t save = spin_lock_blocking(cond->core.spin_lock);
    uint64_t current_broadcast = cond->broadcast_count;
    if (lock_is_owner_id_valid(cond->waiter)) {
        // There is a valid owner of the condition variable: we are not the
        // first waiter.
        assert(cond->mtx_core.spin_lock == mtx->core.spin_lock);

        // wait until it's released
        lock_internal_spin_unlock_with_wait(&cond->core, save);
        do {
            save = spin_lock_blocking(cond->core.spin_lock);
            if (cond->broadcast_count != current_broadcast) {
                // Condition variable was broadcast while we were waiting to
                // own it.
                spin_unlock(cond->core.spin_lock, save);
                broadcast = true;
                break;
            }
            if (!lock_is_owner_id_valid(cond->waiter)) {
                cond->waiter = caller;
                cond->mtx_core = mtx->core;
                spin_unlock(cond->core.spin_lock, save);
                break;
            }
            if (is_at_the_end_of_time(until)) {
                lock_internal_spin_unlock_with_wait(&cond->core, save);
            } else if (lock_internal_spin_unlock_with_best_effort_wait_or_timeout(&cond->core, save, until)) {
                // timed out
                success = false;
                break;
            }
        } while (true);
    } else {
        cond->waiter = caller;
        cond->mtx_core = mtx->core;
        spin_unlock(cond->core.spin_lock, save);
    }

    save = spin_lock_blocking(mtx->core.spin_lock);
    assert(mtx->owner == caller);

    if (success && !broadcast) {
        if (cond->signaled) {
            // as an optimization, do not release the mutex.
            cond->signaled = false;
            mutex_acquired = true;
            spin_unlock(mtx->core.spin_lock, save);
        } else {
            // release mutex
            mtx->owner = LOCK_INVALID_OWNER_ID;
            lock_internal_spin_unlock_with_notify(&mtx->core, save);
            do {
                if (cond->signaled) {
                    cond->signaled = false;
                    if (!lock_is_owner_id_valid(mtx->owner)) {
                        // As an optimization, acquire the mutex here
                        mtx->owner = caller;
                        mutex_acquired = true;
                    }
                    spin_unlock(mtx->core.spin_lock, save);
                    break;
                }
                if (!success) {
                    if (!lock_is_owner_id_valid(mtx->owner)) {
                        // As an optimization, acquire the mutex here
                        mtx->owner = caller;
                        mutex_acquired = true;
                    }
                    spin_unlock(mtx->core.spin_lock, save);
                    break;
                }
                if (is_at_the_end_of_time(until)) {
                    lock_internal_spin_unlock_with_wait(&mtx->core, save);
                } else if (lock_internal_spin_unlock_with_best_effort_wait_or_timeout(&mtx->core, save, until)) {
                    // timed out
                    success = false;
                }
                save = spin_lock_blocking(mtx->core.spin_lock);
            } while (true);
        }
    }

    // free the cond var
    save = spin_lock_blocking(cond->core.spin_lock);
    if (cond->waiter == caller) {
        cond->waiter = LOCK_INVALID_OWNER_ID;
    }
    lock_internal_spin_unlock_with_notify(&cond->core, save);

    if (!mutex_acquired) {
        mutex_enter_blocking(mtx);
    }

    return success;
}

bool __time_critical_func(cond_wait_timeout_ms)(cond_t *cond, mutex_t *mtx, uint32_t timeout_ms) {
    return cond_wait_until(cond, mtx, make_timeout_time_ms(timeout_ms));
}

bool __time_critical_func(cond_wait_timeout_us)(cond_t *cond, mutex_t *mtx, uint32_t timeout_us) {
    return cond_wait_until(cond, mtx, make_timeout_time_us(timeout_us));
}

void __time_critical_func(cond_wait)(cond_t *cond, mutex_t *mtx) {
    cond_wait_until(cond, mtx, at_the_end_of_time);
}

void __time_critical_func(cond_signal)(cond_t *cond) {
    uint32_t save = spin_lock_blocking(cond->core.spin_lock);
    if (lock_is_owner_id_valid(cond->waiter)) {
        lock_core_t mtx_core = cond->mtx_core;
        // spin_locks can be identical
        if (mtx_core.spin_lock != cond->core.spin_lock) {
            spin_unlock(cond->core.spin_lock, save);
            save = spin_lock_blocking(mtx_core.spin_lock);
        }
        cond->signaled = true;
        lock_internal_spin_unlock_with_notify(&mtx_core, save);
    } else {
        spin_unlock(cond->core.spin_lock, save);
    }
}

void __time_critical_func(cond_broadcast)(cond_t *cond) {
    uint32_t save = spin_lock_blocking(cond->core.spin_lock);
    if (lock_is_owner_id_valid(cond->waiter)) {
        cond->broadcast_count++;
        lock_core_t mtx_core = cond->mtx_core;
        // spin_locks can be identical
        if (mtx_core.spin_lock != cond->core.spin_lock) {
            lock_internal_spin_unlock_with_notify(&cond->core, save);
            save = spin_lock_blocking(mtx_core.spin_lock);
        }
        cond->signaled = true;
        lock_internal_spin_unlock_with_notify(&mtx_core, save);
    } else {
        spin_unlock(cond->core.spin_lock, save);
    }
}
