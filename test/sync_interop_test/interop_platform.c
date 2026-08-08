/*
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/time.h"

#include "interop_platform.h"
#include "interop_harness.h"

#if INTEROP_HAVE_FREERTOS
#include "semphr.h"
#endif

mutex_t           test_mutex;
recursive_mutex_t test_rmutex;
semaphore_t       test_sem;

/* ==================================================================================== */
/* bare-SDK core agent                                                                  */
/* ==================================================================================== */

/*
 * Faithful copies of mutex_enter_blocking() and mutex_enter_block_until() from
 * src/common/pico_sync/mutex.c, with the wait iterations counted.
 *
 * Counting is the only reliable way to tell sleeping from spinning here. On Armv8-M
 * DWT_CYCCNT keeps counting straight through WFE - that is exactly what DWT_SLEEPCNT is
 * for, and the RP2350 M33 does not implement it - so a cycle-based occupancy figure cannot
 * distinguish the two. A wait that really blocks takes 1-5 iterations; a busy-poll runs to
 * thousands.
 *
 * NOTE: keep these in step with mutex.c, as pico_sync_test.c already has to.
 */
static uint32_t inline_wait_sleep_delta;
uint32_t last_inline_wait_sleep_delta(void) { return inline_wait_sleep_delta; }

uint32_t counted_mutex_enter_blocking(mutex_t *mtx) {
    uint32_t waits = 0;
    inline_wait_sleep_delta = 0;
    lock_owner_id_t caller = lock_get_caller_owner_id();
    do {
        uint32_t save = spin_lock_blocking(mtx->core.spin_lock);
        if (!lock_is_owner_id_valid(mtx->owner)) {
            mtx->owner = caller;
            spin_unlock(mtx->core.spin_lock, save);
            break;
        }
        waits++;
        uint32_t sc0 = harness_sleep_counter();
        lock_internal_spin_unlock_with_wait(&mtx->core, save);
        inline_wait_sleep_delta += harness_sleep_delta(sc0, harness_sleep_counter());
    } while (true);
    return waits;
}

uint32_t bare_mutex_enter_blocking(mutex_t *mtx) {
    uint32_t waits = 0;
    inline_wait_sleep_delta = 0;
    lock_owner_id_t caller = lock_get_caller_owner_id();
    do {
        uint32_t save = spin_lock_blocking(mtx->core.spin_lock);
        if (!lock_is_owner_id_valid(mtx->owner)) {
            mtx->owner = caller;
            spin_unlock(mtx->core.spin_lock, save);
            break;
        }
        waits++;
        /* deliberately NOT lock_internal_spin_unlock_with_wait() */
        uint32_t sc0 = harness_sleep_counter();
        spin_unlock(mtx->core.spin_lock, save);
        __wfe();
        inline_wait_sleep_delta += harness_sleep_delta(sc0, harness_sleep_counter());
    } while (true);
    return waits;
}

uint32_t counted_mutex_enter_block_until(mutex_t *mtx, absolute_time_t until,
                                                bool *acquired) {
    uint32_t waits = 0;
    lock_owner_id_t caller = lock_get_caller_owner_id();
    *acquired = false;
    do {
        uint32_t save = spin_lock_blocking(mtx->core.spin_lock);
        if (!lock_is_owner_id_valid(mtx->owner)) {
            mtx->owner = caller;
            spin_unlock(mtx->core.spin_lock, save);
            *acquired = true;
            return waits;
        }
        waits++;
        if (lock_internal_spin_unlock_with_best_effort_wait_or_timeout(&mtx->core, save, until)) {
            return waits;   /* timed out */
        }
    } while (true);
}

#if INTEROP_HAS_SDK_CORE

static struct {
    volatile uint32_t cmd;
    volatile uint32_t arg;
    volatile bool     done;
    volatile bool     held;        /* AGENT_HOLD_MS: the lock is now held */
    volatile bool     bool_result;
    volatile uint32_t elapsed_us;
    volatile uint32_t cycle_delta;
    volatile uint32_t wait_count;
    volatile bool     slept;
    volatile uint32_t sleep_delta;
    volatile uint32_t inline_sleep_delta;
    volatile uint32_t cal_busy_per_us;
    volatile uint32_t cal_sleep_per_us;
} agent;

