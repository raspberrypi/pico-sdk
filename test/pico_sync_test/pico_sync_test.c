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

// Counts wakeups from the wait loops inside pico_sync and pico_time, via the blocked_waiter_wakeup hook
volatile int wakeups[NUM_CORES];

void pico_sync_test_wakeup(__unused bool timed_out) {
    wakeups[get_core_num()]++;
}

static semaphore_t release_sem;

static int64_t release_sem_callback(__unused alarm_id_t id, __unused void *user_data) {
    sem_release(&release_sem);
    return 0;
}

static semaphore_t core1_sem;

static int do_test(void) {
    printf("=== Test on core %d ===\n", get_core_num());

    PICOTEST_START_SECTION("check low power lock_core wait loop with timeout");
    // A semaphore with no permits, so the acquire waits out the whole timeout
    semaphore_t empty_sem;
    sem_init(&empty_sem, 0, 1);
    absolute_time_t until = make_timeout_time_ms(50);
    wakeups[get_core_num()] = 0;
    bool acquired = sem_acquire_block_until(&empty_sem, until);
    int wait_count = wakeups[get_core_num()];
    printf("Waited %d times\n", wait_count);
    PICOTEST_CHECK(!acquired, "Expected the acquire to time out");
    PICOTEST_CHECK(time_reached(until), "Expected to have reached the timeout");
    // a count of zero means the wait never happened, or blocked_waiter_wakeup is not reaching us
    PICOTEST_CHECK(wait_count > 0, "Expected at least one wait");
    PICOTEST_CHECK(wait_count <= MAX_WAITS, "Expected <= %d waits", MAX_WAITS);
    PICOTEST_END_SECTION();

    PICOTEST_START_SECTION("check low power lock_core wait loop without timeout");
    // Nothing to acquire until the alarm releases a permit, so the acquire has to wait
    sem_init(&release_sem, 0, 1);
    add_alarm_in_ms(50, release_sem_callback, NULL, false);
    wakeups[get_core_num()] = 0;
    sem_acquire_blocking(&release_sem);
    int wait_count = wakeups[get_core_num()];
    printf("Waited %d times\n", wait_count);
    // a count of zero means the wait never happened, or blocked_waiter_wakeup is not reaching us
    PICOTEST_CHECK(wait_count > 0, "Expected at least one wait");
    PICOTEST_CHECK(wait_count <= MAX_WAITS, "Expected <= %d waits", MAX_WAITS);
    PICOTEST_END_SECTION();

    PICOTEST_START_SECTION("check low power sleep loop");
    absolute_time_t sleep_target = make_timeout_time_ms(500);
    wakeups[get_core_num()] = 0;
    sleep_until(sleep_target);
    int wait_count = wakeups[get_core_num()];
    printf("Waited %d times\n", wait_count);
    PICOTEST_CHECK(time_reached(sleep_target), "Expected to have slept until the target");
    // a count of zero means the wait never happened, or blocked_waiter_wakeup is not reaching us
    PICOTEST_CHECK(wait_count > 0, "Expected at least one wait");
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
