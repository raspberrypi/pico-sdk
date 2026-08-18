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
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/sync.h"
#include "pico/lock_core.h"
#include "pico/time.h"
#include "hardware/clocks.h"
#include "pico/time_adapter.h"

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
        /* Print the detail as its own lines - the verdict has to fit in detail[200], and
         * these numbers are what actually localise the fault. */
        if (agent_run(AGENT_ARM_PROBE, 0, 1000)) {
            uint abit2 = 1u << alarm_pool_timer_alarm_num(alarm_pool_get_default());
            bool armed_now  = agent_probe_armed() & abit2;
            bool armed_late = agent_probe_armed_late() & abit2;
            bool asserted   = agent_probe_ints() & abit2;
            bool still_up   = agent_probe_ints_late() & abit2;
            printf("    arm probe on the bare core: add returned %ld, armed=0x%x->0x%x,"
                   " alarm=%u now=%u, intf=0x%x->0x%x ints=0x%x->0x%x\n",
                   (long)agent_probe_id(), agent_probe_armed(), agent_probe_armed_late(),
                   agent_probe_alarm(), agent_probe_now(),
                   agent_probe_intf(), agent_probe_intf_late(),
                   agent_probe_ints(), agent_probe_ints_late());
            printf("    bare core masks: PRIMASK=0x%x BASEPRI=0x%x (timer irq priority"
                   " 0x%x, configMAX_SYSCALL 16 -> BASEPRI>=1 masks it)\n",
                   agent_probe_primask(), agent_probe_basepri(), PICO_DEFAULT_IRQ_PRIORITY);
            printf("    -> %s\n",
                   armed_now || armed_late
                       ? "the add does arm it; something clears it later"
                   : still_up
                       ? "IRQ asserted and still asserted 200us later, though enabled on this"
                         " core - the core is not taking it (masked, or no handler vectored)"
                   : asserted
                       ? "IRQ was asserted then cleared, but nothing was armed - something"
                         " serviced it without doing the pool's work"
                       : "the force never reached INTS - ta_force_irq() is being defeated");
        } else {
            printf("    arm probe on the bare core: the agent did not answer\n");
        }
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
        /* PASS either way: this case is a control, and confirming its prediction is a
         * success whichever way the prediction went. It is not an expected *failure* - the
         * naive pattern busy-polling where a spin unlock sets the calling core's own event
         * is the correct observation, and is the whole reason the workaround exists. */
        harness_record("D1.9", RESULT_PASS,
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
    /* Repeated waits on an already-expired deadline. Every other D2 case takes a fresh future
     * deadline each iteration, so none of them ever waits on one already in the past. Bounded
     * via the agent so a regression reports rather than hangs (pico_sync_test catches this
     * too, but by hanging). */
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

/* D2.11 timings, in ms. The interfering alarm must land after the agent's first call has
 * armed the deadline and fire well before it. */
#define POLL_DEADLINE_MS     20   /* D2.12 */
/* Which core owns the pool IRQ relative to the waiter decides whether an add is
 * synchronous, so every D2.12 result names it - the number means something different
 * in each regime, and "non-alarm core" is plainly wrong in the reversed builds. */
#define REGIME(local) ((local) ? "pool IRQ local, adds synchronous" \
                               : "pool IRQ on the other core, adds async")
#define STOLEN_DEADLINE_MS   20
#define STOLEN_INJECT_AT_MS   8   /* when the test core adds the interfering alarm */
#define STOLEN_INJECT_IN_MS   4   /* how far ahead it sets it, so it fires at 12ms of 20 */

static volatile int64_t stolen_fired_at_us;

static int64_t stolen_interferer(__unused alarm_id_t id, __unused void *ud) {
    stolen_fired_at_us = (int64_t)to_us_since_boot(get_absolute_time());
    return 0;
}