static int64_t agent_known_sleep_alarm(__unused alarm_id_t id, __unused void *ud) {
    __sev();
    return 0;
}

static void sdk_core_agent(void) {
    harness_cycles_enable_this_core();
    for (;;) {
        while (!agent.cmd) {
            tight_loop_contents();
        }
        uint32_t cmd = agent.cmd;
        uint32_t arg = agent.arg;
        bool result = false;

        absolute_time_t t0 = get_absolute_time();
        uint32_t c0 = harness_cycles();
        uint32_t sc0 = harness_sleep_counter();
        switch (cmd) {
            case AGENT_MUTEX_ENTER_BLOCKING:
                mutex_enter_blocking(&test_mutex);
                result = true;
                break;
            case AGENT_MUTEX_EXIT:
                mutex_exit(&test_mutex);
                result = true;
                break;
            case AGENT_MUTEX_TRY_ENTER_BLOCK_UNTIL:
                result = mutex_try_enter_block_until(&test_mutex, make_timeout_time_ms(arg));
                /* release here: mutex_exit() asserts on an unowned mutex, so ownership must
                 * not outlive the command that took it */
                if (result) mutex_exit(&test_mutex);
                break;
            case AGENT_HOLD_MS:
                mutex_enter_blocking(&test_mutex);
                agent.held = true;
                /* busy_wait, not sleep: sleep_* is built on the machinery under test */
                busy_wait_us(arg * 1000ull);
                mutex_exit(&test_mutex);
                result = true;
                break;
            case AGENT_SEM_ACQUIRE_BLOCKING:
                sem_acquire_blocking(&test_sem);
                result = true;
                break;
            case AGENT_SEM_RELEASE:
                result = sem_release(&test_sem);
                break;
            case AGENT_EXPIRED_DEADLINE: {
                /* Mirrors pico_sync_test's "check repeated deadline issue": run one wait to
                 * completion so the deadline is cached in best_effort_wfe_or_timeout()'s
                 * last_added and its alarm has fired, then call again on the now-expired
                 * deadline. Without the fix that short-circuits to a bare __wfe() with no
                 * event coming, and hangs. Run here rather than locally so agent_wait() can
                 * bound it and report instead of wedging the run. */
                absolute_time_t deadline = make_timeout_time_ms(arg);
                while (!best_effort_wfe_or_timeout(deadline)) {
                    tight_loop_contents();
                }
                for (uint i = 0; i < 5; i++) {
                    best_effort_wfe_or_timeout(deadline);
                }
                /* and the variant seen in the field: a timed acquire on an expired deadline */
                semaphore_t s;
                sem_init(&s, 0, 1);
                result = !sem_acquire_block_until(&s, deadline);
                break;
            }
            case AGENT_KNOWN_SLEEP: {
                /* Exactly what harness_calibrate_cycles_local() does, but reported through
                 * the normal command-boundary instrumentation as well. If the inline delta
                 * is non-zero and the boundary delta is zero for the *same* WFE, the bug is
                 * in the boundary read, not in DWT_SLEEPCNT. */
                add_alarm_in_us(arg * 1000ull, agent_known_sleep_alarm, NULL, true);
                __sev();
                __wfe();                      /* drain */
                uint32_t i0 = harness_sleep_counter();
                __wfe();                      /* the real sleep */
                agent.inline_sleep_delta = harness_sleep_delta(i0, harness_sleep_counter());
                result = true;
                break;
            }
            case AGENT_SLEEP_MS:
                sleep_ms(arg);
                result = true;
                break;
            case AGENT_MUTEX_ENTER_BARE:
                agent.wait_count = bare_mutex_enter_blocking(&test_mutex);
                agent.inline_sleep_delta = last_inline_wait_sleep_delta();
                result = true;
                break;
            case AGENT_MUTEX_ENTER_COUNTED:
                agent.wait_count = counted_mutex_enter_blocking(&test_mutex);
                agent.inline_sleep_delta = last_inline_wait_sleep_delta();
                result = true;
                break;
            case AGENT_MUTEX_TIMED_COUNTED:
                agent.wait_count = counted_mutex_enter_block_until(
                        &test_mutex, make_timeout_time_ms(arg), &result);
                if (result) mutex_exit(&test_mutex);   /* as above */
                break;
            case AGENT_CALIBRATE_CYCLES: {
                uint32_t busy, slp;
                harness_calibrate_cycles_local(&busy, &slp);
                agent.cal_busy_per_us = busy;
                agent.cal_sleep_per_us = slp;
                result = true;
                break;
            }
            default:
                break;
        }
        /* changed => this core definitely slept during the command (it cannot move
         * otherwise); unchanged is inconclusive, so only ever used as corroboration */
        agent.sleep_delta = harness_sleep_delta(sc0, harness_sleep_counter());
        agent.slept = agent.sleep_delta != 0;
        agent.cycle_delta = harness_cycle_delta(c0, harness_cycles());
        agent.elapsed_us = (uint32_t)absolute_time_diff_us(t0, get_absolute_time());
        agent.bool_result = result;
        agent.cmd = AGENT_IDLE;
        agent.done = true;
        __sev();
    }
}

