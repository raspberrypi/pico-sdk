/*
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * pico-sdk lock_core sync tests, run both bare and under FreeRTOS.
 *
 * Under FreeRTOS the SDK's lock_core primitives are overridden
 * (configSUPPORT_PICO_SYNC_INTEROP) so a task blocking on an SDK mutex/semaphore blocks the
 * *task* rather than spinning the core. This suite checks that the override actually does
 * that, and measures how well - but it also runs with no RTOS at all, which serves two
 * purposes: it is a direct test of the SDK's own primitives, and it validates the harness,
 * so that anything failing only under FreeRTOS is attributable to interop rather than to
 * the SDK or to the instrumentation.
 *
 * Two disciplines are modelled directly rather than by calling the real code that uses
 * them, so that no case is contingent on a config macro - with PICO_STDOUT_MUTEX=0,
 * stdout_serialize_begin() compiles to `return true` and a printf-based test would silently
 * exercise nothing:
 *
 *   D1  blocking discipline, as used by pico_malloc:
 *       mutex_enter_blocking() / mutex_exit()
 *       -> port entry lock_internal_spin_unlock_with_wait / _with_notify
 *
 *   D2  try-then-timed discipline, as used by pico_stdio:
 *       mutex_try_enter_block_until() / mutex_exit()
 *       -> port entry lock_internal_spin_unlock_with_best_effort_wait_or_timeout
 *
 * These are separate code paths in the port, not the same test with a timeout, which is why
 * they are separate cases throughout.
 *
 * Build variants (see CMakeLists.txt):
 *   _baseline      no RTOS; core 0 drives, core 1 holds/agents
 *   _baseline_rev  no RTOS; core 1 drives, core 0 agents
 *   _smp           FreeRTOS SMP; both cores are FreeRTOS, so cross-core cases skip
 *   _core0         FreeRTOS on core 0, bare SDK code on core 1
 *   _core1         FreeRTOS on core 1, bare SDK code on core 0
 *
 * _baseline_rev exists so that "core 0 is the agent" and "FreeRTOS is on core 1" do not only
 * ever occur together: without it a failure in _core1 cannot be attributed to either. It is
 * the control that showed the _core1 alarm failures to be FreeRTOS's rather than the SDK's.
 *
 * Note the default alarm pool stays on core 0 in BOTH baseline directions - it is created
 * during runtime init, before either core is chosen as the driver. That is deliberate and
 * must stay that way: if the pool followed the driving core, the reversed baseline would
 * differ from the forward one in two respects at once and would no longer be a control. It
 * also means the reversed baseline shares its pool arrangement with _core1 exactly, so the
 * only remaining difference between those two is the RTOS.
 */

#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/sync.h"
#include "pico/lock_core.h"
#include "pico/time.h"
#include "hardware/clocks.h"

#include "interop_platform.h"
#include "interop_harness.h"

/* D1.7 and D2.5 perform a blocking lock_core wait from ISR context. That is legal bare, but
 * the FreeRTOS port forbids it with configASSERT(!portCHECK_IF_IN_ISR()), which aborts the
 * whole run - so under FreeRTOS they are opt-in and must be run in isolation. */
#ifndef INTEROP_RUN_ISR_BLOCKING
#define INTEROP_RUN_ISR_BLOCKING (!INTEROP_HAVE_FREERTOS)
#endif

/*
 * Busy-polling where the code should block is always a hard failure, under FreeRTOS as much
 * as in the baseline.
 *
 * It used to be RESULT_EXPECTED_FAIL under FreeRTOS, on the grounds that the port's
 * !portIS_FREE_RTOS_CORE() branch is a plain spin_unlock(); __wfe() with no equivalent of
 * lock_internal_notify_count, so on RP2350 with software spin locks the WFE falls straight
 * through. That is a *diagnosis*, not an excuse: RESULT_EXPECTED_FAIL is for behaviour that
 * merely looks wrong and is in fact correct, never for a defect we happen to have explained
 * already. Marking a known bug as expected is how a suite goes green over the very thing it
 * was written to catch.
 *
 * Note this does not fire for the RTOS core's timed path, which waits on the event group
 * bounded by the tick and then yields: that blocks, just at tick granularity, so its wait
 * counts stay low.
 */
#define BUSY_POLL_VERDICT RESULT_FAIL

/*
 * These must be resolved in the preprocessor, not in C: PICO_SYNC_RP2350_SPIN_LOCK_WORKAROUND
 * is #defined *to* PICO_SPIN_LOCK_UNLOCK_CAUSES_SEV, which is simply undefined when using
 * hardware spin locks. #if treats that as 0; C code would see an undeclared identifier.
 */
#if PICO_SPIN_LOCK_UNLOCK_CAUSES_SEV
#define INTEROP_UNLOCK_SEVS 1
#else
#define INTEROP_UNLOCK_SEVS 0
#endif
#if PICO_SYNC_RP2350_SPIN_LOCK_WORKAROUND
#define INTEROP_WORKAROUND 1
#else
#define INTEROP_WORKAROUND 0
#endif
/*
 * When an RTOS supplies its own lock_internal_* macros, lock_core.h's versions - and with
 * them the whole RP2350 workaround - are never compiled. Reporting "workaround enabled" in
 * that case is a lie that made two builds look meaningfully different when only the raw
 * spin_lock/unlock fast path differed. It also decides whether pico_time uses its
 * sleep_notifier: time.c enables that on exactly this condition.
 */
#if LOCK_INTERNAL_SPIN_UNLOCK_WITH_WAIT_OVERRIDDEN | LOCK_INTERNAL_SPIN_UNLOCK_WITH_NOTIFY_OVERRIDDEN | LOCK_INTERNAL_SPIN_UNLOCK_WITH_BEST_EFFORT_WAIT_OR_TIMEOUT_OVERRIDDEN
#define INTEROP_LOCK_MACROS_OVERRIDDEN 1
#else
#define INTEROP_LOCK_MACROS_OVERRIDDEN 0
#endif

#define HOLD_MS     20      /* how long a holder holds a contended lock */
#define ITERS       20      /* contended iterations per case */
#define TIMEOUT_MS  50      /* deadline used by the D2 timed cases */

/*
 * Rescue alarm.
 *
 * A notify that raises a semaphore's count but fails to wake the waiter deadlocks a blocking
 * acquire, and a run that hangs reports nothing about the cases after it. The rescue pool is
 * created on the core that runs the cases, so its callback always takes that core's own
 * notify path - even when the default pool is serviced by the other core, which is exactly
 * the configuration under suspicion. If it has to fire, that *is* the finding.
 */
