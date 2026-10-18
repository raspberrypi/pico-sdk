/**
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/test.h"
#include "hardware/irq.h"
#include "hardware/sync.h"

PICOTEST_MODULE_NAME("alarm_pool_sleep", "alarm_pool_sleep test harness");

// How late a sleep may return; the wake-up alarm is set PICO_TIME_SLEEP_OVERHEAD_ADJUST_US
// early and the rest is a busy wait, so this only allows for IRQ latency
#define MAX_LATE_US 50

#define CORE1_SLEEPS 1000
#define CORE1_SLEEP_US 100

static int wake_time_test(alarm_pool_t *pool);
static int full_pool_test(void);
static int other_core_test(void);

int main(void) {
    setup_default_uart();
    PICOTEST_START();

    alarm_pool_t *pool = alarm_pool_create_with_unused_hardware_alarm(4);
    PICOTEST_CHECK_AND_ABORT(pool, "failed to create alarm pool");
    picotest_error_code |= wake_time_test(pool);
    alarm_pool_destroy(pool);

    picotest_error_code |= full_pool_test();
    picotest_error_code |= other_core_test();

    PICOTEST_END_TEST();
}

// Check a sleep that started at start and was asked to last us
static bool check_elapsed(absolute_time_t start, uint64_t us, const char *what) {
    int64_t late = absolute_time_diff_us(delayed_by_us(start, us), get_absolute_time());
    if (late < 0 || late > MAX_LATE_US) {
        printf("  %s(%llu): returned %lldus late\n", what, us, late);
        return false;
    }
    return true;
}

static int wake_time_test(alarm_pool_t *pool) {
    PICOTEST_START_SECTION("alarm_pool_sleep_* wake at the right time");
    // the shortest ones are shorter than PICO_TIME_SLEEP_OVERHEAD_ADJUST_US, so are busy waits
    static const uint32_t durations_us[] = {0, 1, 5, 10, 100, 1000, 5000};
    for (uint i = 0; i < count_of(durations_us); i++) {
        uint32_t us = durations_us[i];
        absolute_time_t start = get_absolute_time();
        alarm_pool_sleep_until(pool, delayed_by_us(start, us));
        PICOTEST_CHECK(check_elapsed(start, us, "alarm_pool_sleep_until"), "alarm_pool_sleep_until woke at the wrong time");

        start = get_absolute_time();
        alarm_pool_sleep_us(pool, us);
        PICOTEST_CHECK(check_elapsed(start, us, "alarm_pool_sleep_us"), "alarm_pool_sleep_us woke at the wrong time");
    }
    for (uint32_t ms = 1; ms <= 3; ms++) {
        absolute_time_t start = get_absolute_time();
        alarm_pool_sleep_ms(pool, ms);
        PICOTEST_CHECK(check_elapsed(start, ms * 1000ull, "alarm_pool_sleep_ms"), "alarm_pool_sleep_ms woke at the wrong time");
    }
    PICOTEST_END_SECTION();
    return picotest_error_code;
}

static int64_t never_called(__unused alarm_id_t id, __unused void *user_data) {
    return 0;
}

static int full_pool_test(void) {
    PICOTEST_START_SECTION("alarm_pool_sleep_* busy wait when the pool is full");
    alarm_pool_t *pool = alarm_pool_create_with_unused_hardware_alarm(1);
    PICOTEST_CHECK_AND_ABORT(pool, "failed to create alarm pool");
    alarm_id_t id = alarm_pool_add_alarm_in_ms(pool, 10000, never_called, NULL, false);
    PICOTEST_CHECK_AND_ABORT(id > 0, "failed to add alarm");
    PICOTEST_CHECK(alarm_pool_add_alarm_in_ms(pool, 10000, never_called, NULL, false) < 0, "pool should be full");

    absolute_time_t start = get_absolute_time();
    alarm_pool_sleep_us(pool, 2000);
    PICOTEST_CHECK(check_elapsed(start, 2000, "alarm_pool_sleep_us"), "alarm_pool_sleep_us woke at the wrong time");

    alarm_pool_cancel_alarm(pool, id);
    alarm_pool_destroy(pool);
    PICOTEST_END_SECTION();
    return picotest_error_code;
}

static volatile bool core1_done;
static volatile uint32_t core1_max_late_us;
static volatile bool core1_woke_early;

static void core1_sleeps(alarm_pool_t *pool, int count) {
    uint32_t max_late = 0;
    bool early = false;
    for (int i = 0; i < count; i++) {
        absolute_time_t t = make_timeout_time_us(CORE1_SLEEP_US);
        if (pool) {
            alarm_pool_sleep_until(pool, t);
        } else {
#if !PICO_TIME_DEFAULT_ALARM_POOL_DISABLED
            sleep_until(t);
#endif
        }
        int64_t late = absolute_time_diff_us(t, get_absolute_time());
        if (late < 0) early = true;
        else if ((uint64_t)late > max_late) max_late = (uint32_t)late;
    }
    core1_max_late_us = max_late;
    core1_woke_early = early;
}

// Core 1 waits for a command: a positive count to sleep that many times on a pool it creates
// itself, a negative one to sleep on the default alarm pool
static void core1_entry(void) {
    while (true) {
        int count = (int)multicore_fifo_pop_blocking();
        alarm_pool_t *pool = NULL;
        if (count > 0) pool = alarm_pool_create_with_unused_hardware_alarm(4);
        core1_sleeps(pool, count > 0 ? count : -count);
        if (pool) alarm_pool_destroy(pool);
        core1_done = true;
    }
}

static bool wait_core1_done(uint32_t timeout_us) {
    absolute_time_t timeout = make_timeout_time_us(timeout_us);
    while (!core1_done) {
        if (time_reached(timeout)) return false;
    }
    return true;
}

static int other_core_test(void) {
    PICOTEST_START_SECTION("alarm_pool_sleep_* on core 1's own pool doesn't need or interrupt core 0");
    multicore_launch_core1(core1_entry);
    uint default_irq = hardware_alarm_get_irq_num(PICO_TIME_DEFAULT_ALARM_POOL_HARDWARE_ALARM_NUM);

    // With interrupts off on core 0, core 1 must still wake on time, and nothing may be left
    // pending for core 0 to take: core 1's alarms are on its own pool, whose IRQ only it handles
    uint32_t save = save_and_disable_interrupts();
    irq_clear(default_irq);
    core1_done = false;
    multicore_fifo_push_blocking(CORE1_SLEEPS);
    bool done = wait_core1_done(CORE1_SLEEPS * CORE1_SLEEP_US * 2);
    bool pending = irq_is_pending(default_irq);
    restore_interrupts(save);
    PICOTEST_CHECK_AND_ABORT(done, "core 1 did not finish its sleeps");
    printf("  core 1's own pool: core 1 max late %uus\n", (uint)core1_max_late_us);
    PICOTEST_CHECK(!core1_woke_early, "core 1 woke early");
    PICOTEST_CHECK(core1_max_late_us <= MAX_LATE_US, "core 1 woke too late");
    PICOTEST_CHECK(!pending, "core 1's sleeps left the default alarm pool's IRQ pending on core 0");

#if !PICO_TIME_DEFAULT_ALARM_POOL_DISABLED
    // Control: sleeping on the default pool does raise its IRQ for core 0, which shows the check
    // above can see it
    save = save_and_disable_interrupts();
    irq_clear(default_irq);
    core1_done = false;
    multicore_fifo_push_blocking((uint32_t)-1);
    absolute_time_t timeout = make_timeout_time_us(10000);
    while (!(pending = irq_is_pending(default_irq)) && !time_reached(timeout)) tight_loop_contents();
    restore_interrupts(save);
    PICOTEST_CHECK(pending, "expected sleeping on the default alarm pool to raise its IRQ on core 0");
    PICOTEST_CHECK_AND_ABORT(wait_core1_done(100000), "core 1 did not finish sleeping on the default pool");
#endif

    multicore_reset_core1();
    PICOTEST_END_SECTION();
    return picotest_error_code;
}