static void d2_11_stolen_wakeup(void) {
#if !INTEROP_HAS_SDK_CORE
    harness_record("D2.11", RESULT_SKIP, "no bare-SDK core in this configuration");
#else
    /*
     * pico-sdk#3124 - a repeated deadline whose wakeup is spent by somebody else's earlier
     * alarm. See AGENT_STOLEN_WAKEUP for the interleaving; here we play the part TinyUSB
     * plays in the report, adding one earlier alarm from this core once the agent's loop is
     * already running.
     *
     * Note this is the OPPOSITE precondition to D2.9: that case repeats a deadline already in
     * the past, this one repeats a live deadline that nothing is armed for.
     */
    stolen_fired_at_us = 0;
    absolute_time_t t_start = get_absolute_time();

    agent_start(AGENT_STOLEN_WAKEUP, STOLEN_DEADLINE_MS);

    /* busy_wait, not sleep: sleeping would put our own alarms in the pool and change what the
     * handler re-arms to, which is the very thing under test */
    busy_wait_until(delayed_by_ms(t_start, STOLEN_INJECT_AT_MS));
    alarm_id_t interferer = add_alarm_in_ms(STOLEN_INJECT_IN_MS, stolen_interferer, NULL, true);

    if (!agent_wait(STOLEN_DEADLINE_MS * STOLEN_FAR_MULTIPLE * 4)) {
        harness_record("D2.11", RESULT_FAIL,
                       "wait on a %ums deadline never returned at all", STOLEN_DEADLINE_MS);
        if (interferer > 0) cancel_alarm(interferer);
        return;
    }
    if (interferer > 0) cancel_alarm(interferer);

    uint32_t elapsed_us = agent_elapsed_us();
    int64_t  fired_at   = stolen_fired_at_us;
    int64_t  fired_off  = fired_at ? fired_at - (int64_t)to_us_since_boot(t_start) : 0;

    /* Controls first: if the interference did not actually land inside the window, the case
     * proves nothing and must say so rather than passing. */
    if (!agent_stolen_far_armed()) {
        harness_record("D2.11", RESULT_SKIP,
                       "could not queue an alarm beyond the deadline - pool full?");
        return;
    }
    if (!fired_at || fired_off <= 0 || fired_off >= STOLEN_DEADLINE_MS * 1000) {
        harness_record("D2.11", RESULT_SKIP,
                       "interfering alarm did not fire inside the window (at %lldus of %ums)",
                       (long long)fired_off, STOLEN_DEADLINE_MS);
        return;
    }

    if (elapsed_us < STOLEN_DEADLINE_MS * 1000u) {
        harness_record("D2.11", RESULT_FAIL,
                       "returned EARLY: %uus against a %ums deadline",
                       elapsed_us, STOLEN_DEADLINE_MS);
    } else if (elapsed_us > STOLEN_DEADLINE_MS * 1000u + STOLEN_SLACK_US) {
        harness_record("D2.11", RESULT_FAIL,
                       "overslept %ums deadline by %uus (earlier alarm at %lldus spent its"
                       " wakeup; %u iterations)",
                       STOLEN_DEADLINE_MS, elapsed_us - STOLEN_DEADLINE_MS * 1000u,
                       (long long)fired_off, agent_poll_iterations());
    } else if (agent_poll_iterations() > STOLEN_MAX_WAITS) {
        /* Meeting the deadline is not enough: a fix which re-adds an alarm every pass would
         * arrive on time by spinning, and elapsed time cannot tell that from sleeping. Same
         * hole D2.3/D2.8 had. */
        harness_record("D2.11", RESULT_FAIL,
                       "met the %ums deadline but BUSY-WAITED to it - %u iterations against a"
                       " ceiling of %u",
                       STOLEN_DEADLINE_MS, agent_poll_iterations(), STOLEN_MAX_WAITS);
    } else {
        harness_record("D2.11", RESULT_PASS,
                       "%ums deadline met by sleeping (%uus, %u iterations) with an earlier"
                       " alarm firing at %lldus",
                       STOLEN_DEADLINE_MS, elapsed_us, agent_poll_iterations(),
                       (long long)fired_off);
    }
#endif
}

static void d2_12_poll_fixed_deadline(void) {
#if !INTEROP_HAS_SDK_CORE
    harness_record("D2.12", RESULT_SKIP, "no bare-SDK core in this configuration");
#else
    /*
     * pico-sdk#3039, fixed by having the pool arm before it processes cancellations, and
     * unpinned until now. Poll one fixed
     * deadline from a core that does NOT own the alarm pool's IRQ, with nothing else going
     * on. add_alarm_at() is asynchronous from such a core - it only forces the IRQ, and the
     * owning core programs the hardware - so a wait predicate consulting the hardware alone
     * can re-add and re-cancel an alarm that never gets armed, and the loop busy-waits.
     *
     * The iteration count is the criterion, not the elapsed time: both a sleeping and a
     * spinning loop leave at the deadline, which is exactly why this went unpinned.
     *
     * Establishing the precondition matters as much as the measurement - if the pool's IRQ
     * turns out to be live on the agent core, the add is synchronous and the case is testing
     * nothing, so it skips rather than passing.
     */
    /* Which regime this ran in is reported, NOT used to decide whether to run. Skipping when
     * the pool's IRQ is live on the agent core would encode the very belief under test - that
     * synchronous arming makes #3039 impossible there - so the one configuration able to
     * refute it would be the one that never looks. Bounded by agent_run() either way, so
     * running it costs nothing but a few ms. */
    bool irq_local = agent_run(AGENT_IRQ_STATE, 0, 1000) && agent_alarm_irq_enabled();
    if (!agent_run(AGENT_POLL_DEADLINE, POLL_DEADLINE_MS, POLL_DEADLINE_MS * 20)) {
        harness_record("D2.12", RESULT_FAIL,
                       "polling a %ums deadline never returned (%s)", POLL_DEADLINE_MS,
                       irq_local ? "pool IRQ local" : "pool IRQ on the other core");
        return;
    }
    uint32_t elapsed_us = agent_elapsed_us();
    if (elapsed_us < POLL_DEADLINE_MS * 1000u) {
        harness_record("D2.12", RESULT_FAIL, "returned EARLY: %uus against %ums",
                       elapsed_us, POLL_DEADLINE_MS);
    } else if (agent_poll_iterations() > STOLEN_MAX_WAITS) {
        harness_record("D2.12", RESULT_FAIL,
                       "BUSY-WAITED a %ums deadline (%s) - %u iterations against a ceiling"
                       " of %u; the alarm is not being armed",
                       POLL_DEADLINE_MS, REGIME(irq_local), agent_poll_iterations(),
                       STOLEN_MAX_WAITS);
    } else {
        harness_record("D2.12", RESULT_PASS,
                       "slept a %ums deadline (%s): %uus, %u iterations",
                       POLL_DEADLINE_MS, REGIME(irq_local), elapsed_us,
                       agent_poll_iterations());
    }
#endif
}

