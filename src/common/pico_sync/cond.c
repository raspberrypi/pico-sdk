/*
 * Copyright (c) 2022-2023 Paul Guyot <pguyot@kallisys.net>
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
    lock_owner_id_t caller = lock_get_caller_owner_id();
    uint32_t save = save_and_disable_interrupts();
    // Acquire the mutex spin lock
    spin_lock_unsafe_blocking(mtx->core.spin_lock);
    assert(lock_is_owner_id_valid(mtx->owner));
    assert(caller == mtx->owner);

    // Mutex and cond spin locks can be the same as spin locks are attributed
    // using `next_striped_spin_lock_num()`. To avoid any deadlock, we only
    // acquire the condition variable spin lock if it is different from the
    // mutex spin lock
    bool same_spinlock = mtx->core.spin_lock == cond->core.spin_lock;

    // Acquire the condition variable spin_lock
    if (!same_spinlock) {
        spin_lock_unsafe_blocking(cond->core.spin_lock);
    }

    mtx->owner = LOCK_INVALID_OWNER_ID;

    uint64_t current_broadcast = cond->broadcast_count;

    if (lock_is_owner_id_valid(cond->waiter)) {
        // Release the mutex but without restoring interrupts and notify.
        if (!same_spinlock) {
            spin_unlock_unsafe(mtx->core.spin_lock);
        }

        // There is a valid owner of the condition variable: we are not the
        // first waiter.
        // First iteration: notify
        lock_internal_spin_unlock_with_notify(&cond->core, save);
        save = spin_lock_blocking(cond->core.spin_lock);
        // Further iterations: wait
        do {
            if (!lock_is_owner_id_valid(cond->waiter)) {
                break;
            }
            if (cond->broadcast_count != current_broadcast) {
                break;
            }
            if (is_at_the_end_of_time(until)) {
                lock_internal_spin_unlock_with_wait(&cond->core, save);
            } else {
                if (lock_internal_spin_unlock_with_best_effort_wait_or_timeout(&cond->core, save, until)) {
                    // timed out
                    success = false;
                    break;
                }
            }
            save = spin_lock_blocking(cond->core.spin_lock);
        } while (true);
    } else {
        // Release the mutex but without restoring interrupts
        if (!same_spinlock) {
            uint32_t disabled_ints = save_and_disable_interrupts();
            lock_internal_spin_unlock_with_notify(&mtx->core, disabled_ints);
        }
    }

    if (success && cond->broadcast_count == current_broadcast) {
        cond->waiter = caller;

        // Wait for the signal
        do {
            if (cond->signaled) {
                cond->waiter = LOCK_INVALID_OWNER_ID;
                cond->signaled = false;
                break;
            }
            if (is_at_the_end_of_time(until)) {
                lock_internal_spin_unlock_with_wait(&cond->core, save);
            } else {
                if (lock_internal_spin_unlock_with_best_effort_wait_or_timeout(&cond->core, save, until)) {
                    // timed out
                    cond->waiter = LOCK_INVALID_OWNER_ID;
                    success = false;
                    break;
                }
            }
            save = spin_lock_blocking(cond->core.spin_lock);
        } while (true);
    }

    // We got the signal (or timed out)

    if (lock_is_owner_id_valid(mtx->owner)) {
        // Acquire the mutex spin lock and release the core spin lock.
        if (!same_spinlock) {
            spin_lock_unsafe_blocking(mtx->core.spin_lock);
            spin_unlock_unsafe(cond->core.spin_lock);
        }

        // Another core holds the mutex.
        // First iteration: notify
        lock_internal_spin_unlock_with_notify(&mtx->core, save);
        save = spin_lock_blocking(mtx->core.spin_lock);
        // Further iterations: wait
        do {
            if (!lock_is_owner_id_valid(mtx->owner)) {
                break;
            }
            // We always wait for the mutex.
            lock_internal_spin_unlock_with_wait(&mtx->core, save);
            save = spin_lock_blocking(mtx->core.spin_lock);
        } while (true);
    } else {
        // Acquire the mutex spin lock and release the core spin lock
        // with notify but without restoring interrupts
        if (!same_spinlock) {
            spin_lock_unsafe_blocking(mtx->core.spin_lock);
            uint32_t disabled_ints = save_and_disable_interrupts();
            lock_internal_spin_unlock_with_notify(&cond->core, disabled_ints);
        }
    }

    // Eventually hold the mutex.
    mtx->owner = caller;

    // Restore the interrupts now
    spin_unlock(mtx->core.spin_lock, save);

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
        // We have a waiter, we can signal.
        cond->signaled = true;
        lock_internal_spin_unlock_with_notify(&cond->core, save);
    } else {
        spin_unlock(cond->core.spin_lock, save);
    }
}

void __time_critical_func(cond_broadcast)(cond_t *cond) {
    uint32_t save = spin_lock_blocking(cond->core.spin_lock);
    if (lock_is_owner_id_valid(cond->waiter)) {
        // We have a waiter, we can broadcast.
        cond->signaled = true;
        cond->broadcast_count++;
        lock_internal_spin_unlock_with_notify(&cond->core, save);
    } else {
        spin_unlock(cond->core.spin_lock, save);
    }
}