#define RESCUE_GRACE_US 20000   /* a legitimate ISR->waiter wake measures tens of us */
static alarm_pool_t *local_pool;
static volatile bool rescue_flag;

static int64_t rescue_alarm(__unused alarm_id_t id, __unused void *ud) {
    rescue_flag = true;
    sem_release(&test_sem);
    return 0;
}

static void rescue_init(void) {
    /* This claims the hardware alarm itself - do not claim one first and pass it to
     * alarm_pool_create(), which claims internally and would assert on the double claim. */
    local_pool = alarm_pool_create_on_timer_with_unused_hardware_alarm(
            alarm_pool_get_default_timer(), 4);
}

/*
 * Fire a callback on the core running the cases. The default pool belongs to whichever core
 * created it (core 0), which is not necessarily this one - and an ISR that blocks on a mutex
 * held by the core it fired on deadlocks against the thread it interrupted. That is what took
 * out D1.7 the first time the reversed baseline ran.
 */
static alarm_id_t local_alarm_in_us(uint64_t us, alarm_callback_t cb) {
    return local_pool ? alarm_pool_add_alarm_in_us(local_pool, us, cb, NULL, true)
                      : add_alarm_in_us(us, cb, NULL, true);
}

static alarm_id_t rescue_arm(uint64_t us) {
    if (!local_pool) return -1;
    rescue_flag = false;
    return alarm_pool_add_alarm_in_us(local_pool, us, rescue_alarm, NULL, true);
}

/* True only if the rescue actually ran, i.e. the notify under test failed to wake us.
 *
 * The flag is the authority, not alarm_pool_cancel_alarm()'s return value: that was observed
 * returning true for an alarm which had demonstrably already run (the wake latency was the
 * full RESCUE_GRACE_US), which made every rescue count as a normal wake and hid the finding.
 * Cancelling is still worth doing so a pending alarm cannot fire into the next iteration. */
static bool rescue_fired(alarm_id_t id) {
    if (id <= 0) return false;
    alarm_pool_cancel_alarm(local_pool, id);
    return rescue_flag;
}

static int64_t sem_release_alarm(__unused alarm_id_t id, __unused void *ud) {
    sem_release(&test_sem);
    return 0;
}

/* ==================================================================================== */
/* D1 - blocking discipline (as pico_malloc)                                            */
/* ==================================================================================== */

static void d1_1_uncontended(void) {
    const uint n = 1000;
    absolute_time_t t0 = get_absolute_time();
    for (uint i = 0; i < n; i++) {
        mutex_enter_blocking(&test_mutex);
        mutex_exit(&test_mutex);
    }
    int64_t us = absolute_time_diff_us(t0, get_absolute_time());
    bool free_after = mutex_try_enter(&test_mutex, NULL);
    if (free_after) mutex_exit(&test_mutex);

    if (!free_after) {
        harness_record("D1.1", RESULT_FAIL, "mutex still held after balanced enter/exit");
    } else {
        harness_record("D1.1", RESULT_PASS, "%u uncontended cycles, %lldns each",
                       n, (long long)(us * 1000 / n));
    }
}

static void d1_2_contended(void) {
    latency_t lat;
    latency_reset(&lat);
    int worst_yield = -1;    /* -1 = not measured; never report the initialiser as a result */

    for (uint i = 0; i < ITERS; i++) {
        plat_hold_for_ms(HOLD_MS);
        uint32_t sp0 = harness_spinner_count();
        absolute_time_t t0 = get_absolute_time();
        mutex_enter_blocking(&test_mutex);
        int64_t waited = absolute_time_diff_us(t0, get_absolute_time());
        uint32_t sp = harness_spinner_count() - sp0;
        mutex_exit(&test_mutex);

        latency_add(&lat, waited);
        int y = harness_yield_pct(sp, (uint32_t)(waited / 1000));
        if (y >= 0 && (worst_yield < 0 || y < worst_yield)) worst_yield = y;
    }
    latency_print(&lat, "blocked wait duration");

    if (lat.max_us > (int64_t)HOLD_MS * 1000 + 5000) {
        harness_record("D1.2", RESULT_FAIL, "worst wait %lldus for a %dms hold",
                       (long long)lat.max_us, HOLD_MS);
    } else if (worst_yield >= 0 && worst_yield < 50) {
        harness_record("D1.2", RESULT_FAIL, "waiter did not yield (yield %d%%)", worst_yield);
    } else if (worst_yield < 0) {
        harness_record("D1.2", RESULT_PASS, "max wait %lldus for %dms hold (no yield metric)",
                       (long long)lat.max_us, HOLD_MS);
    } else {
        harness_record("D1.2", RESULT_PASS, "max wait %lldus for %dms hold, yield %d%%",
                       (long long)lat.max_us, HOLD_MS, worst_yield);
    }
}

static void d1_3_priority_asymmetric(void) {
#if !INTEROP_HAS_SCHEDULER
    harness_record("D1.3", RESULT_SKIP, "no scheduler, so no priority inversion to measure");
#else
    /* main runs above the holder, so this measures the inversion window. SDK mutexes have
     * no priority inheritance, so this records rather than asserts - beyond "it completes,
     * and does not exceed the hold time by much". */
    latency_t lat;
    latency_reset(&lat);
    for (uint i = 0; i < ITERS; i++) {
        plat_hold_for_ms(HOLD_MS);
        absolute_time_t t0 = get_absolute_time();
        mutex_enter_blocking(&test_mutex);
        latency_add(&lat, absolute_time_diff_us(t0, get_absolute_time()));
        mutex_exit(&test_mutex);
    }
    latency_print(&lat, "inversion window");
    harness_record("D1.3",
                   lat.max_us > (int64_t)HOLD_MS * 1000 + 5000 ? RESULT_FAIL : RESULT_INFO,
                   "inversion window max %lldus (no priority inheritance by design)",
                   (long long)lat.max_us);
#endif
}