/* D2.13 - sustained interference. Deliberately one-shot and spaced, not a repeating timer:
 * a repeating timer would keep something armed before the deadline at all times, so coverage
 * would never lapse and nothing would be re-added. The cost being measured only appears when
 * coverage repeatedly lapses and has to be restored. */
#define INTERFERE_DEADLINE_MS 20
#define INTERFERE_PERIOD_US   2000
#define INTERFERE_LEAD_US      500

static volatile uint32_t interferer_fires;

static int64_t interferer_cb(__unused alarm_id_t id, __unused void *ud) {
    interferer_fires++;
    return 0;
}

static void d2_13_sustained_interference(void) {
#if !INTEROP_HAS_SDK_CORE
    harness_record("D2.13", RESULT_SKIP, "no bare-SDK core in this configuration");
#else
    /*
     * Measurement, not a verdict - hence RESULT_INFO. It exists to compare candidate fixes
     * for pico-sdk#3124 against each other, and the number that matters is the RATIO of wait
     * iterations to interferer fires.
     *
     * Every fire wakes the waiter, so one iteration per fire is the floor for any
     * implementation. What differs is whether the waiter must also re-add its own alarm each
     * time coverage lapses: on a build where a spin unlock sets the calling core's own event
     * (PICO_SPIN_LOCK_UNLOCK_CAUSES_SEV), the __wfe() after an add is eaten immediately, so an
     * add costs an EXTRA pass that does not sleep. So ~1x fires means the waiter kept its own
     * coverage; ~2x means it is re-adding on every lapse.
     *
     * This is the cost the fable review predicted for the `&&` shape and could not measure.
     */
    interferer_fires = 0;
    agent_start(AGENT_POLL_DEADLINE, INTERFERE_DEADLINE_MS);

    absolute_time_t stop = make_timeout_time_ms(INTERFERE_DEADLINE_MS);
    uint32_t added = 0, add_failures = 0;
    while (!time_reached(stop)) {
        /* busy_wait, never sleep_*: sleeping would use the machinery under test */
        busy_wait_us(INTERFERE_PERIOD_US);
        if (add_alarm_in_us(INTERFERE_LEAD_US, interferer_cb, NULL, true) > 0) added++;
        else add_failures++;
    }

    if (!agent_wait(INTERFERE_DEADLINE_MS * 20)) {
        harness_record("D2.13", RESULT_FAIL,
                       "polling a %ums deadline under interference never returned",
                       INTERFERE_DEADLINE_MS);
        return;
    }
    uint32_t fires = interferer_fires;
    if (!added || !fires) {
        harness_record("D2.13", RESULT_SKIP,
                       "no interference landed (%u added, %u failed, %u fired)",
                       added, add_failures, fires);
        return;
    }
    harness_record("D2.13", RESULT_INFO,
                   "%u iterations for %u interferer fires (%u.%02ux) over %uus",
                   agent_poll_iterations(), fires,
                   agent_poll_iterations() / fires,
                   (agent_poll_iterations() * 100u / fires) % 100u,
                   agent_elapsed_us());
#endif
}

/* D2.14 - a deadline polled while a single much LATER alarm sits in the pool, and nothing else
 * is queued. Reuses AGENT_STOLEN_WAKEUP, which queues exactly that alarm and polls the
 * deadline; D2.11 is the same op with interference added on top.
 *
 * This case exists to pin a property that is easy to get backwards: the later alarm PREVENTS
 * the pico-sdk#2706 busy-poll rather than causing it. Measured by restoring the old handler
 * ordering, where the pool processed cancellations before arming - D2.7 and D2.12, which have
 * no later alarm, busy-polled 7e3 to 1.7e4 times, while this case and D2.11 slept throughout.
 *
 * The reason is that a live later entry keeps the handler's arming path reachable: it loops
 * past the freed cancellation and calls ta_set_timeout() again, which the "never move the
 * timeout later" guard then refuses while our own deadline is still armed and unfired - so the
 * register goes on covering us. With an empty pool the handler breaks out at `earliest_index <
 * 0` before reaching the arming at all, and the deadline is left uncovered.
 *
 * It also explains #2706's reporter finding that adding a second timer at ~50ms made their
 * stall disappear, which no other account of that bug predicts.
 */
/* D2.15 - a burst of cancellations issued from the other core.
 *
 * Cancelling several alarms before the pool's core can scan them is the one case where the head
 * of its ordered list is cancelled alongside entries behind it. That is what it takes to reach
 * the scan's linking, and a mistake there loses entries into neither the ordered list nor the
 * free list, where nothing ever recovers them.
 *
 * The agent both adds and cancels, so where its core does not own the pool the cancels are only
 * requests and several land before the handler runs. Where it does own the pool the handler runs
 * between them and no batch forms; the case still runs, and reports which regime it saw, on the
 * same reasoning as D2.12 - skipping the configuration that cannot reproduce it would encode the
 * belief being tested.
 *
 * The detector is that the agent can keep adding: an entry lost per round retires the pool a
 * little at a time, so a later round fails to place the whole burst. No timing is involved.
 */
