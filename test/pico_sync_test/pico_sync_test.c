/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "../../src/common/pico_base_headers/include/pico/types.h"
#include "../../src/common/pico_time/include/pico/time.h"
#include "../../src/rp2_common/hardware_sync_spin_lock/include/hardware/sync/spin_lock.h"
#include "pico/lock_core.h"
#include "pico/test.h"
#include "pico/stdio.h"

PICOTEST_MODULE_NAME("SYNC", "sync test");

typedef struct {
    lock_core_t lock;
    bool flag;
} lock_with_flag_t;;

int64_t notify_lock_with_flag(__unused alarm_id_t id, void *user_data) {
    lock_with_flag_t *lock_with_flag = (lock_with_flag_t *)user_data;
    uint32_t save = spin_lock_blocking(lock_with_flag->lock.spin_lock);
    lock_with_flag->flag = true;
    lock_internal_spin_unlock_with_notify(&lock_with_flag->lock, save);
    return 0;
}

static lock_core_t sleep_notifier;

static int64_t sleep_until_callback(__unused alarm_id_t id, __unused void *user_data) {
    uint32_t save = spin_lock_blocking(sleep_notifier.spin_lock);
    lock_internal_spin_unlock_with_notify(&sleep_notifier, save);
    return 0;
}

int main() {
    stdio_init_all();

    PICOTEST_START();


    PICOTEST_START_SECTION("check low power lock_core wait loop with timeout");
        #if PICO_USE_SW_SPIN_LOCKS
            // looping twice (rather than once) is an implementation detail, however we shouldn't loop continuously (i.e. we should __wfe)
            #define EXPECTED_TIMEOUT_WAITS 2
        #else
            // note that without sw spin locks we wait an extra time (an extra SEV doesn't get eaten)
            #define EXPECTED_TIMEOUT_WAITS 3
        #endif
        lock_core_t lock;
        lock_init(&lock, 0);
        absolute_time_t until = make_timeout_time_ms(50);
        int wait_count = 0;
        do {
            uint32_t save = spin_lock_blocking(lock.spin_lock);
            wait_count++;
            if (lock_internal_spin_unlock_with_best_effort_wait_or_timeout(&lock, save, until)) break;
        } while (true);
        printf("Waited %d times\n", wait_count);
        PICOTEST_CHECK(wait_count == EXPECTED_TIMEOUT_WAITS, "Expected exactly " __XSTRING(EXPECTED_TIMEOUT_WAITS) " waits");
    PICOTEST_END_SECTION();

    PICOTEST_START_SECTION("check low power lock_core wait loop without timeout");
    lock_with_flag_t lock_with_flag;
    lock_init(&lock_with_flag.lock, 0);
    lock_with_flag.flag = false;
    add_alarm_in_ms(50, notify_lock_with_flag, &lock_with_flag, false);
    __wfe(); // consume outstanding one from adding alarm
    int wait_count = 0;
    do {
        uint32_t save = spin_lock_blocking(lock_with_flag.lock.spin_lock);
        if (lock_with_flag.flag) {
            spin_unlock(lock_with_flag.lock.spin_lock, save);
            break;
        }
        wait_count++;
        lock_internal_spin_unlock_with_wait(&lock_with_flag.lock, save);
    } while (true);
    printf("Waited %d times\n", wait_count);
    PICOTEST_CHECK(wait_count == 1, "Expected exactly 1 wait");
    PICOTEST_END_SECTION();

    PICOTEST_START_SECTION("check low power sleep loop");
    // note sleep implementation taken from pico_ime
    lock_init(&sleep_notifier, PICO_SPINLOCK_ID_TIMER);
    int wait_count = 0;
    absolute_time_t t = make_timeout_time_ms(500);
    uint64_t t_us = to_us_since_boot(t);
    uint64_t t_before_us = t_us - PICO_TIME_SLEEP_OVERHEAD_ADJUST_US;
    // needs to work in the first PICO_TIME_SLEEP_OVERHEAD_ADJUST_US of boot
    if (t_before_us > t_us) t_before_us = 0;
    absolute_time_t t_before;
    update_us_since_boot(&t_before, t_before_us);
    if (absolute_time_diff_us(get_absolute_time(), t_before) > 0) {
        if (add_alarm_at(t_before, sleep_until_callback, NULL, false) >= 0) {
            // able to add alarm for just before the time
            while (!time_reached(t_before)) {
                uint32_t save = spin_lock_blocking(sleep_notifier.spin_lock);
                lock_internal_spin_unlock_with_wait(&sleep_notifier, save);
                wait_count++;
            }
        }
    }
    printf("Waited %d times\n", wait_count);
    #undef EXPECTED_TIMEOUT_WAITS
    #if PICO_USE_SW_SPIN_LOCKS
        // looping twice (rather than once) is an implementation detail, however we shouldn't loop continuously (i.e. we should __wfe)
    #define EXPECTED_TIMEOUT_WAITS 1
    #else
        // note that without sw spin locks we wait an extra time (an extra SEV doesn't get eaten)
    #define EXPECTED_TIMEOUT_WAITS 2
    #endif

    PICOTEST_CHECK(wait_count == EXPECTED_TIMEOUT_WAITS, "Expected exactly " __XSTRING(EXPECTED_TIMEOUT_WAITS) " waits");
    PICOTEST_END_SECTION();

    PICOTEST_END_TEST();
}