static void d1_4_sdk_core_waits(void) {
#if !INTEROP_HAS_SDK_CORE
    harness_record("D1.4", RESULT_SKIP, "no bare-SDK core in this configuration");
#else
    /* This core holds; the other (never RTOS-scheduled) core blocks on it. Under FreeRTOS
     * that takes the !portIS_FREE_RTOS_CORE() branch: spin_unlock(); __wfe(). On RP2350
     * with software spin locks the unlock sets that core's own event, so the __wfe() falls
     * straight through and it busy-polls rather than sleeping. */
    mutex_enter_blocking(&test_mutex);
    agent_start(AGENT_MUTEX_ENTER_COUNTED, 0);
    plat_delay_ms(HOLD_MS);
    mutex_exit(&test_mutex);

    if (!agent_wait(1000)) {
        harness_record("D1.4", RESULT_FAIL, "other core never acquired the mutex");
        return;
    }
    uint32_t us = agent_elapsed_us();
    uint32_t waits = agent_wait_count();
    int occ = harness_occupancy_pct(agent_cycles(), us);
    agent_run(AGENT_MUTEX_EXIT, 0, 1000);

    /* wait-loop iterations are the criterion; occupancy is only a secondary note, since on
     * Arm DWT_CYCCNT counts through WFE and so cannot see sleep at all */
    /* Cross-check DWT_SLEEPCNT against the wait count, whose ground truth is independent.
     * Not a verdict - SLEEPCNT is on probation until it agrees with a known-good/known-broken
     * pair (D1.4 blocks, D1.9 busy-polls, same build and core). */
    char occ_note[88] = "";
    if (occ >= 0) snprintf(occ_note, sizeof(occ_note), ", %d%% occupancy", occ);
    /* Use the INLINE delta, taken immediately around the wait. The command-boundary delta
     * reads 0 here even when the core demonstrably slept (D0 shows the two agree for a bare
     * WFE, so the boundary read is not generally broken - just unreliable across this
     * command). Unexplained; the inline read is the one that tracks reality. */
    if (harness_sleep_counter_usable()) {
        bool expect_slept = waits <= MAX_BLOCKING_WAITS;
        bool slept = agent_inline_sleep_delta() != 0;
        snprintf(occ_note + strlen(occ_note), sizeof(occ_note) - strlen(occ_note),
                 ", SLEEPCNT %s (d=%u)", slept == expect_slept ? "agrees" : "DISAGREES",
                 agent_inline_sleep_delta());
    }

    if (waits > MAX_BLOCKING_WAITS) {
        harness_record("D1.4", BUSY_POLL_VERDICT,
                       "other core busy-polled: %u wait iterations over %uus%s",
                       waits, us, occ_note);
    } else {
        harness_record("D1.4", RESULT_PASS, "other core blocked: %u wait iterations, %uus%s",
                       waits, us, occ_note);
    }
#endif
}

static void d1_5_sdk_core_notifies(void) {
#if !INTEROP_HAS_SDK_CORE
    harness_record("D1.5", RESULT_SKIP, "no bare-SDK core in this configuration");
#else
    /* The bare-SDK core holds; this context waits. Under FreeRTOS single-core this is the
     * cross-core delivery path - doorbell on RP2350, multicore FIFO on RP2040 - via
     * uxCrossCoreEventBits. */
    plat_sdk_core_hold_for_ms(HOLD_MS);

    uint32_t sp0 = harness_spinner_count();
    absolute_time_t t0 = get_absolute_time();
    mutex_enter_blocking(&test_mutex);
    int64_t waited = absolute_time_diff_us(t0, get_absolute_time());
    uint32_t sp = harness_spinner_count() - sp0;
    mutex_exit(&test_mutex);
    agent_wait(1000);

    int y = harness_yield_pct(sp, (uint32_t)(waited / 1000));
    if (waited > (int64_t)(HOLD_MS + 30) * 1000) {
        harness_record("D1.5", RESULT_FAIL, "cross-core wakeup took %lldus", (long long)waited);
    } else if (y < 0) {
        harness_record("D1.5", RESULT_PASS, "cross-core wakeup in %lldus", (long long)waited);
    } else {
        harness_record("D1.5", RESULT_PASS, "cross-core wakeup in %lldus, yield %d%%",
                       (long long)waited, y);
    }
#endif
}

static void d1_6_notify_from_isr(void) {
    latency_t lat;
    latency_reset(&lat);

    uint rescued = 0;
    bool add_failed = false;
    int  add_result = 0;
    uint add_fail_iter = 0;
    for (uint i = 0; i < ITERS; i++) {
        uint32_t delay_us = 2000 + (i % 7) * 250;
        sem_reset(&test_sem, 0);
        absolute_time_t target = make_timeout_time_us(delay_us);
        /* Report the id: the two failure modes are indistinguishable without it. -1 means
         * the pool had no free slot; 0 means the deadline was already past and fire_if_past
         * ran the callback inline, which is not a failure of the notify path at all. */
        alarm_id_t primary = add_alarm_in_us(delay_us, sem_release_alarm, NULL, true);
        if (primary <= 0) {
            /* Do not return: if earlier iterations were rescued, THAT is the finding and a
             * full pool is merely its consequence - the entries pile up precisely because
             * their callbacks never ran. Losing that to an early return reported the symptom
             * and hid the cause. */
            add_failed = true;
            add_result = (int)primary;
            add_fail_iter = i;
            break;
        }
        /* The wait must stay sem_acquire_blocking(): a timed acquire would re-check the count
         * when the tick expired and so turn a lost notify into a slightly late success, which
         * is precisely the bug we are looking for. Instead the deadlock is broken from
         * outside, by an alarm on a pool this core services - see local_pool. */
        alarm_id_t rescue = rescue_arm(delay_us + RESCUE_GRACE_US);
        sem_acquire_blocking(&test_sem);
        if (rescue_fired(rescue)) {
            rescued++;
        } else {
            latency_add(&lat, absolute_time_diff_us(target, get_absolute_time()));
        }
    }
    latency_print(&lat, "ISR notify -> waiter wake");

    if (rescued && add_failed) {
        harness_record("D1.6", RESULT_FAIL,
                       "%u of %u ISR notifies never woke the waiter; pool then full at"
                       " iteration %u (%d) as those alarms never ran - callbacks not serviced"
                       " on core %u",
                       rescued, ITERS, add_fail_iter, add_result,
                       alarm_pool_core_num(alarm_pool_get_default()));
    } else if (add_failed) {
        harness_record("D1.6", RESULT_FAIL,
                       "could not add alarm on iteration %u: add_alarm_in_us() returned %d"
                       " (%s)", add_fail_iter, add_result,
                       add_result == 0 ? "deadline already past, fired inline"
                                       : "no free slot in the default pool");
    } else if (rescued) {
        /* The notify raised the semaphore count but never woke the waiter; only the rescue
         * alarm, whose callback runs on this core, got it moving again. */
        harness_record("D1.6", RESULT_FAIL,
                       "%u of %u ISR notifies did not wake the waiter (rescued after %ums);"
                       " alarm callback runs on core %u, this core is %u",
                       rescued, ITERS, RESCUE_GRACE_US / 1000,
                       alarm_pool_core_num(alarm_pool_get_default()), get_core_num());
    } else if (lat.count != ITERS) {
        harness_record("D1.6", RESULT_FAIL, "only %u of %u wakeups", lat.count, ITERS);
    } else if (lat.max_us > 20000) {
        harness_record("D1.6", RESULT_FAIL, "worst ISR->waiter wake %lldus",
                       (long long)lat.max_us);
    } else {
        harness_record("D1.6", RESULT_PASS, "mean %lldus, max %lldus",
                       (long long)latency_mean_us(&lat), (long long)lat.max_us);
    }
}