static void d2_15_cancel_burst(void) {
#if !INTEROP_HAS_SDK_CORE
    harness_record("D2.15", RESULT_SKIP, "no bare-SDK core in this configuration");
#else
    bool irq_local = agent_run(AGENT_IRQ_STATE, 0, 1000) && agent_alarm_irq_enabled();
    for (uint round = 0; round < CANCEL_BURST_ROUNDS; round++) {
        if (!agent_run(AGENT_CANCEL_BURST, CANCEL_BURST_ALARMS, TIMEOUT_MS * 20)) {
            harness_record("D2.15", RESULT_FAIL,
                           "round %u: the agent never returned from the burst (%s)",
                           round, REGIME(irq_local));
            return;
        }
        if (agent_burst_added() != CANCEL_BURST_ALARMS) {
            if (!round) {
                harness_record("D2.15", RESULT_SKIP,
                               "pool only took %u of %u alarms, so there is no batch to test"
                               " with", agent_burst_added(), CANCEL_BURST_ALARMS);
            } else {
                harness_record("D2.15", RESULT_FAIL,
                               "round %u placed only %u of %u alarms - the pool is losing"
                               " entries to the cancellations (%s)",
                               round, agent_burst_added(), CANCEL_BURST_ALARMS,
                               REGIME(irq_local));
            }
            return;
        }
        /* let the pool retire the round before starting the next, so a leak shows as a smaller
         * pool rather than as entries still in flight */
        sleep_ms(2);
    }
    harness_record("D2.15", RESULT_PASS,
                   "%u rounds of %u cancellations, pool still places every one (%s)",
                   CANCEL_BURST_ROUNDS, CANCEL_BURST_ALARMS, REGIME(irq_local));
#endif
}

static void d2_14_far_alarm_only(void) {
#if !INTEROP_HAS_SDK_CORE
    harness_record("D2.14", RESULT_SKIP, "no bare-SDK core in this configuration");
#else
    if (!agent_run(AGENT_STOLEN_WAKEUP, STOLEN_DEADLINE_MS,
                   STOLEN_DEADLINE_MS * STOLEN_FAR_MULTIPLE * 4)) {
        harness_record("D2.14", RESULT_FAIL,
                       "polling a %ums deadline with a later alarm queued never returned",
                       STOLEN_DEADLINE_MS);
        return;
    }
    if (!agent_stolen_far_armed()) {
        harness_record("D2.14", RESULT_SKIP,
                       "could not queue the later alarm - pool full?");
        return;
    }
    uint32_t elapsed_us = agent_elapsed_us();
    if (elapsed_us < STOLEN_DEADLINE_MS * 1000u) {
        harness_record("D2.14", RESULT_FAIL, "returned EARLY: %uus against %ums",
                       elapsed_us, STOLEN_DEADLINE_MS);
    } else if (elapsed_us > STOLEN_DEADLINE_MS * 1000u + STOLEN_SLACK_US) {
        harness_record("D2.14", RESULT_FAIL, "overslept %ums deadline by %uus",
                       STOLEN_DEADLINE_MS, elapsed_us - STOLEN_DEADLINE_MS * 1000u);
    } else if (agent_poll_iterations() > STOLEN_MAX_WAITS) {
        harness_record("D2.14", RESULT_FAIL,
                       "BUSY-WAITED a %ums deadline with only a later alarm queued - %u"
                       " iterations against a ceiling of %u; this is #2706",
                       STOLEN_DEADLINE_MS, agent_poll_iterations(), STOLEN_MAX_WAITS);
    } else {
        harness_record("D2.14", RESULT_PASS,
                       "slept a %ums deadline with a later alarm queued (%uus, %u iterations)",
                       STOLEN_DEADLINE_MS, elapsed_us, agent_poll_iterations());
    }
#endif
}

/* D4.1 - the alarm pool must leave its hardware alarm armed even when it holds nothing.
 *
 * ta_wakes_up_on_or_before() reads only the compare register, not the ARMED bit, so the whole
 * design rests on an alarm always being armed: the handler's own comment says it is "leaving a
 * timeout every 2^32 microseconds anyway". That was not true once the pool emptied - the
 * hardware clears ARMED when an alarm fires, and the handler returned at `earliest_index < 0`
 * without calling ta_set_timeout(), so the register kept a stale value with nothing pending.
 * The predicate then answers "a wakeup is due before then" for any target more than 2^32us out,
 * and best_effort_wfe_or_timeout() waits for an event that never comes.
 *
 * Tested as the invariant rather than the symptom: the symptom needs a deadline over 71 minutes
 * away, the invariant is one register bit and takes a couple of milliseconds. Uses local_pool
 * so the default pool's own traffic (stdio and friends) cannot keep it accidentally armed and
 * hide the failure.
 */
static volatile bool d4_alarm_fired;

static int64_t d4_alarm_cb(__unused alarm_id_t id, __unused void *ud) {
    d4_alarm_fired = true;
    return 0;
}

