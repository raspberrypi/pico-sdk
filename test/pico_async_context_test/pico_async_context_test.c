/**
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/async_context_threadsafe_background.h"
#include "pico/test.h"

PICOTEST_MODULE_NAME("pico_async_context_test", "pico_async_context_test test harness");

static void at_time_worker_fn(async_context_t *context, async_at_time_worker_t *worker) {
    worker->user_data = (void*)true;
}
static async_at_time_worker_t at_time_worker = { .do_work = at_time_worker_fn };

static void pending_worker_fn(async_context_t *async_context, async_when_pending_worker_t *worker) {
    worker->user_data = (void*)true;
    async_context_remove_at_time_worker(async_context, &at_time_worker); // remove at time worker
}
static async_when_pending_worker_t pending_worker = { .do_work = pending_worker_fn };

static int issue_2836_test(void) {

    async_context_threadsafe_background_t async_context;

    // Set everything up
    hard_assert(async_context_threadsafe_background_init_with_defaults(&async_context));
    async_context_add_when_pending_worker(&async_context.core, &pending_worker);

    // set an "at time" worker to run a long time in the future
    hard_assert(!at_time_worker.user_data);
    async_context_add_at_time_worker_at(&async_context.core, &at_time_worker, make_timeout_time_ms(100)); // .1s

    // run a "pending" now, this will remove the "at time" worker we just setup above
    hard_assert(!pending_worker.user_data);
    async_context_set_work_pending(&async_context.core, &pending_worker);
    busy_wait_us_32(500); // Pending worker should run pretty quickly
    hard_assert(pending_worker.user_data);
    hard_assert(!at_time_worker.user_data);

    // Setup the "at time" worker again in .1s
    async_context_add_at_time_worker_at(&async_context.core, &at_time_worker, make_timeout_time_ms(100)); // .1s

    // Wait long enough that the "at time" worker to happen
    busy_wait_us_32(200000); // .2s
    if (!at_time_worker.user_data) { // this should be set by the "at time" worker, it's a bug if it doesn't!
        panic("test fail: async at time worker did not run");
    }
    return 0;
}

int main() {
    setup_default_uart();

    PICOTEST_START();

    PICOTEST_START_SECTION("Issue #2836 defect - async at time worker problem");
    issue_2836_test();
    PICOTEST_END_SECTION();

    PICOTEST_END_TEST();
}