void agent_start(uint32_t cmd, uint32_t arg) {
    /* never overwrite an in-flight command - see the note on plat_sdk_core_hold_for_ms() */
    absolute_time_t idle_by = make_timeout_time_ms(5000);
    while (agent.cmd != AGENT_IDLE && !time_reached(idle_by)) {
        tight_loop_contents();
    }
    assert(agent.cmd == AGENT_IDLE);
    agent.done = false;
    agent.held = false;
    agent.arg = arg;
    __compiler_memory_barrier();
    agent.cmd = cmd;
    __sev();
}

bool agent_wait(uint32_t timeout_ms) {
    absolute_time_t until = make_timeout_time_ms(timeout_ms);
    while (!agent.done) {
        if (time_reached(until)) return false;
        plat_delay_ms(1);
    }
    return true;
}

bool agent_run(uint32_t cmd, uint32_t arg, uint32_t timeout_ms) {
    agent_start(cmd, arg);
    return agent_wait(timeout_ms);
}

void plat_calibrate_agent_cycles(void) {
    if (agent_run(AGENT_CALIBRATE_CYCLES, 0, 2000)) {
        harness_set_cycle_calibration(agent.cal_busy_per_us, agent.cal_sleep_per_us);
    }
}

/* Note this returns while the command is still running - the agent is busy holding until
 * `ms` has elapsed - so no other agent command may be issued until it completes. */
void plat_sdk_core_hold_for_ms(uint32_t ms) {
    agent_start(AGENT_HOLD_MS, ms);
    absolute_time_t until = make_timeout_time_ms(ms + 1000);
    while (!agent.held && !time_reached(until)) {
        tight_loop_contents();
    }
}

bool     agent_result(void)     { return agent.bool_result; }
uint32_t agent_elapsed_us(void) { return agent.elapsed_us; }
uint32_t agent_cycles(void)     { return agent.cycle_delta; }
uint32_t agent_wait_count(void) { return agent.wait_count; }
bool     agent_slept(void)      { return agent.slept; }
uint32_t agent_sleep_delta(void) { return agent.sleep_delta; }
uint32_t agent_inline_sleep_delta(void) { return agent.inline_sleep_delta; }

#endif /* INTEROP_HAS_SDK_CORE */

/* ==================================================================================== */
/* FreeRTOS implementation                                                              */
/* ==================================================================================== */

#if INTEROP_HAVE_FREERTOS

#define MAIN_PRIORITY   ( tskIDLE_PRIORITY + 3UL )
#define HOLDER_PRIORITY ( tskIDLE_PRIORITY + 2UL )

static SemaphoreHandle_t go_sem;
static SemaphoreHandle_t held_sem;
static volatile uint32_t hold_ms;
static volatile uint32_t spinner_counter;
static void (*main_body)(void);

static void holder_task(__unused void *params) {
    for (;;) {
        xSemaphoreTake(go_sem, portMAX_DELAY);
        mutex_enter_blocking(&test_mutex);
        xSemaphoreGive(held_sem);
        vTaskDelay(pdMS_TO_TICKS(hold_ms));
        mutex_exit(&test_mutex);
    }
}

static void spinner_task(__unused void *params) {
    for (;;) {
        spinner_counter++;
    }
}

static void main_task(__unused void *params) {
    main_body();
    for (;;) {
        vTaskDelay(portMAX_DELAY);
    }
}