static void d4_1_pool_leaves_alarm_armed(void) {
    if (!local_pool) {
        harness_record("D4.1", RESULT_SKIP, "no private alarm pool available");
        return;
    }
    uint alarm_num = alarm_pool_hardware_alarm_num(local_pool);
    d4_alarm_fired = false;
    if (alarm_pool_add_alarm_in_us(local_pool, 2000, d4_alarm_cb, NULL, true) <= 0) {
        harness_record("D4.1", RESULT_SKIP, "could not add an alarm to the private pool");
        return;
    }
    /* busy_wait, never sleep_*: sleeping would put alarms in the default pool, not this one,
     * but it also runs the very machinery under test */
    absolute_time_t bail = make_timeout_time_ms(200);
    while (!d4_alarm_fired && !time_reached(bail)) tight_loop_contents();
    if (!d4_alarm_fired) {
        harness_record("D4.1", RESULT_FAIL, "private pool alarm never fired");
        return;
    }
    busy_wait_us(2000);   /* let the handler finish emptying the pool */

    uint32_t armed = timer_hw_from_timer(alarm_pool_get_default_timer())->armed;
    if (armed & (1u << alarm_num)) {
        harness_record("D4.1", RESULT_PASS,
                       "pool left hardware alarm %u armed after emptying (armed=0x%02x)",
                       alarm_num, armed);
    } else {
        harness_record("D4.1", RESULT_FAIL,
                       "pool left hardware alarm %u DISARMED after emptying (armed=0x%02x) -"
                       " ta_wakes_up_on_or_before() can now report a wakeup that will not happen",
                       alarm_num, armed);
    }
}

static void d2_10_subtick_deadline(void) {
#if !INTEROP_HAS_SCHEDULER
    harness_record("D2.10", RESULT_SKIP,
                   "needs a scheduler: a sub-tick deadline only means something against a tick");
#else
    /*
     * A deadline shorter than one tick. prvGetTicksToWaitBefore() then returns 0, so the port
     * cannot block on the event group at all and falls through to spin_unlock() + portYIELD(),
     * leaving the SDK's caller to loop. That spins at *this task's* priority, which starves
     * every lower-priority task on this core - a busy-wait that costs more than power.
     *
     * Every other D2 case uses TIMEOUT_MS = 50, i.e. fifty ticks, so none of them reach this
     * branch. Short timeouts are not exotic - pico_stdio takes them - and this is where
     * "sub-tick precision under FreeRTOS" and "task starvation" turn out to be one question.
     *
     * Repeated rather than measured once: the spinner rate is per millisecond, so a single
     * 300us window cannot be resolved.
     */
    const uint32_t deadline_us = 300;
    plat_hold_for_ms(HOLD_MS);
    uint32_t sp0 = harness_spinner_count();
    absolute_time_t t0 = get_absolute_time();
    uint iters = 0, waits = 0;
    bool got = false;
    while (absolute_time_diff_us(t0, get_absolute_time()) < (int64_t)(HOLD_MS - 4) * 1000) {
        waits += counted_mutex_enter_block_until(&test_mutex,
                                                 make_timeout_time_us(deadline_us), &got);
        iters++;
        if (got) {                 /* holder let go early; nothing more to measure */
            mutex_exit(&test_mutex);
            break;
        }
    }
    int64_t us = absolute_time_diff_us(t0, get_absolute_time());
    int y = harness_yield_pct(harness_spinner_count() - sp0, (uint32_t)(us / 1000));

    if (got) {
        harness_record("D2.10", RESULT_INFO,
                       "holder released early after %u sub-tick attempts; not measured", iters);
    } else if (y < 0) {
        harness_record("D2.10", RESULT_PASS,
                       "%u sub-tick (%uus) deadlines, %u waits over %lldus (no yield metric)",
                       iters, deadline_us, waits, (long long)us);
    } else if (y < 25) {
        /* INFO, not FAIL: the port cannot block for less than a tick, so spinning here buys
         * sub-tick precision at the cost of the lower-priority tasks on this core. That is a
         * design trade-off to decide on, not a defect to assert - but it must be visible. */
        harness_record("D2.10", RESULT_INFO,
                       "%u sub-tick (%uus) deadlines spun at task priority: yield %d%%, %u"
                       " waits over %lldus - lower-priority tasks starved throughout",
                       iters, deadline_us, y, waits, (long long)us);
    } else {
        harness_record("D2.10", RESULT_PASS,
                       "%u sub-tick (%uus) deadlines, yield %d%%, %u waits over %lldus",
                       iters, deadline_us, y, waits, (long long)us);
    }
#endif
}

