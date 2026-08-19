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
    AGENT_ARM_PROBE,          /* diagnostic: add an alarm and read the hardware straight
                               * back, to tell "the add never armed it" from "something
                               * disarmed it afterwards" */
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
    AGENT_STOLEN_WAKEUP,              /* repeated waits on ONE future deadline while another
                                       * party's earlier alarm fires; arg = deadline ms.
                                       * pico-sdk#3124 */
    AGENT_POLL_DEADLINE,              /* undisturbed polling on one fixed deadline; arg = ms.
                                       * pico-sdk#3039 */
    AGENT_CANCEL_BURST,               /* add arg alarms and cancel them all as fast as possible,
                                       * so that several are marked before the pool's core gets
                                       * to scan them */
};

/* How many alarms AGENT_CANCEL_BURST adds and cancels per round, and how many rounds D2.15
 * runs. The burst has to be at least two for the pool to see a batch at all, and small enough
 * that several rounds fit in the default pool if entries are being lost. */
#define CANCEL_BURST_ALARMS 6
/* Enough rounds that a leak has to exhaust the pool well before the last one: losing the whole
 * burst but one per round, a 16 entry pool runs out on round 3. */
#define CANCEL_BURST_ROUNDS 6
/* Far enough out that the burst is cancelled long before any of it is due, near enough that it
 * sorts to the front of the pool's list - the case needs the head itself to be cancelled. */
#define CANCEL_BURST_MS 100

/* How many wait-loop iterations still count as "it really blocked". pico_sync_test uses 5;
 * 1-5 is normal depending on platform and core, a busy-poll runs to thousands. */
#define MAX_BLOCKING_WAITS 8

/* AGENT_STOLEN_WAKEUP queues an alarm this many times its deadline out, so that the pool has
 * somewhere later to re-arm to once the earlier alarm has fired. Large enough that an
 * oversleep to it cannot be mistaken for jitter, small enough to keep the case short. */
#define STOLEN_FAR_MULTIPLE 20
/* Tolerance on meeting the deadline. Generous: the failure it must not swallow is an
 * oversleep by nineteen further deadlines, so precision buys nothing here. */
#define STOLEN_SLACK_US 2000
/*
 * D2.11's own wait-iteration ceiling. It cannot use MAX_BLOCKING_WAITS: that is calibrated for
 * a wait with no other alarm traffic, whereas this case deliberately creates some, so its floor
 * is structurally higher (observed 7 with hardware spin locks, 10 with software ones, where an
 * add costs an extra non-sleeping pass). Set far above both, because the failure it has to
 * catch is not marginal - D1.9 busy-polls at ~5e4 iterations in the same run. A ceiling tight
 * enough to argue about is a ceiling measuring the wrong thing.
 */
#define STOLEN_MAX_WAITS 100

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
/* AGENT_STOLEN_WAKEUP / AGENT_POLL_DEADLINE: how many times the wait loop went round. The failure is few
 * iterations and a long elapsed, so the two together separate "parked past the deadline"
 * from "spun to it". */
uint32_t agent_poll_iterations(void);
/* AGENT_CANCEL_BURST: how many alarms the agent actually managed to add. */
uint32_t agent_burst_added(void);
/* False if add_alarm_at() failed to queue the far alarm, i.e. the precondition that
 * something is queued *beyond* the deadline was never established. */
bool     agent_stolen_far_armed(void);

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
/* AGENT_ARM_PROBE results: the hardware read back immediately after a successful add. */
int32_t  agent_probe_id(void);           /* what add_alarm_in_us() returned */
uint32_t agent_probe_armed(void);
uint32_t agent_probe_alarm(void);
uint32_t agent_probe_now(void);
/* INTF is what ta_force_irq() sets; INTS is what the NVIC actually sees ("status after
 * masking & forcing"). Sampled immediately after the add and again 200us later: still
 * asserted means the core is not taking an enabled interrupt; cleared with nothing armed
 * means something serviced it without doing the pool's work. */
uint32_t agent_probe_primask(void);      /* 0xffffffff = not applicable on this arch */
uint32_t agent_probe_basepri(void);
uint32_t agent_probe_intf(void);
uint32_t agent_probe_ints(void);
uint32_t agent_probe_intf_late(void);
uint32_t agent_probe_ints_late(void);
uint32_t agent_probe_armed_late(void);
/* Have the agent core calibrate its own cycle counter and install the result. */
void     plat_calibrate_agent_cycles(void);
#endif

/* ---- spinner (scheduler only) ------------------------------------------------------ */

/* Start a background task looping sleep_ms(period_ms) forever, recording its worst
 * oversleep. FreeRTOS only - the Q2 bug needs more than one concurrent sleeper. */
/* D3.4: hammer a mutex bound to `spin_lock_num` from two tasks, so its waiters keep
 * consuming that spin lock's event-group bit. No-op without a scheduler. */
/* D1.10: n tasks each blocking in sem_acquire_blocking(&test_sem), counting completions.
 * Needs a scheduler - two waiters must be blocked on the same event-group bit. */
void     plat_start_sem_acquirers(uint n);
/* Add one more acquirer without resetting the completion count - used to introduce a second
 * waiter at a controlled moment relative to the releases. */
void     plat_start_one_sem_acquirer(void);
/* As above, but the task runs `hook` immediately before it acquires. Lets a probe time
 * something relative to *this* waiter's own execution rather than to the caller's - under SMP
 * the new task may start on the other core before the caller has finished setting up. */
void     plat_start_one_sem_acquirer_with_hook(void (*hook)(void));
uint     plat_sem_acquirers_done(void);
/* Bit per acquirer in creation order, so a failure can name which one did not finish. */
uint     plat_sem_acquirers_mask(void);
/* True if any acquirer task could not be created - which otherwise looks exactly like a
 * waiter that was stranded, and would be reported as the bug under test. */
bool     plat_sem_acquirer_create_failed(void);
uint32_t plat_free_heap(void);

/* D1.10 churn: n consumers looping on test_sem, each counting its own acquires. */
void     plat_sem_churn_start(uint n);
uint32_t plat_sem_churn_count(uint i);
bool     plat_sem_churn_stop(uint n);
/* True only if that consumer is genuinely blocked, not merely waiting its turn. */
bool     plat_sem_churn_blocked(uint i);

void     plat_start_bit_thief(uint spin_lock_num);
void     plat_stop_bit_thief(void);

void     plat_start_background_sleeper(uint32_t period_ms, volatile int64_t *worst_late_us);

void     plat_spinner_start(void);
uint32_t plat_spinner_count(void);

#endif
