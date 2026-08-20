/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/sem.h"
#include "pico/time.h"

void sem_init(semaphore_t *sem, int16_t initial_permits, int16_t max_permits) {
    lock_init(&sem->core, next_striped_spin_lock_num());
    sem->permits = initial_permits;
    sem->max_permits = max_permits;
    __mem_fence_release();
}

int __time_critical_func(sem_available)(semaphore_t *sem) {
#if defined(__GNUC__) && !defined(__STRICT_ANSI__)
    return *(volatile typeof(sem->permits) *) &sem->permits;
#else
    static_assert(sizeof(sem->permits) == 2, "");
    return *(volatile int16_t *) &sem->permits;
#endif
}

void __time_critical_func(sem_acquire_blocking)(semaphore_t *sem) {
    // only a caller which waited can have consumed a notification, so only it can owe one to
    // the waiters it did not exclude - taking one of several permits leaves the others able to
    // proceed. Taking a permit on the first pass consumes no notification, so owes nothing.
    bool __unused waited = false;
    do {
        uint32_t save = spin_lock_blocking(sem->core.spin_lock);
        if (sem->permits > 0) {
            sem->permits--;
            lock_internal_spin_unlock_maybe_notify(&sem->core, save, waited && sem->permits != 0);
            break;
        }
        lock_internal_spin_unlock_with_wait(&sem->core, save);
        waited = true;
        blocked_waiter_wakeup();
    } while (true);
}

bool __time_critical_func(sem_acquire_timeout_ms)(semaphore_t *sem, uint32_t timeout_ms) {
    return sem_acquire_block_until(sem, make_timeout_time_ms(timeout_ms));
}

bool __time_critical_func(sem_acquire_timeout_us)(semaphore_t *sem, uint32_t timeout_us) {
    return sem_acquire_block_until(sem, make_timeout_time_us(timeout_us));
}

bool __time_critical_func(sem_acquire_block_until)(semaphore_t *sem, absolute_time_t until) {
    bool __unused waited = false;
    do {
        uint32_t save = spin_lock_blocking(sem->core.spin_lock);
        if (sem->permits > 0) {
            sem->permits--;
            lock_internal_spin_unlock_maybe_notify(&sem->core, save, waited && sem->permits != 0);
            return true;
        }
        if (lock_internal_spin_unlock_with_best_effort_wait_or_timeout(&sem->core, save, until)) {
            blocked_waiter_wakeup();
            return false;
        }
        waited = true;
        blocked_waiter_wakeup();
    } while (true);
}

bool __time_critical_func(sem_try_acquire)(semaphore_t *sem) {
    uint32_t save = spin_lock_blocking(sem->core.spin_lock);
    if (sem->permits > 0) {
        sem->permits--;
        // no notify: this never waits, so it cannot have consumed one. A waiter it beat to the
        // permit is still parked with its own notification intact.
        spin_unlock(sem->core.spin_lock, save);
        return true;
    }
    spin_unlock(sem->core.spin_lock, save);
    return false;
}

// todo this should really have a blocking variant for when permits are maxed out
bool __time_critical_func(sem_release)(semaphore_t *sem) {
    uint32_t save = spin_lock_blocking(sem->core.spin_lock);
    int32_t count = sem->permits;
    if (count < sem->max_permits) {
        sem->permits = (int16_t)(count + 1);
        lock_internal_spin_unlock_with_notify(&sem->core, save);
        return true;
    } else {
        spin_unlock(sem->core.spin_lock, save);
        return false;
    }
}

void __time_critical_func(sem_reset)(semaphore_t *sem, int16_t permits) {
    assert(permits >= 0 && permits <= sem->max_permits);
    uint32_t save = spin_lock_blocking(sem->core.spin_lock);
    if (permits > sem->permits) {
        sem->permits = permits;
        lock_internal_spin_unlock_with_notify(&sem->core, save);
    } else {
        sem->permits = permits;
        spin_unlock(sem->core.spin_lock, save);
    }
}
