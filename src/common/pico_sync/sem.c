/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/sem.h"
#include "pico/time.h"
#include "sys/select.h"

void sem_init(semaphore_t *sem, int16_t initial_permits, int16_t max_permits) {
    lock_init(&sem->core, next_striped_spin_lock_num());
    sem->permits = initial_permits;
    sem->max_permits = max_permits;
    __mem_fence_release();
}

int __time_critical_func(sem_available)(semaphore_t *sem) {
    return *(volatile typeof(sem->permits) *) &sem->permits;
}

void __time_critical_func(sem_acquire_blocking)(semaphore_t *sem) {
    do {
        uint32_t save = spin_lock_blocking(sem->core.spin_lock);
        if (sem->permits > 0) {
            sem->permits--;
            lock_internal_spin_unlock_with_notify(&sem->core, save);
            break;
        }
        lock_internal_spin_unlock_with_wait(&sem->core, save);
    } while (true);
}

bool __time_critical_func(sem_acquire_timeout_ms)(semaphore_t *sem, uint32_t timeout_ms) {
    absolute_time_t target = nil_time;
    do {
        uint32_t save = spin_lock_blocking(sem->core.spin_lock);
        if (sem->permits > 0) {
            sem->permits--;
            lock_internal_spin_unlock_with_notify(&sem->core, save);
            return true;
        }
        if (is_nil_time(target)) {
            target = make_timeout_time_ms(timeout_ms);
        }
        if (lock_internal_spin_unlock_with_best_effort_wait_or_timeout(&sem->core, save, target)) {
            return false;
        }
    } while (true);
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