#if INTEROP_RUN_ISR_BLOCKING
static volatile bool d1_7_returned;
static int64_t blocking_acquire_in_isr(__unused alarm_id_t id, __unused void *ud) {
    mutex_enter_blocking(&test_mutex);
    mutex_exit(&test_mutex);
    d1_7_returned = true;
    return 0;
}
#endif

static void d1_7_blocking_acquire_from_isr(void) {
#if !INTEROP_RUN_ISR_BLOCKING
    harness_record("D1.7", RESULT_SKIP,
                   "opt-in under FreeRTOS: trips configASSERT(!in ISR); -DINTEROP_RUN_ISR_BLOCKING=1");
#else
    /* Legal bare: the ISR simply spins/WFEs until the holder releases. Under FreeRTOS this
     * is what the port forbids, which is exactly the contrast worth documenting. */
    d1_7_returned = false;
    plat_sdk_core_hold_for_ms(HOLD_MS);
    local_alarm_in_us(1000, blocking_acquire_in_isr);
    plat_delay_ms(HOLD_MS * 3);
    harness_record("D1.7", d1_7_returned ? RESULT_PASS : RESULT_FAIL,
                   "blocking acquire from ISR %s",
                   d1_7_returned ? "completed" : "never returned");
#endif
}

/*
 * Assert that this platform has the counters we expect it to have, and that they work.
 *
 * Without this the suite degrades silently: a counter that stops working just makes the
 * calibration report "unusable" and every case still passes. That is exactly what happened
 * with mcycle on Hazard3, where RVCSR_MCOUNTINHIBIT resets with the CY inhibit bit set - the
 * cycle rate read 0/us for a whole run and nothing flagged it.
 */
static void d0_counter_selftest(void) {
#if !INTEROP_HAS_SDK_CORE
    harness_record("D0", RESULT_SKIP, "counters are calibrated on the agent core; none here");
#else
    /* 0. a calibration that never arrived is not a counter fault, and reporting it as one
     *    sends you looking at DWT enables instead of at the agent core's alarms. Note this
     *    runs before rescue_init(), so a failure here cannot be the rescue pool's claim. */
    if (!harness_agent_calibrated()) {
        /* Ask the bare core itself whether the default pool's alarm IRQ is live there. The
         * NVIC enable is per core, so this is the one question no other core can answer, and
         * it separates "the IRQ was never enabled on that core" from "it is enabled but the
         * alarm is never armed". */
        if (agent_run(AGENT_IRQ_STATE, 0, 1000)) {
            /* Verdict first: harness_record() truncates at detail[120], and putting the
             * ENABLED/DISABLED word last once cost a whole hardware run. */
            /* The NVIC enable only says the core would take the interrupt. Whether the
             * timer is set up to raise one is a separate question, so read its own state:
             * INTE not set means the pool never enabled it; ARMED not set means the alarm
             * was never programmed; INTR set means it fired and was never serviced. */
            uint abit = 1u << alarm_pool_timer_alarm_num(alarm_pool_get_default());
            const char *why =
                    !agent_alarm_irq_enabled()          ? "NVIC irq disabled on that core" :
                    !(agent_timer_inte() & abit)        ? "timer INTE clear: irq never enabled" :
                    (agent_timer_intr() & abit)         ? "INTR set: fired but never serviced" :
                    !(agent_timer_armed() & abit)       ? "not armed: alarm never programmed" :
                                                          "armed and pending, simply not firing";
            harness_record("D0", RESULT_FAIL,
                           "no calibration from bare core - %s (irq %u inte=%x intr=%x"
                           " armed=%x alarm=%u now=%u)", why, agent_alarm_irq_num(),
                           agent_timer_inte(), agent_timer_intr(), agent_timer_armed(),
                           agent_timer_alarm_val(), agent_timer_now());
        } else {
            harness_record("D0", RESULT_FAIL,
                           "the agent core never returned a calibration, and did not answer"
                           " the IRQ-state probe either");
        }
        return;
    }

    /* 1. the cycle counter must actually count, at roughly clk_sys */
    uint32_t expected = (uint32_t)(clock_get_hz(clk_sys) / 1000000u);
    uint32_t actual = harness_cal_cycles_per_us();
    if (actual < expected * 3 / 4 || actual > expected * 5 / 4) {
        harness_record("D0", RESULT_FAIL,
                       "cycle counter reads %u/us, expected ~%u (clk_sys);"
                       " agent core DWT_CTRL=0x%08x DEMCR=0x%08x (CYCCNTENA=%u TRCENA=%u)",
                       actual, expected, harness_agent_dwt_ctrl(), harness_agent_demcr(),
                       harness_agent_dwt_ctrl() & 1u, (harness_agent_demcr() >> 24) & 1u);
        return;
    }

    /* 2. Where this platform is expected to have a sleep counter, prove it actually works.
     *    (There is nothing to assert where none is expected: harness_sleep_counter_present()
     *    is derived from the same architecture macros as the expectation, so comparing them
     *    would only restate a compile-time constant. The runtime probe below is the real
     *    check - it is what would have caught the mcycle inhibit had it applied to SLEEPCNT.)
     */
    if (!HARNESS_EXPECT_SLEEP_COUNTER) {
        harness_record("D0", RESULT_PASS,
                       "cycle counter %u/us; no DWT sleep counter on this architecture",
                       actual);
        return;
    }

    /*    one known WFE, measured inline
     *    and at the command boundary, so an instrumentation fault is distinguishable from a
     *    hardware one. */
    if (!agent_run(AGENT_KNOWN_SLEEP, 20, 2000)) {
        harness_record("D0", RESULT_FAIL, "known-sleep probe never returned");
        return;
    }
    uint32_t inl = agent_inline_sleep_delta();
    uint32_t bnd = agent_sleep_delta();
    if (!inl) {
        harness_record("D0", RESULT_FAIL,
                       "sleep counter present but did not move across a known 20ms WFE"
                       " (inline d=0, boundary d=%u)", bnd);
    } else if (!bnd) {
        harness_record("D0", RESULT_INFO,
                       "sleep counter works (inline d=%u) but the boundary read is 0"
                       " - instrumentation, not hardware", inl);
    } else {
        harness_record("D0", RESULT_PASS,
                       "cycle counter %u/us; sleep counter moved (inline d=%u, boundary d=%u)",
                       actual, inl, bnd);
    }
#endif
}