/*
 * D3.4 - event-bit aliasing against the sleep_notifier (Q5a).
 *
 * The FreeRTOS ports multiplex every SDK spin lock onto one event group:
 *
 *     32-bit ticks: bit = 1u << (spin_lock_num % 24)
 *     16-bit ticks: bit = 1u << (spin_lock_num & 0x7)
 *
 * A waiter uses xClearOnExit=pdTRUE, so it *consumes* the bit. For locks that is harmless -
 * the port's own comment argues any thief must itself unlock, which re-sets the bit. The
 * sleep_notifier is the exception: its notify is one-shot (an alarm callback) and its
 * predicate is the clock, so a stolen bit is not re-set by anything and the sleeper waits for
 * unrelated traffic that may never come.
 *
 * Whether that is reachable at all depends on the arithmetic, so compute it rather than
 * assume it. With 32-bit ticks and 32 spin locks nothing can alias PICO_SPINLOCK_ID_TIMER
 * (it would need lock 10 or 34, and 34 does not exist), so the only possible thief is another
 * sleeper - which is D3.2. With 16-bit ticks locks 2/10/18/26 all collide and the hazard is
 * real.
 *
 * NOTE: this duplicates prvGetEventGroupBit(), which is static inside the port. If the port
 * changes its mapping this case will quietly describe the wrong thing - it reports the
 * mapping it used so that is at least visible in the log.
 */
static uint alias_bit_of(uint lock_num) {
#if INTEROP_HAVE_FREERTOS && ( configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_16_BITS )
    return lock_num & 0x7u;
#else
    return lock_num % 24u;
#endif
}

static void d3_4_notifier_bit_aliasing(void) {
#if !INTEROP_HAS_SCHEDULER
    harness_record("D3.4", RESULT_SKIP, "needs tasks: a thief must block on an aliasing lock");
#else
    const uint timer_lock = PICO_SPINLOCK_ID_TIMER;
    const uint timer_bit  = alias_bit_of(timer_lock);
    uint aliases[8];
    uint n_alias = 0;
    for (uint i = 0; i < (uint)NUM_SPIN_LOCKS && n_alias < count_of(aliases); i++) {
        if (i != timer_lock && alias_bit_of(i) == timer_bit) aliases[n_alias++] = i;
    }

    if (!n_alias) {
        /* Not a skip: the absence is the result, and it is what makes the sleep_notifier
         * safe from everything except another sleeper in this configuration. */
        harness_record("D3.4", RESULT_PASS,
                       "no lock of %d aliases the notifier's bit %u (timer lock %u): the only"
                       " possible thief is another sleeper, which is D3.2",
                       NUM_SPIN_LOCKS, timer_bit, timer_lock);
        return;
    }

    /* A thief exists: contend on it hard while sleeping, so its waiters keep consuming the
     * shared bit, and see whether any sleep is left stranded. */
    plat_start_bit_thief(aliases[0]);
    latency_t lat;
    latency_reset(&lat);
    for (uint i = 0; i < 40; i++) {
        absolute_time_t target = make_timeout_time_ms(2);
        sleep_ms(2);
        latency_add(&lat, absolute_time_diff_us(target, get_absolute_time()));
    }
    plat_stop_bit_thief();
    latency_print(&lat, "2ms sleeps while an aliasing lock is contended");

    if (lat.early) {
        harness_record("D3.4", RESULT_FAIL, "sleep_ms returned EARLY %u times", lat.early);
    } else if (lat.max_us > 50000) {
        harness_record("D3.4", RESULT_FAIL,
                       "a 2ms sleep overslept by %lldus with lock %u contending bit %u - the"
                       " notifier's wakeup was consumed by an aliasing waiter",
                       (long long)lat.max_us, aliases[0], timer_bit);
    } else {
        harness_record("D3.4", RESULT_PASS,
                       "%u lock(s) alias bit %u; worst oversleep %lldus under contention",
                       n_alias, timer_bit, (long long)lat.max_us);
    }
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
    } else if (harness_sleep_counter_usable() && slept_count == 0 && INTEROP_HAS_SCHEDULER) {
        /* Not a fault, and not a doubtful signal either: under an RTOS sleep_ms blocks the
         * *task*, and with configUSE_TICKLESS_IDLE=0 the idle task spins, so the core is
         * never idled. SLEEPCNT is right to see no sleep. */
        harness_record("D3.1", RESULT_PASS,
                       "never early, worst oversleep %lldus; no core sleep, as expected with"
                       " the task blocked and a spinning idle task", (long long)lat.max_us);
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

    /*
     * Sleep durations just above the busy-wait threshold, swept.
     *
     * sleep_until() waits until t - PICO_TIME_SLEEP_OVERHEAD_ADJUST_US, so a sleep of about
     * that length leaves its alarm due within microseconds of the wait starting - which is the
     * only way for it to fire while this task is still between its spin unlock and its block.
     * A sleep_ms(2) arms an alarm two milliseconds after the task has already parked, so it
     * cannot reach the window at all; that is what this case used to do, and why it never
     * caught anything.
     *
     * If the notification is stolen the symptom is unmistakable - the sleeper waits for the
     * 500ms sleeper's alarm instead of its own - so unlike the semaphore probe, detection is
     * free and only provocation is hard. It has not been provoked, and probably cannot be from
     * here: the stolen notification has to be this sleeper's *own* alarm (another sleeper's
     * landing in the window is harmless, since our own alarm would still wake us), so t_before
     * must fall inside the few instructions between the wait's spin unlock and its block. The
     * loop only enters the wait if time_reached(t_before) is still false at the top, so the
     * target interval and "the wait is skipped entirely" are adjacent, separated by less than
     * the jitter in add_alarm_at() itself. A caller cannot aim that finely, and the window
     * cannot be widened from outside because lock_internal_spin_unlock_with_wait() performs
     * the unlock and the block as one macro.
     */
    const uint32_t adjust = PICO_TIME_SLEEP_OVERHEAD_ADJUST_US;
    latency_t lat;
    latency_reset(&lat);
    for (uint i = 0; i < 4000; i++) {
        uint32_t us = adjust + 5 + (i % 250);      /* alarm due 5-254us into the wait */
        absolute_time_t target = make_timeout_time_us(us);
        sleep_us(us);
        latency_add(&lat, absolute_time_diff_us(target, get_absolute_time()));
    }
    latency_print(&lat, "short sleeps alongside a 500ms sleeper");

    if (lat.early) {
        harness_record("D3.2", RESULT_FAIL, "sleep_ms returned EARLY %u times", lat.early);
    } else if (lat.max_us > 50000) {
        harness_record("D3.2", RESULT_FAIL,
                       "a %u-%uus sleep overslept %lldus beside a 500ms sleeper - its own"
                       " alarm had already fired, so its wakeup was stolen and it waited for"
                       " the other sleeper's", adjust + 5, adjust + 254,
                       (long long)lat.max_us);
    } else {
        harness_record("D3.2", RESULT_PASS,
                       "4000 sleeps of %u-%uus alongside a 500ms sleeper, worst oversleep"
                       " %lldus", adjust + 5, adjust + 254, (long long)lat.max_us);
    }
#endif
}