void plat_delay_ms(uint32_t ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void plat_hold_for_ms(uint32_t ms) {
    hold_ms = ms;
    xSemaphoreGive(go_sem);
    xSemaphoreTake(held_sem, portMAX_DELAY);
}

typedef struct { uint32_t period_ms; volatile int64_t *worst; } sleeper_arg_t;
static sleeper_arg_t sleeper_args[4];
static uint sleeper_count;

static void sleeper_task(void *params) {
    sleeper_arg_t *a = (sleeper_arg_t *)params;
    for (;;) {
        absolute_time_t target = make_timeout_time_ms(a->period_ms);
        sleep_ms(a->period_ms);
        int64_t late = absolute_time_diff_us(target, get_absolute_time());
        if (late > *a->worst) *a->worst = late;
    }
}

void plat_start_background_sleeper(uint32_t period_ms, volatile int64_t *worst_late_us) {
    if (sleeper_count >= count_of(sleeper_args)) return;
    sleeper_arg_t *a = &sleeper_args[sleeper_count++];
    a->period_ms = period_ms;
    a->worst = worst_late_us;
    xTaskCreate(sleeper_task, "sleeper", configMINIMAL_STACK_SIZE, a, HOLDER_PRIORITY, NULL);
}

void plat_spinner_start(void) {
    TaskHandle_t t;
    xTaskCreate(spinner_task, "spin", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, &t);
#if configNUMBER_OF_CORES > 1 && configUSE_CORE_AFFINITY
    vTaskCoreAffinitySet(t, 1u << 0);   /* pin to the core the tests run on */
#endif
}

uint32_t plat_spinner_count(void) {
    return spinner_counter;
}

const char *plat_config_name(void) {
#if configNUMBER_OF_CORES > 1
    return "FreeRTOS SMP on both cores (no bare-SDK core)";
#elif RUN_FREE_RTOS_ON_CORE == 1
    return "FreeRTOS on core 1, bare SDK code on core 0";
#else
    return "FreeRTOS on core 0, bare SDK code on core 1";
#endif
}

static void start_scheduler(void) {
    xTaskCreate(main_task, "main", configMINIMAL_STACK_SIZE * 2, NULL, MAIN_PRIORITY, NULL);
    xTaskCreate(holder_task, "holder", configMINIMAL_STACK_SIZE, NULL, HOLDER_PRIORITY, NULL);
    vTaskStartScheduler();
}

void plat_init(void) {
    mutex_init(&test_mutex);
    recursive_mutex_init(&test_rmutex);
    sem_init(&test_sem, 0, 16);
    go_sem = xSemaphoreCreateBinary();
    held_sem = xSemaphoreCreateBinary();
}

void plat_run(void (*body)(void)) {
    main_body = body;
#if INTEROP_HAS_SDK_CORE
#if RUN_FREE_RTOS_ON_CORE == 1
    multicore_launch_core1(start_scheduler);
    sdk_core_agent();       /* core 0 becomes the bare-SDK agent */
#else
    multicore_launch_core1(sdk_core_agent);
    start_scheduler();
#endif
#else
    start_scheduler();
#endif
    for (;;) {
        tight_loop_contents();
    }
}

/* ==================================================================================== */
/* baseline (no RTOS) implementation                                                    */
/* ==================================================================================== */

#else

void plat_delay_ms(uint32_t ms) {
    /* deliberately not sleep_ms: that is built on the alarm/WFE machinery under test */
    busy_wait_us(ms * 1000ull);
}

void plat_hold_for_ms(uint32_t ms) {
    /* no tasks here, so the holder is always the other core */
    plat_sdk_core_hold_for_ms(ms);
}

void plat_start_background_sleeper(__unused uint32_t period_ms,
                                   __unused volatile int64_t *worst_late_us) {
    /* no tasks without a scheduler */
}

void plat_spinner_start(void) {
    /* no scheduler: nothing to yield to, so the yield metric does not apply */
}

uint32_t plat_spinner_count(void) {
    return 0;
}

const char *plat_config_name(void) {
    return "baseline, no RTOS (SDK lock_core macros, core 0 drives, core 1 agents)";
}

void plat_init(void) {
    mutex_init(&test_mutex);
    recursive_mutex_init(&test_rmutex);
    sem_init(&test_sem, 0, 16);
}

void plat_run(void (*body)(void)) {
    multicore_launch_core1(sdk_core_agent);
    body();
    for (;;) {
        tight_loop_contents();
    }
}

#endif