static void d1_9_bare_wfe_pattern(void) {
#if !INTEROP_HAS_SDK_CORE
    harness_record("D1.9", RESULT_SKIP, "no bare-SDK core in this configuration");
#else
    /* Tests the bare spin_unlock(); __wfe() pattern directly - the one the FreeRTOS ports
     * hard-code for a non-FreeRTOS core. It must busy-poll precisely when a spin unlock sets
     * the calling core's own event. Anything else means our model of the hardware is wrong,
     * which is worth a hard failure in either direction. */
    mutex_enter_blocking(&test_mutex);
    agent_start(AGENT_MUTEX_ENTER_BARE, 0);
    plat_delay_ms(HOLD_MS);
    mutex_exit(&test_mutex);
    if (!agent_wait(1000)) {
        harness_record("D1.9", RESULT_FAIL, "bare wait never completed");
        return;
    }
    uint32_t waits = agent_wait_count();
    bool slept = agent_slept();
    agent_run(AGENT_MUTEX_EXIT, 0, 1000);
    bool polled = waits > MAX_BLOCKING_WAITS;
    char sc_note[56] = "";
    if (harness_sleep_counter_usable()) {
        /* inline delta - see the note in d1_4_sdk_core_waits() */
        snprintf(sc_note, sizeof(sc_note), ", SLEEPCNT %s (d=%u)",
                 (agent_inline_sleep_delta() != 0) == !polled ? "agrees" : "DISAGREES",
                 agent_inline_sleep_delta());
    }

    const bool expect_poll = INTEROP_UNLOCK_SEVS;
    if (polled == expect_poll) {
        harness_record("D1.9", expect_poll ? RESULT_EXPECTED_FAIL : RESULT_PASS,
                       "bare spin_unlock()+__wfe() %s as predicted: %u wait iterations%s",
                       expect_poll ? "busy-polls" : "blocks", waits, sc_note);
    } else {
        harness_record("D1.9", RESULT_FAIL,
                       "bare pattern %s (%u iterations) but PICO_SPIN_LOCK_UNLOCK_CAUSES_SEV"
                       " predicts it would %s", polled ? "busy-polled" : "blocked", waits,
                       expect_poll ? "busy-poll" : "block");
    }
#endif
}

static void d1_8_recursive(void) {
    recursive_mutex_enter_blocking(&test_rmutex);
    recursive_mutex_enter_blocking(&test_rmutex);
    recursive_mutex_enter_blocking(&test_rmutex);
    recursive_mutex_exit(&test_rmutex);
    recursive_mutex_exit(&test_rmutex);
    recursive_mutex_exit(&test_rmutex);

    bool now_free = recursive_mutex_try_enter(&test_rmutex, NULL);
    if (now_free) recursive_mutex_exit(&test_rmutex);

    if (!now_free) {
        harness_record("D1.8", RESULT_FAIL, "recursive mutex not released after balanced exits");
    } else {
        harness_record("D1.8", RESULT_PASS, "3-deep nesting released correctly");
    }
}

static void d2_9_expired_deadline(void) {
#if !INTEROP_HAS_SDK_CORE
    harness_record("D2.9", RESULT_SKIP, "no bare-SDK core in this configuration");
#else
    /* Repeated waits on an already-expired deadline - the fff4363b3 case. Every other D2
     * case takes a fresh future deadline each iteration, so last_added never matches and the
     * short-circuit branch is never entered. Bounded via the agent so a regression reports
     * rather than hangs (pico_sync_test catches this too, but by hanging). */
    if (!agent_run(AGENT_EXPIRED_DEADLINE, TIMEOUT_MS, TIMEOUT_MS * 20)) {
        harness_record("D2.9", RESULT_FAIL,
                       "wedged on an already-expired deadline - no event is coming, so the"
                       " bare __wfe() never returns");
        return;
    }
    harness_record("D2.9", agent_result() ? RESULT_PASS : RESULT_FAIL,
                   "repeated waits on an expired deadline returned promptly (%uus)",
                   agent_elapsed_us());
#endif
}

/* ==================================================================================== */
/* D3 - sleep_until / sleep_ms                                                          */
/*                                                                                      */
/* Deliberately NOT a sub-tick sweep: short_sleep_test already does 0-50us across both   */
/* cores, per-sleep never-early plus a cumulative bound, and does it better. These are   */
/* the three things it cannot express - ms-scale, more than one sleeper, and the bare-   */
/* SDK core sleeping while the other hammers the alarm pool.                             */
/* ==================================================================================== */

static void d3_1_sleep_accuracy_ms(void) {
    latency_t lat;
    latency_reset(&lat);
    uint32_t slept_count = 0;
    for (uint i = 0; i < 20; i++) {
        uint32_t ms = 1 + (i % 5) * 3;      /* 1..13ms, beyond short_sleep_test's 50us */
        absolute_time_t target = make_timeout_time_ms(ms);
        /* We do not own sleep_ms()'s loop, so there is no iteration count to take.
         * DWT_SLEEPCNT is the only way to tell a real sleep from a busy-wait that happens to
         * finish on time - a change proves the core slept (it cannot move otherwise). */
        uint32_t sc0 = harness_sleep_counter();
        sleep_ms(ms);
        if (harness_sleep_counter() != sc0) slept_count++;
        latency_add(&lat, absolute_time_diff_us(target, get_absolute_time()));
    }
    latency_print(&lat, "sleep_ms lateness");
    if (lat.early) {
        harness_record("D3.1", RESULT_FAIL,
                       "sleep_ms returned EARLY %u times (contract violation)", lat.early);
    } else if (lat.max_us > 2000) {
        harness_record("D3.1", RESULT_FAIL, "worst oversleep %lldus", (long long)lat.max_us);
    } else if (harness_sleep_counter_usable() && slept_count == 0) {
        /* INFO not FAIL: SLEEPCNT is corroboration on probation, never a verdict */
        harness_record("D3.1", RESULT_INFO,
                       "never early, worst oversleep %lldus, but SLEEPCNT saw no sleep in 20"
                       " - suspect the signal, not sleep_ms", (long long)lat.max_us);
    } else if (harness_sleep_counter_usable()) {
        harness_record("D3.1", RESULT_PASS, "never early, worst oversleep %lldus, slept in %u/20",
                       (long long)lat.max_us, slept_count);
    } else {
        harness_record("D3.1", RESULT_PASS, "never early, worst oversleep %lldus"
                       " (no sleep signal)", (long long)lat.max_us);
    }
}

