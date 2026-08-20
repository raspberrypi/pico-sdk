/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/sync.h"
#include "pico/multicore.h"
#include "pico/test.h"

// Note in this test, the main thing is that the processor doesn't spin - the actual number of loops
// when successfully coming to a sleep is between 1 and 5 depending on platform/core etc. and the reasoning
// is left as an exercise for the reader!
#define MAX_WAITS 5

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

static int64_t sleep_until_callback(__unused alarm_id_t id, __unused void *user_data) {
    // note: this implementation is a copy of the code from pico_time/time.c and should be kept in sync
    __sev();
    return 0;
}

static semaphore_t core1_sem;

static int do_test(void) {
    printf("=== Test on core %d ===\n", get_core_num());

    PICOTEST_START_SECTION("check low power lock_core wait loop with timeout");
    // note: this implementation is implemented in a similar functions to mutex_enter_block_until(),
    //       sem_acquire_block_until() etc. and should be updated if they are
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

    PICOTEST_CHECK(wait_count <= MAX_WAITS, "Expected <= %d waits", MAX_WAITS);
    PICOTEST_END_SECTION();

    PICOTEST_START_SECTION("check low power lock_core wait loop without timeout");
    // note: this implementation is implemented in a similar functions to mutex_enter_blocking(),
    //       sem_acquire_blocking() etc. and should be updated if they are
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
    PICOTEST_CHECK(wait_count <= MAX_WAITS, "Expected <= %d waits", MAX_WAITS);
    PICOTEST_END_SECTION();

    PICOTEST_START_SECTION("check low power sleep loop");
    // note: this sleep implementation is a copy of the code from pico_time/time.c and should be kept in sync
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
                __wfe();
                wait_count++;
            }
        }
    }
    printf("Waited %d times\n", wait_count);
    PICOTEST_CHECK(wait_count <= MAX_WAITS, "Expected <= %d waits", MAX_WAITS);
    PICOTEST_END_SECTION();

    PICOTEST_START_SECTION("check repeated deadline issue");
    // wait one deadline out, so it is in the past and its one-shot alarm has fired
    absolute_time_t deadline = make_timeout_time_ms(100);
    while (!best_effort_wfe_or_timeout(deadline)) tight_loop_contents();
    PICOTEST_CHECK(time_reached(deadline), "Expected to reach timeout");
    printf("deadline %lld us expired\n", (long long)to_us_since_boot(deadline));

    // waiting on it again must return at once, every time; repeated because a latched event
    // can carry the first call or two before anything actually parks
    for (int i = 1; i <= 5; i++) {
        printf("call %d\n", i);
        bool reached = best_effort_wfe_or_timeout(deadline);
        PICOTEST_CHECK(reached, "Expected to reach timeout");
    }

    // the same, via the API the field report used
    printf("sem_acquire_block_until on the expired deadline\n");
    semaphore_t sem;
    sem_init(&sem, 0, 1);
    bool acquired = sem_acquire_block_until(&sem, deadline);
    PICOTEST_CHECK(!acquired, "Expected not to acquire semaphore");
    PICOTEST_END_SECTION();

    return picotest_error_code;
}

static int core1_picotest_error_code;

static void core1_func(void) {
    core1_picotest_error_code = do_test();
    sem_release(&core1_sem);
}

int main() {
    stdio_init_all();

    PICOTEST_START();

    // core 0 first; only then core 1, since the sections share picotest's globals
    picotest_error_code = do_test();
    if (!picotest_error_code) {
        sem_init(&core1_sem, 0, 1);
        multicore_launch_core1(core1_func);
        sem_acquire_blocking(&core1_sem);
        picotest_error_code = core1_picotest_error_code;
    }

    PICOTEST_END_TEST();
}
