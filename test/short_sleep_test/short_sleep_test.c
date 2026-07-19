/**
* Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "pico/test.h"

static_assert(PICO_TIME_SLEEP_OVERHEAD_ADJUST_US == 0, "want to test without sleep busy wait");


#define DELAY_US_MIN 0
#define DELAY_US_MAX 50
#define SLEEPS_PER_DELAY 20
#ifndef NDEBUG
#define MIN_SLEEP_US 20
#define SLOWER_CORE0_MARGIN_US 5
#else
#define MIN_SLEEP_US 10
#define SLOWER_CORE0_MARGIN_US 3
#endif

bool allow_slower_core0;

int do_sleeps() {
    uint core_num = get_core_num();
    uint32_t tstart = time_us_32();
    for (int delay = DELAY_US_MIN; delay < DELAY_US_MAX; delay++) {
        uint32_t t0 = time_us_32();
        uint32_t t2;
        for (int count = 0; count < SLEEPS_PER_DELAY; count++) {
            uint32_t t1 = time_us_32();
            sleep_us(delay);
            t2 = time_us_32();
            // do minimum time check per sleep as we should never wake up too soon
            int tdelta = (int)(t2 - t1);
            if (tdelta < delay) {
                printf("Core %u: tdelta for sleep %d %d < expected min %d\n", core_num , delay, tdelta, delay);
                return -1;
            }
        }
        // do maximum time check per group of sleeps to allow for some jitter
        int tdelta = (int)(t2 - t0);
        int max_delay = MAX((delay + 2), MIN_SLEEP_US) * SLEEPS_PER_DELAY + 1;
        if (allow_slower_core0 && !core_num) max_delay += SLOWER_CORE0_MARGIN_US * SLEEPS_PER_DELAY;
        if (tdelta > max_delay) {
            printf("Cored %u: cumulative tdelta for sleep %d %d > expected max %d\n", core_num, delay, tdelta, max_delay);
            return -1;
        }
    }
    uint32_t tend = time_us_32();
    printf("Core %u: total time %dus\n", core_num, (int)(tend - tstart));
    return 0;
}

semaphore_t core1_sem;
int core1_picotest_error_code;

void core1_do_sleeps() {
    core1_picotest_error_code = do_sleeps();
    sem_release(&core1_sem);
}

int main() {
    stdio_init_all();
    sem_init(&core1_sem, 0, 1);

    PICOTEST_MODULE_NAME("short_sleep_test", "short_sleep test harness");
    PICOTEST_START();
    PICOTEST_START_SECTION("sleeps core 0");
    picotest_error_code = do_sleeps();
    PICOTEST_END_SECTION();
    PICOTEST_START_SECTION("sleeps core 1");
    multicore_launch_core1(core1_do_sleeps);
    sem_acquire_blocking(&core1_sem);
    picotest_error_code = core1_picotest_error_code;
    PICOTEST_END_SECTION();
    PICOTEST_START_SECTION("sleeps core 0 & 1");
    allow_slower_core0 = true;
    multicore_reset_core1();
    multicore_launch_core1(core1_do_sleeps);
    picotest_error_code = do_sleeps();
    sem_acquire_blocking(&core1_sem);
    if (!picotest_error_code) picotest_error_code = core1_picotest_error_code;
    PICOTEST_END_SECTION();
    PICOTEST_END_TEST();
}