static volatile int64_t bg_sleeper_worst;

static void d3_2_concurrent_sleepers(void) {
#if !INTEROP_HAS_SCHEDULER
    harness_record("D3.2", RESULT_SKIP, "needs tasks: more than one concurrent sleeper");
#else
    /*
     * The Q2 reproducer, and the case nothing else covers. Every sleep_until() caller shares
     * one global sleep_notifier on PICO_SPINLOCK_ID_TIMER; the FreeRTOS ports map that to a
     * single event-group bit waited on with xClearOnExit, so the first task to wake consumes
     * the notification for everyone. A short sleeper can then oversleep until a long
     * sleeper's alarm fires.
     */
    bg_sleeper_worst = 0;
    plat_start_background_sleeper(500, &bg_sleeper_worst);
    plat_delay_ms(20);

    latency_t lat;
    latency_reset(&lat);
    for (uint i = 0; i < 40; i++) {
        absolute_time_t target = make_timeout_time_ms(2);
        sleep_ms(2);
        latency_add(&lat, absolute_time_diff_us(target, get_absolute_time()));
    }
    latency_print(&lat, "2ms sleep alongside a 500ms sleeper");

    if (lat.early) {
        harness_record("D3.2", RESULT_FAIL, "sleep_ms returned EARLY %u times", lat.early);
    } else if (lat.max_us > 50000) {
        harness_record("D3.2", RESULT_FAIL,
                       "2ms sleep overslept %lldus beside a 500ms sleeper - wakeup stolen",
                       (long long)lat.max_us);
    } else {
        harness_record("D3.2", RESULT_PASS,
                       "worst oversleep %lldus with a concurrent 500ms sleeper",
                       (long long)lat.max_us);
    }
#endif
}

static void d3_3_sleep_on_sdk_core(void) {
#if !INTEROP_HAS_SDK_CORE
    harness_record("D3.3", RESULT_SKIP, "no bare-SDK core in this configuration");
#else
    /* the bare-SDK core sleeps while this core hammers the same alarm pool */
    agent_start(AGENT_SLEEP_MS, 50);
    for (uint i = 0; i < 20; i++) {
        sleep_ms(2);
    }
    if (!agent_wait(2000)) {
        harness_record("D3.3", RESULT_FAIL, "sleep on the other core never returned");
        return;
    }
    uint32_t us = agent_elapsed_us();
    if (us < 50000) {
        harness_record("D3.3", RESULT_FAIL, "50ms sleep on the other core took only %uus"
                       " (returned EARLY)", us);
    } else if (harness_sleep_counter_usable() && !agent_slept()) {
        harness_record("D3.3", RESULT_INFO,
                       "50ms sleep on the other core took %uus but SLEEPCNT saw no sleep", us);
    } else {
        harness_record("D3.3", us > 60000 ? RESULT_FAIL : RESULT_PASS,
                       "50ms sleep on the other core took %uus%s", us,
                       harness_sleep_counter_usable() ? " (confirmed asleep)" : "");
    }
#endif
}

/* ==================================================================================== */
/* D2 - try-then-timed discipline (as pico_stdio)                                       */
/* ==================================================================================== */

static void d2_1_uncontended_try(void) {
    absolute_time_t t0 = get_absolute_time();
    bool ok = mutex_try_enter_block_until(&test_mutex, make_timeout_time_ms(TIMEOUT_MS));
    int64_t us = absolute_time_diff_us(t0, get_absolute_time());
    if (ok) mutex_exit(&test_mutex);

    if (!ok) {
        harness_record("D2.1", RESULT_FAIL, "uncontended try failed");
    } else if (us > 1000) {
        harness_record("D2.1", RESULT_FAIL, "took %lldus - entered the wait path",
                       (long long)us);
    } else {
        harness_record("D2.1", RESULT_PASS, "immediate (%lldus)", (long long)us);
    }
}

static void d2_2_contended_released(void) {
    latency_t lat;
    latency_reset(&lat);
    int worst_yield = -1;    /* -1 = not measured; never report the initialiser as a result */

    for (uint i = 0; i < ITERS; i++) {
        plat_hold_for_ms(HOLD_MS);
        uint32_t sp0 = harness_spinner_count();
        absolute_time_t t0 = get_absolute_time();
        /* deadline well beyond the hold, so this must succeed via a notify not a timeout */
        bool ok = mutex_try_enter_block_until(&test_mutex, make_timeout_time_ms(HOLD_MS * 10));
        int64_t waited = absolute_time_diff_us(t0, get_absolute_time());
        uint32_t sp = harness_spinner_count() - sp0;
        if (!ok) {
            harness_record("D2.2", RESULT_FAIL, "timed out despite release at %dms", HOLD_MS);
            return;
        }
        mutex_exit(&test_mutex);
        latency_add(&lat, waited);
        int y = harness_yield_pct(sp, (uint32_t)(waited / 1000));
        if (y >= 0 && (worst_yield < 0 || y < worst_yield)) worst_yield = y;
    }
    latency_print(&lat, "timed acquire, released before deadline");

    if (lat.max_us > (int64_t)HOLD_MS * 1000 + 5000) {
        harness_record("D2.2", RESULT_FAIL, "worst wait %lldus for a %dms hold",
                       (long long)lat.max_us, HOLD_MS);
    } else if (worst_yield >= 0 && worst_yield < 50) {
        harness_record("D2.2", RESULT_FAIL, "waiter did not yield (%d%%)", worst_yield);
    } else if (worst_yield < 0) {
        harness_record("D2.2", RESULT_PASS, "max wait %lldus for %dms hold (no yield metric)",
                       (long long)lat.max_us, HOLD_MS);
    } else {
        harness_record("D2.2", RESULT_PASS, "max wait %lldus for %dms hold, yield %d%%",
                       (long long)lat.max_us, HOLD_MS, worst_yield);
    }
}