static void d3_3_sleep_on_sdk_core(void) {
#if !INTEROP_HAS_SDK_CORE
    harness_record("D3.3", RESULT_SKIP, "no bare-SDK core in this configuration");
#else
    /*
     * The bare-SDK core sleeps while this core hammers the same alarm pool.
     *
     * Repeated, because a single sample cannot support a verdict: DWT_SLEEPCNT is 8 bits and
     * wraps, so any one sleep of an exact multiple of 256 cycles reads back unchanged - about
     * 1/256, and a longer sleep does not help. N independent sleeps all reading unchanged is
     * (1/256)^N, so three rounds put a false "never slept" at about 1 in 16 million. That is
     * enough to fail on, where one sample was only enough to note.
     */
    const uint rounds = 3;
    uint slept_rounds = 0, worst_us = 0;
    for (uint r = 0; r < rounds; r++) {
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
            return;
        }
        if (us > worst_us) worst_us = us;
        if (agent_slept()) slept_rounds++;
    }

    if (worst_us > 60000) {
        harness_record("D3.3", RESULT_FAIL, "50ms sleep on the other core took %uus",
                       worst_us);
    } else if (harness_sleep_counter_usable() && slept_rounds == 0) {
        /* A hard failure, and the same defect D1.4 reports: on a core the RTOS does not
         * schedule, sleep_until() waits via lock_internal_spin_unlock_with_wait() on the
         * sleep_notifier, so a port branch that omits the RP2350 workaround busy-waits the
         * whole sleep. There is no idle task here to excuse it - this core has no scheduler. */
        harness_record("D3.3", RESULT_FAIL,
                       "50ms sleep on the other core took %uus but never slept in %u rounds -"
                       " it busy-waited", worst_us, rounds);
    } else {
        harness_record("D3.3", RESULT_PASS,
                       "50ms sleep on the other core took %uus%s", worst_us,
                       harness_sleep_counter_usable() ? " (confirmed asleep)" : "");
    }
#endif
}

/*
 * D1.10 - two waiters, two releases: can one be stranded with a permit available?
 *
 * The FreeRTOS ports multiplex every SDK spin lock onto one event group, and a waiter consumes
 * the bit (xClearOnExit=pdTRUE). The port argues that is safe because "any intervening blocked
 * lock ... will need to unlock it before we proceed, which will set the event bit again". That
 * holds for a mutex - the thief's mutex_exit() notifies - but not for a semaphore: on success
 * sem_acquire_blocking() takes a plain spin_unlock() (sem.c:32) and never notifies. A waiter
 * that consumes a permit leaves the shared bit clear behind it.
 *
 * Two releases collapse onto one bit, so the damaging interleaving is:
 *
 *   A parked on the event group; B between its spin_unlock() and xEventGroupWaitBits()
 *   (port.c:2584-2586); release, release -> permits=2, bit set once; A wakes, clears the bit,
 *   takes a permit; B then parks on a bit that is now clear, with permits=1 and nothing left
 *   to notify it.
 *
 * That window is a few instructions wide, so it must be *scanned* rather than hoped for: the
 * releases are issued from an alarm at a swept offset, while B is started just before this task
 * blocks (so B runs, and the alarm lands somewhere inside its acquire). An earlier version of
 * this case simply parked both waiters and released twice - which is the safe ordering by
 * construction, and passed whether or not the bug exists. A test that cannot fail is worse than
 * no test, so note what a pass here does and does not mean: not provoking it is weak evidence,
 * exactly as in D3.4.
 */
static void release_twice_from_isr(__unused alarm_id_t id, __unused void *ud) {
    sem_release(&test_sem);
    sem_release(&test_sem);
}

static int64_t release_twice_alarm(alarm_id_t id, void *ud) {
    release_twice_from_isr(id, ud);
    return 0;
}

/* Run by waiter B as its first act, so the releases are timed from B's own execution. */
static volatile uint32_t d1_10_offset_us;
static void d1_10_arm_releases(void) {
    local_alarm_in_us(d1_10_offset_us, release_twice_alarm);
}

