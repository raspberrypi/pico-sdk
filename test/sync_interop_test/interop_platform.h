/*
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * Thin platform shim so the D1/D2 case bodies are single-source across:
 *
 *   baseline  - no RTOS at all. core 0 drives, core 1 holds/agents. This is the reference:
 *               it exercises the SDK's own lock_core macros, so a failure here is an SDK
 *               bug, and anything that only fails under FreeRTOS is an interop bug.
 *   freertos  - the port's lock_internal_* overrides are in play.
 *
 * Note on delays: the baseline uses busy_wait_*, never sleep_*, because sleep_* is built on
 * the alarm pool and WFE machinery that these tests are measuring. Under FreeRTOS the delay
 * is vTaskDelay, which is what we want there - it yields, so a blocked task's yield can be
 * observed.
 */

#ifndef _INTEROP_PLATFORM_H
#define _INTEROP_PLATFORM_H

#include "pico.h"
#include "pico/sync.h"

#ifndef INTEROP_HAVE_FREERTOS
#define INTEROP_HAVE_FREERTOS 0
#endif

#if INTEROP_HAVE_FREERTOS
#include "FreeRTOS.h"
#include "task.h"
/* a scheduler exists, so "did the waiter yield the CPU" is a meaningful question */
#define INTEROP_HAS_SCHEDULER 1
/* in SMP both cores are FreeRTOS cores, so there is no bare-SDK core to test against */
#if configNUMBER_OF_CORES == 1
#define INTEROP_HAS_SDK_CORE 1
#else
#define INTEROP_HAS_SDK_CORE 0
#endif
#else
#define INTEROP_HAS_SCHEDULER 0
#define INTEROP_HAS_SDK_CORE 1
#endif

#ifndef RUN_FREE_RTOS_ON_CORE
#define RUN_FREE_RTOS_ON_CORE 0
#endif

/*
 * Which core runs the test cases in the baseline (no-RTOS) build; the other one agents.
 * Reversing it is the control for anything that looks core-specific: without it, "core 0 is
 * the agent" and "FreeRTOS is on core 1" only ever occur together, so a failure cannot be
 * attributed to either.
 */
#ifndef INTEROP_TESTS_ON_CORE
#define INTEROP_TESTS_ON_CORE 0
#endif

/* the objects under test, shared with the second core */
extern mutex_t           test_mutex;
extern recursive_mutex_t test_rmutex;
extern semaphore_t       test_sem;

/* ---- lifecycle -------------------------------------------------------------------- */

/* Initialise the primitives and start the second core. Call from main(). */
void plat_init(void);
/* Run `body` as the main test context (a task under FreeRTOS) and never return. */
void plat_run(void (*body)(void)) __attribute__((noreturn));
/* Describe the configuration, for the test banner. */
const char *plat_config_name(void);

/* ---- delays ----------------------------------------------------------------------- */

/* vTaskDelay under FreeRTOS (yields); busy_wait_ms on the baseline (never sleep_ms, which
 * is built on the machinery under test). */
void plat_delay_ms(uint32_t ms);

/* ---- contention ------------------------------------------------------------------- */

/*
 * Arrange for test_mutex to be held for `ms`, and return only once it is actually held.
 * Under FreeRTOS this is a holder task; on the baseline it is core 1.
 */
void plat_hold_for_ms(uint32_t ms);

/* ---- counted copies of the SDK primitives ------------------------------------------
 * Faithful copies of mutex_enter_blocking() / mutex_enter_block_until() with the wait
 * iterations counted. Counting is the only reliable way to tell sleeping from spinning:
 * a timeout that fires exactly on time looks the same either way. Available locally as
 * well as via the agent, so the driving core's waits can be judged too.
 * NOTE: keep in step with src/common/pico_sync/mutex.c.
 */
uint32_t counted_mutex_enter_blocking(mutex_t *mtx);
uint32_t counted_mutex_enter_block_until(mutex_t *mtx, absolute_time_t until, bool *acquired);

/*
 * The same loop, but using the BARE `spin_unlock(); __wfe()` pattern instead of
 * lock_internal_spin_unlock_with_wait(). This is what the FreeRTOS ports hard-code on their
 * !portIS_FREE_RTOS_CORE() branch, so it is worth testing as itself rather than only where
 * the SDK macro happens to resolve to it. It must busy-poll exactly when a spin unlock sets
 * the calling core's own event, i.e. when PICO_SPIN_LOCK_UNLOCK_CAUSES_SEV.
 */
uint32_t bare_mutex_enter_blocking(mutex_t *mtx);