static void d2_3_contended_times_out(void) {
    latency_t lat;
    latency_reset(&lat);
    uint32_t worst_waits = 0;

    for (uint i = 0; i < 5; i++) {
        /* held for far longer than the deadline, so the wait must time out */
        plat_hold_for_ms(TIMEOUT_MS * 3);
        absolute_time_t deadline = make_timeout_time_ms(TIMEOUT_MS);
        bool ok;
        /* counted, so a timeout that fires exactly on time cannot hide a 50ms busy-poll */
        uint32_t w = counted_mutex_enter_block_until(&test_mutex, deadline, &ok);
        if (w > worst_waits) worst_waits = w;
        int64_t lateness = absolute_time_diff_us(deadline, get_absolute_time());
        if (ok) {
            mutex_exit(&test_mutex);
            harness_record("D2.3", RESULT_FAIL, "acquired a lock held for the whole deadline");
            return;
        }
        latency_add(&lat, lateness);
        plat_delay_ms(TIMEOUT_MS * 3);   /* let the holder finish */
    }
    latency_print(&lat, "timeout overshoot");

    if (lat.early) {
        harness_record("D2.3", RESULT_FAIL, "%u timeouts fired EARLY (contract violation)",
                       lat.early);
    } else if (lat.max_us > 5000) {
        harness_record("D2.3", RESULT_FAIL, "timeout overshot by %lldus", (long long)lat.max_us);
    } else if (worst_waits > MAX_BLOCKING_WAITS) {
        harness_record("D2.3", BUSY_POLL_VERDICT,
                       "timeout accurate (max %lldus) but busy-polled: %u wait iterations",
                       (long long)lat.max_us, worst_waits);
    } else {
        harness_record("D2.3", RESULT_PASS, "never early, overshoot max %lldus, %u waits",
                       (long long)lat.max_us, worst_waits);
    }
}

static void d2_4_contended_by_self(void) {
    mutex_enter_blocking(&test_mutex);
    absolute_time_t t0 = get_absolute_time();
    bool ok = mutex_try_enter_block_until(&test_mutex, make_timeout_time_ms(TIMEOUT_MS));
    int64_t us = absolute_time_diff_us(t0, get_absolute_time());
    if (ok) mutex_exit(&test_mutex);
    mutex_exit(&test_mutex);

    if (ok) {
        harness_record("D2.4", RESULT_FAIL, "re-entered a non-recursive mutex we already hold");
    } else if (us > 2000) {
        harness_record("D2.4", RESULT_FAIL,
                       "self-deadlock not detected: waited %lldus for the deadline",
                       (long long)us);
    } else {
        harness_record("D2.4", RESULT_PASS, "self-owner detected immediately (%lldus)",
                       (long long)us);
    }
}

#if INTEROP_RUN_ISR_BLOCKING
static volatile bool d2_5_returned;
static volatile bool d2_5_result;
static int64_t timed_acquire_in_isr(__unused alarm_id_t id, __unused void *ud) {
    /* the owner is not us, so the self-owner check in mutex_try_enter_block_until() misses
     * and this falls into a timed *block* - which the FreeRTOS port forbids from an ISR */
    d2_5_result = mutex_try_enter_block_until(&test_mutex, make_timeout_time_ms(5));
    if (d2_5_result) mutex_exit(&test_mutex);
    d2_5_returned = true;
    return 0;
}
#endif

static void d2_5_timed_from_isr_contended(void) {
#if !INTEROP_RUN_ISR_BLOCKING
    harness_record("D2.5", RESULT_SKIP,
                   "opt-in under FreeRTOS: trips configASSERT(!in ISR); -DINTEROP_RUN_ISR_BLOCKING=1");
#else
    d2_5_returned = false;
    plat_sdk_core_hold_for_ms(HOLD_MS);
    local_alarm_in_us(1000, timed_acquire_in_isr);
    plat_delay_ms(HOLD_MS * 3);
    if (!d2_5_returned) {
        harness_record("D2.5", RESULT_FAIL, "timed acquire from ISR never returned");
    } else {
        harness_record("D2.5", RESULT_PASS, "timed acquire from ISR returned (%s)",
                       d2_5_result ? "acquired" : "timed out");
    }
#endif
}

static volatile bool d2_6_ran;
static volatile bool d2_6_ok;
static volatile uint32_t d2_6_us;

static int64_t try_in_isr_uncontended(__unused alarm_id_t id, __unused void *ud) {
    absolute_time_t t0 = get_absolute_time();
    d2_6_ok = mutex_try_enter_block_until(&test_mutex, make_timeout_time_ms(5));
    d2_6_us = (uint32_t)absolute_time_diff_us(t0, get_absolute_time());
    if (d2_6_ok) mutex_exit(&test_mutex);
    d2_6_ran = true;
    return 0;
}

static void d2_6_try_from_isr_uncontended(void) {
    /* the ordinary printf-from-ISR case: nothing holds the lock, the try succeeds, no wait
     * path is entered. This one must keep working everywhere. */
    d2_6_ran = false;
    local_alarm_in_us(1000, try_in_isr_uncontended);
    plat_delay_ms(50);

    if (!d2_6_ran) {
        harness_record("D2.6", RESULT_FAIL, "ISR callback never ran");
    } else if (!d2_6_ok) {
        harness_record("D2.6", RESULT_FAIL, "uncontended try from ISR failed");
    } else if (d2_6_us > 1000) {
        harness_record("D2.6", RESULT_FAIL, "took %uus - entered the wait path", d2_6_us);
    } else {
        harness_record("D2.6", RESULT_PASS, "immediate from ISR (%uus)", d2_6_us);
    }
}

static void d2_7_sdk_core_timed(void) {
#if !INTEROP_HAS_SDK_CORE
    harness_record("D2.7", RESULT_SKIP, "no bare-SDK core in this configuration");
#else
    /* The bare-SDK core does the timed acquire while this context holds it. Under FreeRTOS
     * that is the non-FreeRTOS branch, which routes to best_effort_wfe_or_timeout(). */
    mutex_enter_blocking(&test_mutex);
    agent_start(AGENT_MUTEX_TIMED_COUNTED, TIMEOUT_MS);
    bool returned = agent_wait(TIMEOUT_MS * 4);
    bool got = agent_result();
    uint32_t us = agent_elapsed_us();
    uint32_t waits = agent_wait_count();
    int occ = harness_occupancy_pct(agent_cycles(), us);
    mutex_exit(&test_mutex);

    if (!returned) {
        harness_record("D2.7", RESULT_FAIL, "timed acquire on the other core never returned");
    } else if (got) {
        harness_record("D2.7", RESULT_FAIL, "acquired despite the lock being held throughout");
    } else if (us < (uint32_t)TIMEOUT_MS * 1000) {
        harness_record("D2.7", RESULT_FAIL, "timed out EARLY after %uus (deadline %dms)",
                       us, TIMEOUT_MS);
    } else if (waits > MAX_BLOCKING_WAITS) {
        harness_record("D2.7", BUSY_POLL_VERDICT,
                       "timed out at %uus but busy-polled: %u wait iterations", us, waits);
    } else {
        char occ_note[24] = "";
        if (occ >= 0) snprintf(occ_note, sizeof(occ_note), ", %d%% occupancy", occ);
        harness_record("D2.7", RESULT_PASS, "timed out at %uus, %u wait iterations%s",
                       us, waits, occ_note);
    }
#endif
}