static void d1_10_two_waiters_two_releases(void) {
#if !INTEROP_HAS_SCHEDULER
    harness_record("D1.10", RESULT_SKIP,
                   "needs two tasks blocked on one semaphore's event-group bit");
#else
    const uint consumers = 3;              /* >2, so one can be cycling while another parks */
    const uint window_ms = 5;
    /* Dial up for a soak - 300 has been run, and found nothing. */
#ifndef D1_10_SECONDS
#define D1_10_SECONDS 5
#endif
    const uint windows = (D1_10_SECONDS * 1000) / window_ms;
    const uint stall_windows = 3;          /* 15ms of no progress with permits available */

    sem_reset(&test_sem, 0);
    plat_sem_churn_start(consumers);     /* also starts the task-context releaser */

    uint32_t last[3] = { 0, 0, 0 };
    uint stalled[3] = { 0, 0, 0 };
    uint worst_stall = 0;
    uint transients = 0;                   /* stalls that recovered before the threshold */
    uint stalled_idx = 0;
    int16_t stalled_permits = 0;
    for (uint w = 0; w < windows; w++) {
        plat_delay_ms(window_ms);
        for (uint i = 0; i < consumers; i++) {
            uint32_t now = plat_sem_churn_count(i);
            /* A consumer that has not acquired anything for a whole window, while permits are
             * sitting available, is the bug: a release notifies every *parked* waiter, so one
             * that is making no progress with permits there has been missed. */
            if (now == last[i] && sem_available(&test_sem) > 0 && plat_sem_churn_blocked(i)) {
                stalled[i]++;
                if (stalled[i] > worst_stall) {
                    worst_stall = stalled[i];
                    stalled_idx = i;
                    stalled_permits = sem_available(&test_sem);
                }
            } else {
                /* A recovered stall is still the bug happening - it was rescued by a later
                 * release rather than never occurring - so count it rather than discard it. */
                if (stalled[i]) transients++;
                stalled[i] = 0;
            }
            last[i] = now;
        }
        if (worst_stall >= stall_windows) break;
    }
    /* Does one more release get it moving? A lost notification wakes on the next set of the
     * bit; something stuck for another reason does not. This distinguishes the two without
     * guessing, which has not gone well. */
    bool woke_on_poke = false;
    if (worst_stall >= stall_windows) {
        uint32_t before = plat_sem_churn_count(stalled_idx);
        sem_release(&test_sem);
        for (uint i = 0; i < 10 && !woke_on_poke; i++) {
            plat_delay_ms(2);
            woke_on_poke = plat_sem_churn_count(stalled_idx) > before;
        }
    }
    uint32_t total = plat_sem_churn_count(0) + plat_sem_churn_count(1) + plat_sem_churn_count(2);
    bool drained = plat_sem_churn_stop(consumers);

    if (plat_sem_acquirer_create_failed()) {
        harness_record("D1.10", RESULT_FAIL,
                       "could not create the churn tasks (%u bytes of FreeRTOS heap free)",
                       plat_free_heap());
    } else if (worst_stall >= stall_windows) {
        harness_record("D1.10", RESULT_FAIL,
                       "consumer %u made no progress for %ums with %d permits available"
                       " after %u acquires; one more release %s - %s",
                       stalled_idx, worst_stall * window_ms, stalled_permits, total,
                       woke_on_poke ? "woke it" : "did NOT wake it",
                       woke_on_poke ? "a lost notification, as expected"
                                    : "so it is not simply a missed event-group set");
    } else if (!drained) {
        harness_record("D1.10", RESULT_FAIL,
                       "churn tasks did not all exit after %u acquires - one is still blocked"
                       " with permits released to it", total);
    } else {
        harness_record("D1.10", RESULT_PASS,
                       "%u acquires across %u consumers, no confirmed stall (worst %ums,"
                       " %u transient stalls seen - each one a waiter blocked with a permit"
                       " available that a later release rescued)", total, consumers,
                       worst_stall * window_ms, transients);
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
    /* Repeats one deadline from both cores at once. Every wait must respect its deadline. */
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
    d1_10_two_waiters_two_releases();

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
    d2_10_subtick_deadline();
    d2_11_stolen_wakeup();
    d2_12_poll_fixed_deadline();
    d2_13_sustained_interference();
    d2_14_far_alarm_only();
    d2_15_cancel_burst();

    printf("\n--- D3: sleep_ms / sleep_until (ms-scale, concurrent, cross-core) ---\n");
    d3_1_sleep_accuracy_ms();
    d3_2_concurrent_sleepers();
    d3_3_sleep_on_sdk_core();
    d3_4_notifier_bit_aliasing();

    printf("\n--- D4: alarm pool state ---\n");
    d4_1_pool_leaves_alarm_armed();

    int failed = harness_summary();
    printf("%s\n", failed ? "FAILED" : "PASSED");
    stdio_deinit_all();
    // don't spin here: an automated harness would have to wait out its timeout to collect a
    // result that is already known
    exit(failed ? 1 : 0);
}

int main(void) {
    stdio_init_all();
    plat_init();
    plat_run(test_body);
}
