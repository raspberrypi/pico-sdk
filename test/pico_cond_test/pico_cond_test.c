/**
 * Copyright (c) 2022-2023 Paul Guyot <pguyot@kallisys.net>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "pico/cond.h"
#include "pico/test.h"
#include "pico/multicore.h"
#include "pico/stdio.h"

PICOTEST_MODULE_NAME("COND", "condition variable test");

static cond_t cond;
static mutex_t mutex;

static volatile bool test_cond_wait_done;
static volatile bool test_cond_wait_ready;
static void test_cond_wait(void) {
    busy_wait_ms(100);
    mutex_enter_blocking(&mutex);
    test_cond_wait_ready = true;
    cond_wait(&cond, &mutex);
    test_cond_wait_done = true;
    mutex_exit(&mutex);
}

static volatile bool test_cond_wait_timedout;
static void test_cond_wait_timeout(void) {
    busy_wait_ms(100);
    mutex_enter_blocking(&mutex);
    test_cond_wait_ready = true;
    test_cond_wait_timedout = cond_wait_timeout_ms(&cond, &mutex, 200);
    test_cond_wait_done = true;
    mutex_exit(&mutex);
}

int main() {
    stdio_init_all();
    mutex_init(&mutex);
    cond_init(&cond);

    PICOTEST_START();

    PICOTEST_START_SECTION("test cond wait / signal with mutex");
        test_cond_wait_ready = false;
        test_cond_wait_done = false;
        multicore_launch_core1(test_cond_wait);
        busy_wait_ms(200);
        PICOTEST_CHECK(test_cond_wait_ready, "core1 is not ready");
        PICOTEST_CHECK(!test_cond_wait_done, "core1 did not wait for signal");
        mutex_enter_blocking(&mutex);
        cond_signal(&cond);
        busy_wait_ms(200);
        PICOTEST_CHECK(!test_cond_wait_done, "core1 did not wait for mutex release");
        mutex_exit(&mutex);
        busy_wait_ms(200);
        PICOTEST_CHECK(test_cond_wait_done, "core1 isn't done");
        multicore_reset_core1();
    PICOTEST_END_SECTION();

    PICOTEST_START_SECTION("test cond wait / signal without mutex");
        test_cond_wait_ready = false;
        test_cond_wait_done = false;
        multicore_launch_core1(test_cond_wait);
        busy_wait_ms(200);
        PICOTEST_CHECK(test_cond_wait_ready, "core1 is not ready");
        PICOTEST_CHECK(!test_cond_wait_done, "core1 did not wait for signal");
        cond_signal(&cond);
        busy_wait_ms(200);
        PICOTEST_CHECK(test_cond_wait_done, "core1 isn't done");
        multicore_reset_core1();
    PICOTEST_END_SECTION();

    PICOTEST_START_SECTION("test cond wait / broadcast with mutex");
        test_cond_wait_ready = false;
        test_cond_wait_done = false;
        multicore_launch_core1(test_cond_wait);
        busy_wait_ms(200);
        PICOTEST_CHECK(test_cond_wait_ready, "core1 is not ready");
        PICOTEST_CHECK(!test_cond_wait_done, "core1 did not wait for signal");
        mutex_enter_blocking(&mutex);
        cond_broadcast(&cond);
        busy_wait_ms(200);
        PICOTEST_CHECK(!test_cond_wait_done, "core1 did not wait for mutex release");
        mutex_exit(&mutex);
        busy_wait_ms(200);
        PICOTEST_CHECK(test_cond_wait_done, "core1 isn't done");
        multicore_reset_core1();
    PICOTEST_END_SECTION();

    PICOTEST_START_SECTION("test cond wait / broadcast without mutex");
        test_cond_wait_ready = false;
        test_cond_wait_done = false;
        multicore_launch_core1(test_cond_wait);
        busy_wait_ms(200);
        PICOTEST_CHECK(test_cond_wait_ready, "core1 is not ready");
        PICOTEST_CHECK(!test_cond_wait_done, "core1 did not wait for signal");
        cond_broadcast(&cond);
        busy_wait_ms(200);
        PICOTEST_CHECK(test_cond_wait_done, "core1 isn't done");
        multicore_reset_core1();
    PICOTEST_END_SECTION();

    PICOTEST_START_SECTION("test cond wait with timeout and no signal");
        test_cond_wait_ready = false;
        test_cond_wait_done = false;
        test_cond_wait_timedout = false;
        multicore_launch_core1(test_cond_wait_timeout);
        busy_wait_ms(200);
        PICOTEST_CHECK(test_cond_wait_ready, "core1 is not ready");
        PICOTEST_CHECK(!test_cond_wait_done, "core1 did not wait");
        busy_wait_ms(200);
        PICOTEST_CHECK(!test_cond_wait_timedout, "core1 did not time out");
        PICOTEST_CHECK(test_cond_wait_done, "core1 isn't done");
        multicore_reset_core1();
    PICOTEST_END_SECTION();

    PICOTEST_START_SECTION("test cond wait with timeout and signal");
        test_cond_wait_ready = false;
        test_cond_wait_done = false;
        test_cond_wait_timedout = false;
        multicore_launch_core1(test_cond_wait_timeout);
        busy_wait_ms(200);
        PICOTEST_CHECK(test_cond_wait_ready, "core1 is not ready");
        PICOTEST_CHECK(!test_cond_wait_done, "core1 did not wait");
        mutex_enter_blocking(&mutex);
        cond_signal(&cond);
        mutex_exit(&mutex);
        busy_wait_ms(200);
        PICOTEST_CHECK(test_cond_wait_timedout, "core1 did time out");
        PICOTEST_CHECK(test_cond_wait_done, "core1 isn't done");
        multicore_reset_core1();
    PICOTEST_END_SECTION();

    PICOTEST_END_TEST();
}