/* Accumulated DWT_SLEEPCNT delta measured *inline* around the wait inside the last
 * counted_/bare_mutex_enter_blocking() call. Compare against the command-boundary delta:
 * they measure the same sleep, so a difference localises the discrepancy. */
uint32_t last_inline_wait_sleep_delta(void);

/* ---- bare-SDK core agent ------------------------------------------------------------
 * Available whenever INTEROP_HAS_SDK_CORE. Runs the requested operation on a core that is
 * not under RTOS control, and reports how long it took and how many cycles it burned.
 */

enum {
    AGENT_IDLE = 0,
    AGENT_MUTEX_ENTER_BLOCKING,
    AGENT_MUTEX_EXIT,
    AGENT_MUTEX_TRY_ENTER_BLOCK_UNTIL,  /* arg = timeout ms */
    AGENT_HOLD_MS,                      /* arg = hold duration ms */
    AGENT_SEM_ACQUIRE_BLOCKING,
    AGENT_SEM_RELEASE,
    AGENT_CALIBRATE_CYCLES,   /* measure this core's busy/asleep cycle rates */
    AGENT_IRQ_STATE,          /* diagnostic: is the default pool's alarm IRQ live on this
                               * core? Read from the core itself - the NVIC enable is per
                               * core, so no other core can answer this question. */
    AGENT_MUTEX_ENTER_COUNTED,        /* mutex_enter_blocking, counting wait iterations */
    AGENT_MUTEX_TIMED_COUNTED,        /* mutex_enter_block_until, ditto; arg = timeout ms */
    AGENT_MUTEX_ENTER_BARE,           /* bare spin_unlock(); __wfe() loop, counted */
    AGENT_SLEEP_MS,                   /* sleep_ms(arg) on the bare-SDK core */
    AGENT_EXPIRED_DEADLINE,           /* repeated waits on an already-expired deadline */
    AGENT_KNOWN_SLEEP,                /* diagnostic: a bare known WFE, measured BOTH inline
                                       * and at the command boundary, to tell an
                                       * instrumentation bug from a hardware one */
};

/* How many wait-loop iterations still count as "it really blocked". pico_sync_test uses 5;
 * 1-5 is normal depending on platform and core, a busy-poll runs to thousands. */
#define MAX_BLOCKING_WAITS 8

#if INTEROP_HAS_SDK_CORE
/* Have the bare-SDK core take test_mutex and hold it for `ms`; returns once it is held. */
void     plat_sdk_core_hold_for_ms(uint32_t ms);
void     agent_start(uint32_t cmd, uint32_t arg);
bool     agent_wait(uint32_t timeout_ms);
bool     agent_run(uint32_t cmd, uint32_t arg, uint32_t timeout_ms);
bool     agent_result(void);
uint32_t agent_elapsed_us(void);
uint32_t agent_cycles(void);
/* wait-loop iterations from the last AGENT_*_COUNTED command */
uint32_t agent_wait_count(void);
/* Did DWT_SLEEPCNT move during the last command, i.e. did that core really sleep? */
bool     agent_slept(void);
uint32_t agent_sleep_delta(void);
/* AGENT_KNOWN_SLEEP: the delta measured *inline*, immediately around the WFE */
uint32_t agent_inline_sleep_delta(void);

/* AGENT_IRQ_STATE results, valid after a successful AGENT_IRQ_STATE command. */
uint32_t agent_alarm_irq_num(void);      /* the IRQ the default pool's alarm uses */
bool     agent_alarm_irq_enabled(void);  /* enabled in *that core's* NVIC */
/* The timer peripheral's own view, read on the agent core. The NVIC enable says only that the
 * core would take the interrupt; these say whether the timer is set up to raise it at all. */
uint32_t agent_timer_inte(void);         /* per-alarm interrupt enable */
uint32_t agent_timer_intr(void);         /* raw (latched) interrupt status */
uint32_t agent_timer_armed(void);        /* which alarms are armed */
uint32_t agent_timer_alarm_val(void);    /* the alarm's compare value */
uint32_t agent_timer_now(void);          /* timerawl at the same moment */
/* Have the agent core calibrate its own cycle counter and install the result. */
void     plat_calibrate_agent_cycles(void);
#endif

/* ---- spinner (scheduler only) ------------------------------------------------------ */

/* Start a background task looping sleep_ms(period_ms) forever, recording its worst
 * oversleep. FreeRTOS only - the Q2 bug needs more than one concurrent sleeper. */
void     plat_start_background_sleeper(uint32_t period_ms, volatile int64_t *worst_late_us);

void     plat_spinner_start(void);
uint32_t plat_spinner_count(void);

#endif
