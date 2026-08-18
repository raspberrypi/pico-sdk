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
    volatile uint32_t irq_num;
    volatile bool     irq_enabled;
    volatile uint32_t t_inte, t_intr, t_armed, t_alarm, t_now;
    volatile int32_t  probe_id;
    volatile uint32_t probe_armed, probe_alarm, probe_now;
    volatile uint32_t probe_intf, probe_ints;
    volatile uint32_t probe_primask, probe_basepri;
    volatile uint32_t probe_intf_late, probe_ints_late, probe_armed_late;
    volatile uint32_t cal_dwt_ctrl;
    volatile uint32_t cal_demcr;
    volatile bool     cal_sleepcnt_moved;
    volatile bool     cal_sleepcnt_awake;
    volatile uint32_t poll_iterations;
    volatile bool     stolen_far_armed;
} agent;

static int64_t agent_stolen_far_alarm(__unused alarm_id_t id, __unused void *ud) {
    return 0;   /* exists only to be queued beyond the deadline, and as the backstop */
}

static volatile bool agent_known_sleep_fired;

static int64_t agent_known_sleep_alarm(__unused alarm_id_t id, __unused void *ud) {
    agent_known_sleep_fired = true;
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
                /* Mirrors pico_sync_test's "check repeated deadline issue": wait one deadline
                 * out, then wait on it again now that it is in the past. Run here rather than
                 * locally so agent_wait() can bound it and report instead of wedging the
                 * run. */
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
            case AGENT_POLL_DEADLINE: {
                /*
                 * The pico-sdk#3039 shape, with nothing else going on: poll one fixed deadline
                 * from a core that does not own the alarm pool's IRQ. add_alarm_at() is
                 * asynchronous from here - it only forces the IRQ, and the owning core
                 * programs the hardware - so a wait predicate that consults the hardware alone
                 * can keep re-adding an alarm it then cancels, and the loop busy-waits instead
                 * of sleeping.
                 *
                 * The iteration count is the criterion, exactly as for D1/D2: sleeping takes a
                 * couple of passes, spinning takes thousands. Elapsed time alone cannot tell
                 * them apart, since both arrive at the deadline on time.
                 */
                absolute_time_t deadline = make_timeout_time_ms(arg);
                uint32_t iterations = 0;
                while (!best_effort_wfe_or_timeout(deadline)) {
                    iterations++;
                    tight_loop_contents();
                }
                agent.poll_iterations = iterations;
                result = true;
                break;
            }
            case AGENT_STOLEN_WAKEUP: {
                /*
                 * pico-sdk#3124: the wakeup a waiter is relying on, spent by somebody else's
                 * earlier alarm. A waiter may only sleep while a wakeup at or before its
                 * deadline is still due, and the single SEV its alarm was worth can be taken
                 * from it:
                 *
                 *   1. we add at T; the pool arms T
                 *   2. another party adds at T' < T; the pool moves the hardware earlier
                 *   3. T' fires, waking our __wfe() early; the deadline is not reached
                 *   4. we reach the unconditional cancel_alarm(), so OUR entry is gone - and
                 *      because the alarm has just fired, ta_set_timeout()'s "never move later"
                 *      guard no longer holds (time_til_alarm has wrapped huge), so the handler
                 *      re-arms to the next live entry, which may be well past T
                 *   5. nothing is now armed before T, and the next pass must notice that
                 *      rather than assume its earlier add still covers it
                 *
                 * The FAR alarm is the part that is easy to forget: without something queued
                 * beyond T, step 4 leaves the fired register alone - the handler breaks out
                 * before ta_set_timeout() when the pool is empty - and the stale value goes on
                 * working as a ghost.
                 *
                 * The earlier alarm is added by the *test* core while this loop runs, because
                 * it has to arrive after step 1; anything queued beforehand would be visible to
                 * the first pass and the interleaving would never be reached.
                 *
                 * Reported by elapsed rather than by hanging: the far alarm doubles as the
                 * backstop, so a failure oversleeps to it and returns.
                 */
                absolute_time_t t0 = get_absolute_time();
                absolute_time_t deadline = delayed_by_ms(t0, arg);
                alarm_id_t far_id = add_alarm_at(delayed_by_ms(t0, arg * STOLEN_FAR_MULTIPLE),
                                                 agent_stolen_far_alarm, NULL, true);
                agent.stolen_far_armed = (far_id > 0);
                uint32_t iterations = 0;
                while (!best_effort_wfe_or_timeout(deadline)) {
                    iterations++;
                    tight_loop_contents();
                }
                agent.poll_iterations = iterations;
                if (far_id > 0) cancel_alarm(far_id);
                result = true;
                break;
            }
            case AGENT_KNOWN_SLEEP: {
                /* Exactly what harness_calibrate_cycles_local() does, but reported through
                 * the normal command-boundary instrumentation as well. If the inline delta
                 * is non-zero and the boundary delta is zero for the *same* WFE, the bug is
                 * in the boundary read, not in DWT_SLEEPCNT. */
                agent_known_sleep_fired = false;
                add_alarm_in_us(arg * 1000ull, agent_known_sleep_alarm, NULL, true);
                __sev();
                __wfe();                      /* drain */
                uint32_t i0 = harness_sleep_counter();
                /* wait on the flag, not on a bare __wfe(): the add forces the pool IRQ, whose
                 * handler ends in __sev(), and if that lands after the drain a single __wfe()
                 * returns at once and the core never sleeps. A spurious wake then costs an
                 * iteration rather than reading d=0 and failing. */
                while (!agent_known_sleep_fired) {
                    __wfe();
                }
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
            case AGENT_ARM_PROBE: {
                /* Add on this core - the pool's own core - and read the compare register
                 * back at once. If armed is set here the add does arm and something clears
                 * it later; if not, the pool never programmed the hardware. */
                timer_hw_t *tp  = alarm_pool_get_default_timer();
                uint anum       = alarm_pool_timer_alarm_num(alarm_pool_get_default());
                alarm_id_t pid  = add_alarm_in_us(20000, agent_known_sleep_alarm, NULL, true);
                agent.probe_armed = tp->armed;
                agent.probe_alarm = tp->alarm[anum];
                agent.probe_now   = tp->timerawl;
                /* Read the core's own interrupt masks. An enabled, asserted IRQ that is not
                 * taken means this core is masking it: PRIMASK masks everything, BASEPRI
                 * masks at or below its priority value (FreeRTOS's mechanism, and the SDK
                 * puts the timer at PICO_DEFAULT_IRQ_PRIORITY 0x80). */
#if defined(__riscv)
                agent.probe_primask = 0xffffffffu;   /* n/a */
                agent.probe_basepri = 0xffffffffu;
#elif defined(__ARM_ARCH_6M__)
                { uint32_t pm; __asm volatile ("mrs %0, primask" : "=r"(pm));
                  agent.probe_primask = pm; agent.probe_basepri = 0xffffffffu; }
#else
                { uint32_t pm, bp;
                  __asm volatile ("mrs %0, primask" : "=r"(pm));
                  __asm volatile ("mrs %0, basepri" : "=r"(bp));
                  agent.probe_primask = pm; agent.probe_basepri = bp; }
#endif
                agent.probe_intf  = tp->intf;
                agent.probe_ints  = tp->ints;
                agent.probe_id    = (int32_t)pid;
                /* Sample again after the interrupt has had ample time to be taken. Nothing
                 * here waits on an event, so this cannot itself be blocked by the fault. */
                busy_wait_us(200);
                agent.probe_intf_late  = tp->intf;
                agent.probe_ints_late  = tp->ints;
                agent.probe_armed_late = tp->armed;
                if (pid > 0) cancel_alarm(pid);
                result = pid > 0;
                break;
            }
            case AGENT_IRQ_STATE: {
                uint alarm_num = alarm_pool_timer_alarm_num(alarm_pool_get_default());
                uint irq = timer_hardware_alarm_get_irq_num(alarm_pool_get_default_timer(),
                                                            alarm_num);
                agent.irq_num     = irq;
                agent.irq_enabled = irq_is_enabled(irq);
                timer_hw_t *t     = alarm_pool_get_default_timer();
                agent.t_inte      = t->inte;
                agent.t_intr      = t->intr;
                agent.t_armed     = t->armed;
                agent.t_alarm     = t->alarm[alarm_num];
                agent.t_now       = t->timerawl;
                result = true;
                break;
            }
            case AGENT_CALIBRATE_CYCLES: {
                /* Measured here, on the agent core, and reported back as a value - the harness
                 * must not learn it from a global written on the wrong core. */
                harness_cal_t cal;
                harness_calibrate_cycles_local(&cal);
                agent.cal_busy_per_us     = cal.busy_per_us;
                agent.cal_sleep_per_us    = cal.sleep_per_us;
                agent.cal_dwt_ctrl        = cal.dwt_ctrl;
                agent.cal_demcr           = cal.demcr;
                agent.cal_sleepcnt_moved  = cal.sleepcnt_moved;
                agent.cal_sleepcnt_awake  = cal.sleepcnt_moved_awake;
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

uint32_t agent_alarm_irq_num(void)     { return agent.irq_num; }
bool     agent_alarm_irq_enabled(void) { return agent.irq_enabled; }
uint32_t agent_timer_inte(void)        { return agent.t_inte; }
uint32_t agent_timer_intr(void)        { return agent.t_intr; }
uint32_t agent_timer_armed(void)       { return agent.t_armed; }
uint32_t agent_timer_alarm_val(void)   { return agent.t_alarm; }
uint32_t agent_timer_now(void)         { return agent.t_now; }
int32_t  agent_probe_id(void)          { return agent.probe_id; }
uint32_t agent_probe_armed(void)       { return agent.probe_armed; }
uint32_t agent_probe_alarm(void)       { return agent.probe_alarm; }
uint32_t agent_probe_now(void)         { return agent.probe_now; }
uint32_t agent_probe_primask(void)     { return agent.probe_primask; }
uint32_t agent_probe_basepri(void)     { return agent.probe_basepri; }
uint32_t agent_probe_intf(void)        { return agent.probe_intf; }
uint32_t agent_probe_ints(void)        { return agent.probe_ints; }
uint32_t agent_probe_intf_late(void)   { return agent.probe_intf_late; }
uint32_t agent_probe_ints_late(void)   { return agent.probe_ints_late; }
uint32_t agent_probe_armed_late(void)  { return agent.probe_armed_late; }

void plat_calibrate_agent_cycles(void) {
    if (agent_run(AGENT_CALIBRATE_CYCLES, 0, 2000)) {
        harness_cal_t cal = {
                .busy_per_us          = agent.cal_busy_per_us,
                .sleep_per_us         = agent.cal_sleep_per_us,
                .dwt_ctrl             = agent.cal_dwt_ctrl,
                .demcr                = agent.cal_demcr,
                .sleepcnt_moved       = agent.cal_sleepcnt_moved,
                .sleepcnt_moved_awake = agent.cal_sleepcnt_awake,
        };
        harness_set_agent_calibration(&cal);
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
uint32_t agent_poll_iterations(void) { return agent.poll_iterations; }
bool     agent_stolen_far_armed(void)  { return agent.stolen_far_armed; }

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
    /*
     * Yields periodically, and must keep doing so.
     *
     * A loop that never yields is a permanently-runnable lower-priority task, which can only
     * be dislodged by preemption - and on the RP2350 RISC-V single-core-scheduler path it
     * never is. Measured 2026-08-08: the tick advanced normally (31 -> 83 over a 50ms busy
     * wait) and portASM.S calls vTaskSwitchContext() when xTaskIncrementTick() reports an
     * unblocked task, yet a task returning from vTaskDelay() never got the core back. Every
     * other task here blocks voluntarily, so the spinner was the only thing exposing it, and
     * it hung the whole run at the first plat_delay_ms().
     *
     * Yielding every 256 iterations keeps the loop overwhelmingly spinning, so the idle and
     * busy calibration poles still separate (measured 16791/ms vs 0/ms). Do not remove the
     * yield to "make the spinner busier" without re-testing that configuration.
     */
    uint32_t i = 0;
    for (;;) {
        spinner_counter++;
        if (!(++i & 0xffu)) taskYIELD();
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

/*
 * Two long-lived acquirers, signalled per round.
 *
 * They used to be created and deleted per round, which quietly turned the D1.10 sweep into a
 * heap-exhaustion test: 320 tasks with configMINIMAL_STACK_SIZE stacks against a 64KB heap,
 * whose stacks are only returned when the idle task runs. Creation began failing part-way
 * through the sweep, and a waiter that was never created looks exactly like one that was
 * stranded - which is what it was reported as, at the same attempt number on two consecutive
 * runs, until xTaskCreate's return value was checked.
 */
#define ACQUIRERS 2
static volatile uint acquirers_done;
static volatile uint acquirers_mask;
static volatile bool acquirer_create_failed;
static SemaphoreHandle_t acq_go[ACQUIRERS];
static void (* volatile acq_hook[ACQUIRERS])(void);
static TaskHandle_t acq_task[ACQUIRERS];

static void acquirer_task(void *param) {
    uint idx = (uint)(uintptr_t)param;
    for (;;) {
        xSemaphoreTake(acq_go[idx], portMAX_DELAY);
        void (*hook)(void) = acq_hook[idx];
        if (hook) hook();
        sem_acquire_blocking(&test_sem);
        acquirers_mask |= 1u << idx;
        acquirers_done++;
    }
}

static void ensure_acquirers(void) {
    if (acq_task[0]) return;
    for (uint i = 0; i < ACQUIRERS; i++) {
        acq_go[i] = xSemaphoreCreateBinary();
        if (!acq_go[i] ||
            xTaskCreate(acquirer_task, "acq", configMINIMAL_STACK_SIZE, (void *)(uintptr_t)i,
                        HOLDER_PRIORITY, &acq_task[i]) != pdPASS) {
            acquirer_create_failed = true;
        }
    }
}

void plat_start_sem_acquirers(uint n) {
    ensure_acquirers();
    acquirers_done = 0;
    acquirers_mask = 0;
    __compiler_memory_barrier();
    for (uint i = 0; i < n && i < ACQUIRERS; i++) {
        acq_hook[i] = NULL;
        xSemaphoreGive(acq_go[i]);
    }
}

void plat_start_one_sem_acquirer(void) {
    ensure_acquirers();
    acq_hook[1] = NULL;
    xSemaphoreGive(acq_go[1]);
}

void plat_start_one_sem_acquirer_with_hook(void (*hook)(void)) {
    ensure_acquirers();
    acq_hook[1] = hook;
    xSemaphoreGive(acq_go[1]);
}

uint plat_sem_acquirers_mask(void) {
    return acquirers_mask;
}

bool plat_sem_acquirer_create_failed(void) {
    return acquirer_create_failed;
}

uint32_t plat_free_heap(void) {
    return (uint32_t)xPortGetFreeHeapSize();
}

uint plat_sem_acquirers_done(void) {
    return acquirers_done;
}

/*
 * Continuous churn, for the D1.10 invariant probe: n consumers looping on the semaphore, each
 * counting its own acquires. Rather than trying to land inside a few-instruction window, this
 * lets the window be hit naturally and watches for its consequence - a consumer making no
 * progress while permits are available.
 */
#define CHURN_MAX 3
static volatile uint32_t churn_count[CHURN_MAX];
static volatile bool     churn_stop;
static volatile uint     churn_exited;
static bool              churn_started;

static TaskHandle_t churn_handle[CHURN_MAX];

static void churn_task_fn(void *param) {
    uint idx = (uint)(uintptr_t)param;
    while (!churn_stop) {
        sem_acquire_blocking(&test_sem);
        churn_count[idx]++;
    }
    churn_exited++;
    vTaskSuspend(NULL);
}

/*
 * Whether this consumer is actually blocked, as opposed to merely losing the race for permits.
 * "Has not progressed while permits are available" is not sufficient on its own: permits
 * arrive in pairs and a sample can land while the consumer is ready and about to run, so a
 * consumer that loses a few round-robin turns looks identical to one that was stranded.
 */
bool plat_sem_churn_blocked(uint i) {
    return i < CHURN_MAX && churn_handle[i] && eTaskGetState(churn_handle[i]) == eBlocked;
}

/*
 * The releaser runs in task context on purpose.
 *
 * Releasing from an alarm ISR goes through xEventGroupSetBitsFromISR, which defers the set to
 * the timer/daemon task rather than applying it inline. That scheduling hop gives a waiter
 * which is still between its spin_unlock() and xEventGroupWaitBits() ample time to arrive and
 * register properly, so it tends to close the very window under test - which is likely why
 * four ISR-driven probe designs never provoked anything. From a task the bits are set inline,
 * and this task runs above the consumers so it can preempt one inside that window.
 *
 * The gap within a pair is swept, since it decides where the second release lands; the gap
 * after a pair is long, because a stranded waiter is rescued by the next release and the stall
 * is only observable during silence.
 */
static void releaser_task_fn(__unused void *param) {
    uint32_t gap_us = 1;
    while (!churn_stop) {
        /*
         * Two phases, because the two requirements pull against each other.
         *
         * A consumer only occupies the vulnerable window in the microseconds just after it
         * consumes a permit and loops round. Releases spaced milliseconds apart therefore
         * always land with every consumer parked, and one xEventGroupSetBits wakes them all
         * together - nobody is ever mid-window, so the interleaving under test never arises.
         * Hence a rapid phase, many pairs close together, so a release lands while another
         * consumer is still cycling.
         *
         * But a stranded waiter is rescued by the next release, so the stall is only visible
         * once the releases stop. Hence the silent phase, during which the invariant is
         * checked: a permit still available with a consumer genuinely blocked.
         */
        for (uint i = 0; i < 10 && !churn_stop; i++) {
            /*
             * To soak this harder: raise D1_10_SECONDS, and call xip_cache_invalidate_all()
             * here (link hardware_xip_cache). The vulnerable window is spin_unlock() to the
             * vTaskSuspendAll() at the top of xEventGroupWaitBits(), so its width is however
             * long that prologue takes to fetch, and a churn loop keeps it hot - the test
             * optimising against itself. Neither is on by default: a five minute soak with the
             * cache flushed on every pair found nothing either, and it is far too slow to
             * leave in the suite.
             */
            sem_release(&test_sem);
            busy_wait_us(gap_us);          /* swept: where the second release lands */
            sem_release(&test_sem);
            gap_us = 1 + (gap_us % 40);
            busy_wait_us(30);              /* just enough for a consumer to cycle */
        }
        /* Long enough for a stall to be *confirmed*, not merely glimpsed. While releases keep
         * coming a stranded waiter is rescued by the next one, so with a short silence the
         * probe can watch the bug happen and then disqualify it for recovering. */
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    churn_exited++;
    vTaskSuspend(NULL);
}

void plat_sem_churn_start(uint n) {
    if (churn_started) return;
    churn_started = true;
    churn_stop = false;
    churn_exited = 0;
    if (xTaskCreate(releaser_task_fn, "rel", configMINIMAL_STACK_SIZE, NULL,
                    MAIN_PRIORITY, NULL) != pdPASS) {
        acquirer_create_failed = true;
    }
    for (uint i = 0; i < n && i < CHURN_MAX; i++) {
        churn_count[i] = 0;
        if (xTaskCreate(churn_task_fn, "churn", configMINIMAL_STACK_SIZE,
                        (void *)(uintptr_t)i, HOLDER_PRIORITY, &churn_handle[i]) != pdPASS) {
            acquirer_create_failed = true;
        }
    }
}

uint32_t plat_sem_churn_count(uint i) {
    return i < CHURN_MAX ? churn_count[i] : 0;
}

bool plat_sem_churn_stop(uint n) {
    churn_stop = true;
    /* n consumers plus the releaser; consumers may be blocked in the acquire, so feed them out */
    for (uint i = 0; i < 200 && churn_exited < n + 1; i++) {
        sem_release(&test_sem);
        vTaskDelay(pdMS_TO_TICKS(2));
    }
    return churn_exited >= n + 1;
}

static mutex_t       alias_mutex;
static volatile bool thief_run;

static void thief_task(__unused void *param) {
    while (thief_run) {
        mutex_enter_blocking(&alias_mutex);
        busy_wait_us(50);
        mutex_exit(&alias_mutex);
        taskYIELD();
    }
    vTaskDelete(NULL);
}

void plat_start_bit_thief(uint spin_lock_num) {
    /* Initialise normally first so every field is valid, then re-point the lock_core at the
     * spin lock whose event-group bit we want contended. */
    mutex_init(&alias_mutex);
    lock_init(&alias_mutex.core, spin_lock_num);
    thief_run = true;
    /* Two, so each genuinely blocks on the other and its waiter consumes the bit. Below
     * MAIN_PRIORITY, so they run while the case under test is sleeping. */
    for (uint i = 0; i < 2; i++) {
        xTaskCreate(thief_task, "thief", configMINIMAL_STACK_SIZE, NULL, HOLDER_PRIORITY, NULL);
    }
}

void plat_stop_bit_thief(void) {
    thief_run = false;
    vTaskDelay(pdMS_TO_TICKS(5));
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
    /* Created here, on the core about to call vTaskStartScheduler(), so that the critical
     * section this takes is repaired by xPortStartScheduler() - see plat_init(). */
    go_sem = xSemaphoreCreateBinary();
    held_sem = xSemaphoreCreateBinary();

    TaskHandle_t xMain;
    xTaskCreate(main_task, "main", configMINIMAL_STACK_SIZE * 2, NULL, MAIN_PRIORITY, &xMain);
#if configNUMBER_OF_CORES > 1 && configUSE_CORE_AFFINITY
    /* Pin the cases to the same core as the spinner. The yield metric compares the spinner's
     * progress against its idle rate, so it can only see starvation if the two are on one
     * core; unpinned under SMP the spinner just runs on the other one and every wait looks
     * like it yielded. SMP is the configuration where a scheduler-core spin would matter
     * most, so leaving it unmeasurable was the wrong trade. The holder is left unpinned, so
     * cross-core contention is still exercised. */
    vTaskCoreAffinitySet(xMain, 1u << 0);
#else
    (void)xMain;
#endif
    xTaskCreate(holder_task, "holder", configMINIMAL_STACK_SIZE, NULL, HOLDER_PRIORITY, NULL);
    vTaskStartScheduler();
}

void plat_init(void) {
    /*
     * SDK primitives only. The FreeRTOS objects are deliberately NOT created here.
     *
     * plat_init() runs on core 0, which under RUN_FREE_RTOS_ON_CORE=1 never starts a
     * scheduler. Creating a queue or semaphore takes a FreeRTOS critical section, and
     * ulCriticalNesting is initialised to 0xaaaaaaaa (port.c) rather than 0 - so
     * vPortEnterCritical() sets BASEPRI, vPortExitCritical() decrements the count without
     * reaching zero, and interrupts are never re-enabled. Only xPortStartScheduler() repairs
     * it, by zeroing the count. On the core that starts the scheduler that happens moments
     * later and nothing is noticed; on a core that stays bare, BASEPRI stays set for the
     * whole run, masking every IRQ at or below configMAX_SYSCALL_INTERRUPT_PRIORITY - the
     * timer included, at PICO_DEFAULT_IRQ_PRIORITY 0x80 against a threshold of 16.
     *
     * That is what made every alarm on the bare core silently fail to arm: ta_force_irq()
     * asserted INTS and the core simply would not take it.
     *
     * NOTE the port does the same thing to itself, and this test cannot avoid that one:
     * prvRuntimeInitializer() is a constructor, so it runs on core 0 before main and calls
     * xEventGroupCreate(). Until the port is fixed - by normalising the sentinel in
     * vPortEnterCritical(), or by using the static event group - the RUN_FREE_RTOS_ON_CORE=1
     * variants will fail D0/D1.6/D2.6/D2.7 for that reason and not for any fault of the SDK.
     * Deliberately not worked around here: clearing the mask would hide the defect.
     */
    mutex_init(&test_mutex);
    recursive_mutex_init(&test_rmutex);
    sem_init(&test_sem, 0, 16);
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

void plat_start_sem_acquirers(__unused uint n) {
    /* no tasks, so there is no shared event-group bit to contend for; D1.10 skips */
}

void plat_start_one_sem_acquirer(void) {
}

void plat_start_one_sem_acquirer_with_hook(__unused void (*hook)(void)) {
}

uint plat_sem_acquirers_done(void) {
    return 0;
}

uint plat_sem_acquirers_mask(void) {
    return 0;
}

bool plat_sem_acquirer_create_failed(void) {
    return false;
}

uint32_t plat_free_heap(void) {
    return 0;
}

void plat_sem_churn_start(__unused uint n) {
}

uint32_t plat_sem_churn_count(__unused uint i) {
    return 0;
}

bool plat_sem_churn_stop(__unused uint n) {
    return true;
}

bool plat_sem_churn_blocked(__unused uint i) {
    return false;
}

void plat_start_bit_thief(__unused uint spin_lock_num) {
    /* no tasks, so no waiter can consume a bit; D3.4 skips */
}

void plat_stop_bit_thief(void) {
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
#if INTEROP_TESTS_ON_CORE == 1
    return "baseline, no RTOS (SDK lock_core macros, core 1 drives, core 0 agents)";
#else
    return "baseline, no RTOS (SDK lock_core macros, core 0 drives, core 1 agents)";
#endif
}

void plat_init(void) {
    mutex_init(&test_mutex);
    recursive_mutex_init(&test_rmutex);
    sem_init(&test_sem, 0, 16);
}

static void (*baseline_body)(void);
static void baseline_body_trampoline(void) { baseline_body(); }

void plat_run(void (*body)(void)) {
#if INTEROP_TESTS_ON_CORE == 1
    /* Cases on core 1, agent on core 0 - the same core assignment as the _core1 FreeRTOS
     * build, but with no RTOS in the picture at all. */
    baseline_body = body;
    multicore_launch_core1(baseline_body_trampoline);
    sdk_core_agent();
#else
    multicore_launch_core1(sdk_core_agent);
    body();
#endif
    for (;;) {
        tight_loop_contents();
    }
}

#endif
