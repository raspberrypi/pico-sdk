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
 *   _baseline  no RTOS; core 0 drives, core 1 holds/agents
 *   _smp       FreeRTOS SMP; both cores are FreeRTOS, so cross-core cases skip
 *   _core0     FreeRTOS on core 0, bare SDK code on core 1
 *   _core1     FreeRTOS on core 1, bare SDK code on core 0
 */

#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/sync.h"
#include "pico/lock_core.h"
#include "pico/time.h"

#include "interop_platform.h"
#include "interop_harness.h"

/* D1.7 and D2.5 perform a blocking lock_core wait from ISR context. That is legal bare, but
 * the FreeRTOS port forbids it with configASSERT(!portCHECK_IF_IN_ISR()), which aborts the
 * whole run - so under FreeRTOS they are opt-in and must be run in isolation. */
#ifndef INTEROP_RUN_ISR_BLOCKING
#define INTEROP_RUN_ISR_BLOCKING (!INTEROP_HAVE_FREERTOS)
#endif

/*
 * A bare-SDK core that busy-polls instead of blocking is *predicted* under FreeRTOS: the
 * port's !portIS_FREE_RTOS_CORE() branch is a plain spin_unlock(); __wfe(), with no
 * equivalent of lock_internal_notify_count, so on RP2350 with software spin locks the WFE
 * falls straight through. In the baseline there is no such excuse - the SDK's own workaround
 * is meant to handle exactly this - so there it is a hard failure and must fail the run.
 */
#if INTEROP_HAVE_FREERTOS
#define BUSY_POLL_VERDICT RESULT_EXPECTED_FAIL
#else
#define BUSY_POLL_VERDICT RESULT_FAIL
#endif

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

#define HOLD_MS     20      /* how long a holder holds a contended lock */
#define ITERS       20      /* contended iterations per case */
#define TIMEOUT_MS  50      /* deadline used by the D2 timed cases */

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
    sem_reset(&test_sem, 0);

    for (uint i = 0; i < ITERS; i++) {
        uint32_t delay_us = 2000 + (i % 7) * 250;
        absolute_time_t target = make_timeout_time_us(delay_us);
        if (add_alarm_in_us(delay_us, sem_release_alarm, NULL, true) <= 0) {
            harness_record("D1.6", RESULT_FAIL, "could not add alarm");
            return;
        }
        sem_acquire_blocking(&test_sem);
        latency_add(&lat, absolute_time_diff_us(target, get_absolute_time()));
    }
    latency_print(&lat, "ISR notify -> waiter wake");

    if (lat.count != ITERS) {
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
    add_alarm_in_us(1000, blocking_acquire_in_isr, NULL, true);
    plat_delay_ms(HOLD_MS * 3);
    harness_record("D1.7", d1_7_returned ? RESULT_PASS : RESULT_FAIL,
                   "blocking acquire from ISR %s",
                   d1_7_returned ? "completed" : "never returned");
#endif
}

static void d0_sleepcnt_selftest(void) {
#if !INTEROP_HAS_SDK_CORE
    harness_record("D0", RESULT_SKIP, "no bare-SDK core in this configuration");
#else
    /* Isolates instrumentation from hardware: one known 20ms WFE on the agent core,
     * measured inline AND at the command boundary. */
    if (!harness_sleep_counter_present()) {
        harness_record("D0", RESULT_SKIP, "no DWT sleep counter on this platform");
        return;
    }
    if (!agent_run(AGENT_KNOWN_SLEEP, 20, 2000)) {
        harness_record("D0", RESULT_FAIL, "known-sleep probe never returned");
        return;
    }
    uint32_t inl = agent_inline_sleep_delta();
    uint32_t bnd = agent_sleep_delta();
    if (inl && !bnd) {
        harness_record("D0", RESULT_INFO,
                       "SLEEPCNT inline d=%u but boundary d=0 - the boundary read is at"
                       " fault, not the counter", inl);
    } else if (!inl && !bnd) {
        harness_record("D0", RESULT_INFO,
                       "SLEEPCNT saw nothing for a known 20ms WFE (inline and boundary both"
                       " 0) - counter not counting this WFE");
    } else {
        harness_record("D0", RESULT_INFO, "SLEEPCNT known 20ms WFE: inline d=%u, boundary d=%u",
                       inl, bnd);
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
    add_alarm_in_us(1000, timed_acquire_in_isr, NULL, true);
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
    add_alarm_in_us(1000, try_in_isr_uncontended, NULL, true);
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
    printf("config: %s\n", plat_config_name());
    printf("spin locks: %s, unlock %s the event; workaround %s\n",
           PICO_USE_SW_SPIN_LOCKS ? "software" : "hardware",
           INTEROP_UNLOCK_SEVS ? "sets" : "does not set",
           INTEROP_WORKAROUND ? "enabled" : "disabled");

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

    printf("\n--- D1: blocking discipline (mutex_enter_blocking, as pico_malloc) ---\n");
    d0_sleepcnt_selftest();
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