static void d2_8_repeated_deadline(void) {
    /* Hammers the `static uint64_t last_added` cache inside best_effort_wfe_or_timeout(),
     * which is shared by both cores and non-atomic. Every wait must respect its deadline. */
    latency_t lat;
    latency_reset(&lat);
    uint32_t worst_waits = 0;

    for (uint i = 0; i < 10; i++) {
        plat_hold_for_ms(TIMEOUT_MS * 3);
        absolute_time_t deadline = make_timeout_time_ms(TIMEOUT_MS);
#if INTEROP_HAS_SDK_CORE && INTEROP_HAS_SCHEDULER
        /* The same deadline requested concurrently from the other core. Only where the
         * holder is a task: in the baseline the holder is the agent itself, so it cannot
         * also be asked to acquire. */
        agent_start(AGENT_MUTEX_TRY_ENTER_BLOCK_UNTIL, TIMEOUT_MS);
#endif
        bool ok;
        uint32_t w = counted_mutex_enter_block_until(&test_mutex, deadline, &ok);
        if (w > worst_waits) worst_waits = w;
        latency_add(&lat, absolute_time_diff_us(deadline, get_absolute_time()));
        if (ok) {
            mutex_exit(&test_mutex);
            harness_record("D2.8", RESULT_FAIL, "acquired a lock held for the whole deadline");
            return;
        }
#if INTEROP_HAS_SDK_CORE && INTEROP_HAS_SCHEDULER
        /* the agent releases anything it managed to acquire itself */
        if (!agent_wait(TIMEOUT_MS * 4)) {
            harness_record("D2.8", RESULT_FAIL, "other core's timed acquire never returned");
            return;
        }
#endif
        plat_delay_ms(TIMEOUT_MS * 3);
    }
    latency_print(&lat, "repeated-deadline overshoot");

    if (lat.early) {
        harness_record("D2.8", RESULT_FAIL, "%u timeouts fired EARLY", lat.early);
    } else if (lat.max_us > 10000) {
        harness_record("D2.8", RESULT_FAIL, "overshoot %lldus", (long long)lat.max_us);
    } else if (worst_waits > MAX_BLOCKING_WAITS) {
        harness_record("D2.8", BUSY_POLL_VERDICT,
                       "timeouts accurate (max %lldus) but busy-polled: %u wait iterations",
                       (long long)lat.max_us, worst_waits);
    } else {
        harness_record("D2.8", RESULT_PASS, "never early, overshoot max %lldus, %u waits",
                       (long long)lat.max_us, worst_waits);
    }
}

/* ==================================================================================== */

static void test_body(void) {
    printf("\n=== pico-sdk sync interop test ===\n");
#ifdef PICO_PROCESSOR_NAME
    printf("cpu: " ## PICO_PROCESSOR_NAME "\n");
#endif
    printf("platform: %s\n", PICO_PLATFORM_STRING);
    printf("config: %s\n", plat_config_name());
    printf("spin locks: %s, unlock %s the event\n",
           PICO_USE_SW_SPIN_LOCKS ? "software" : "hardware",
           INTEROP_UNLOCK_SEVS ? "sets" : "does not set");
    if (INTEROP_LOCK_MACROS_OVERRIDDEN) {
        /* Say this plainly: it means the SDK's WFE machinery, workaround and all, is not the
         * code under test here - the RTOS port's event-group implementation is. */
        printf("lock_core: overridden by the RTOS, so the SDK's RP2350 workaround is NOT"
               " compiled; pico_time uses its sleep_notifier\n");
    } else {
        printf("lock_core: the SDK's own; RP2350 workaround %s; pico_time sleeps on a bare"
               " __wfe()\n", INTEROP_WORKAROUND ? "enabled" : "disabled");
    }
#if !PICO_TIME_DEFAULT_ALARM_POOL_DISABLED
    /* Which core services the default pool decides whether an alarm callback notifies from
     * the RTOS core or has to cross over to it - the difference between the _core0 and
     * _core1 configurations. */
    printf("default alarm pool: core %u\n", alarm_pool_core_num(alarm_pool_get_default()));
#endif

    /* DWT is per core: the driving core needs its own enable, or SLEEPCNT here reads a dead
     * value and every local sleep looks like a busy-wait */
    harness_cycles_enable_this_core();
    harness_spinner_start();
    plat_delay_ms(20);
    harness_calibrate();
#if INTEROP_HAS_SDK_CORE
    /* the cycle counter is only ever used to judge the bare-SDK core, and on Arm SysTick
     * belongs to FreeRTOS, so it is calibrated over there rather than here */
    plat_calibrate_agent_cycles();
#endif
    harness_print_calibration();
    rescue_init();

    printf("\n--- D1: blocking discipline (mutex_enter_blocking, as pico_malloc) ---\n");
    d0_counter_selftest();
    d1_1_uncontended();
    d1_2_contended();
    d1_3_priority_asymmetric();
    d1_4_sdk_core_waits();
    d1_5_sdk_core_notifies();
    d1_6_notify_from_isr();
    d1_7_blocking_acquire_from_isr();
    d1_8_recursive();
    d1_9_bare_wfe_pattern();

    printf("\n--- D2: try-then-timed discipline (mutex_try_enter_block_until, as pico_stdio) ---\n");
    d2_1_uncontended_try();
    d2_2_contended_released();
    d2_3_contended_times_out();
    d2_4_contended_by_self();
    d2_5_timed_from_isr_contended();
    d2_6_try_from_isr_uncontended();
    d2_7_sdk_core_timed();
    d2_8_repeated_deadline();
    d2_9_expired_deadline();

    printf("\n--- D3: sleep_ms / sleep_until (ms-scale, concurrent, cross-core) ---\n");
    d3_1_sleep_accuracy_ms();
    d3_2_concurrent_sleepers();
    d3_3_sleep_on_sdk_core();

    int failed = harness_summary();
    printf("%s\n", failed ? "FAILED" : "PASSED");
    stdio_deinit_all();
}

int main(void) {
    stdio_init_all();
    plat_init();
    plat_run(test_body);
}
