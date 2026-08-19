/*
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * Measurement harness for the FreeRTOS/SDK sync interop tests.
 *
 * Two things need measuring, and both are calibrated in-run against a deliberate
 * busy-wait and a deliberate proper block, so no threshold depends on assumptions about
 * DWT/mcycle behaviour or clock speed. If a calibration fails to separate the two poles,
 * the corresponding metric reports "unavailable" and its assertions are skipped rather
 * than producing numbers nobody should trust.
 *
 *  1. RTOS-level yielding -- did the blocked task give the CPU up, so other work runs?
 *     Measured with a lowest-priority spinner task incrementing a counter.
 *
 *  2. Hardware-level sleeping -- did the core actually enter WFE, or spin?
 *     Measured with the cycle counter (DWT CYCCNT on Arm, mcycle on RISC-V) compared
 *     against wall clock. Not available on RP2040's M0+.
 */

#ifndef _INTEROP_HARNESS_H
#define _INTEROP_HARNESS_H

#include "pico.h"
#include "pico/time.h"

/* ---- result recording ------------------------------------------------------------- */

typedef enum {
    RESULT_PASS = 0,
    RESULT_FAIL,
    RESULT_SKIP,          /* not meaningful in this configuration */
    RESULT_EXPECTED_FAIL, /* known-broken; recorded, does not fail the run */
    RESULT_INFO,          /* measurement only, no pass/fail criterion */
} result_kind_t;

#define MAX_RESULTS 48

void harness_record(const char *id, result_kind_t kind, const char *fmt, ...) __printflike(3, 4);
/* returns non-zero if any RESULT_FAIL was recorded */
int  harness_summary(void);

/* ---- latency ---------------------------------------------------------------------- */

#define LATENCY_BUCKETS 20   /* log2 microsecond buckets: <1, <2, <4, ... */

typedef struct {
    uint32_t count;
    uint32_t early;        /* samples that completed *before* the target - always a bug */
    int64_t  min_us;
    int64_t  max_us;
    int64_t  sum_us;
    uint32_t bucket[LATENCY_BUCKETS];
} latency_t;

void    latency_reset(latency_t *l);
void    latency_add(latency_t *l, int64_t lateness_us);
void    latency_print(const latency_t *l, const char *name);
int64_t latency_mean_us(const latency_t *l);

/* ---- spinner: RTOS-level yield detection ------------------------------------------ */

/* Create the lowest-priority spinner. In SMP it is pinned to the test core. */
void     harness_spinner_start(void);
uint32_t harness_spinner_count(void);
/*
 * 0   = the core was fully occupied by something other than the spinner (busy-wait)
 * 100 = the core was as free as when idle (the waiter yielded properly)
 * -1  = calibration did not separate the two poles; metric unusable
 */
int harness_yield_pct(uint32_t spinner_delta, uint32_t window_ms);

/* ---- cycle occupancy: hardware WFE detection -------------------------------------- */

uint32_t harness_cycles(void);
/* DWT_SLEEPCNT gives a direct "did this core sleep" signal, unlike CYCCNT which counts
 * through WFE. Not implemented on the RP2350 M33; expected on future parts.
 *
 * 8-bit and WRAPPING, so read it asymmetrically: a change proves the core slept, but no
 * change is inconclusive rather than proof it did not (~1/256 of sleeps are an exact
 * multiple of 256 cycles and read back unchanged, whatever the duration).
 * Per core: must be read on the core being judged. */
/*
 * Whether this platform is expected to have the DWT profiling counters (of which SLEEPCNT is
 * one). Positively specified - name the architectures that have them - so an unrecognised
 * future platform defaults to "not expected" rather than raising a false failure.
 *
 * Absent on Armv6-M (Cortex-M0+) and, importantly, on Armv8-M *Baseline* (Cortex-M23) - the
 * profiling counters come with Mainline, which is why the SDK gates its software spin lock on
 * __ARM_ARCH_8M_MAIN__ rather than on Armv8-M generally. Absent on RISC-V, which has no DWT.
 */
#if defined(__ARM_ARCH_8M_MAIN__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
#define HARNESS_EXPECT_SLEEP_COUNTER 1
#else
#define HARNESS_EXPECT_SLEEP_COUNTER 0
#endif

/* 1 only where a DWT sleep counter exists at all; 0 elsewhere, where
 * harness_sleep_counter() is a constant 0. Should agree with the expectation above. */
bool     harness_sleep_counter_present(void);
uint32_t harness_sleep_counter(void);
/* 8-bit wrapping delta between two harness_sleep_counter() reads */
uint32_t harness_sleep_delta(uint32_t before, uint32_t after);
/*
 * The only signal worth acting on: was DWT_SLEEPCNT *observed to move* across a known sleep
 * during calibration? It cannot move unless it genuinely works.
 *
 * Deliberately not derived from DWT_CTRL.NOPRFCNT. That bit covers the profiling counters as
 * a group, and the RP2350 M33 implements EXCCNT/FOLDCNT/LSUCNT but not SLEEPCNT - so it
 * reads clear (truthfully) while the counter we want is absent. m33.h reflects this: it
 * defines M33_DWT_{EXCCNT,FOLDCNT,LSUCNT}_OFFSET but no SLEEPCNT offset at all.
 */
bool     harness_sleep_counter_usable(void);
/* Counters differ in width and direction (SysTick counts down), so deltas go through this. */
uint32_t harness_cycle_delta(uint32_t before, uint32_t after);
/* The counter is per core, and on Arm SysTick belongs to FreeRTOS - so this may only be
 * called on a core that is not running FreeRTOS. */
void     harness_cycles_enable_this_core(void);
/* Measured cycles/us while spinning, from the calibration. Should be ~clk_sys in MHz; a
 * wildly wrong value (notably 0) means the counter is not running. */
uint32_t harness_cal_cycles_per_us(void);
/*
 * A counter verdict belongs to the core that measured it - DWT is per core, and on Arm
 * SysTick belongs to FreeRTOS. Keeping the measurement in a value rather than in harness
 * globals is what stops one core's result being reported as a program-wide fact: that is how
 * a run once printed "DWT_SLEEPCNT ... not usable" in its banner and then "SLEEPCNT agrees"
 * in D1.4, from two different cores' counters.
 */
typedef struct {
    uint32_t busy_per_us;           /* cycles/us while spinning */
    uint32_t sleep_per_us;          /* cycles/us while in WFE   */
    uint32_t dwt_ctrl;              /* 0 where there is no DWT; diagnoses a dead counter */
    uint32_t demcr;
    bool     sleepcnt_moved;        /* moved across a known sleep */
    bool     sleepcnt_moved_awake;  /* moved while busy => not a sleep counter at all */
} harness_cal_t;

/* Measure this core's counters; must run on the core being judged. Writes nothing global. */
void     harness_calibrate_cycles_local(harness_cal_t *out);
/* Install the agent core's measurement. The queries above report the agent's numbers, since
 * the agent core is the only one whose sleeping the cases ever judge. */
void     harness_set_agent_calibration(const harness_cal_t *cal);
uint32_t harness_agent_dwt_ctrl(void);
uint32_t harness_agent_demcr(void);
/* False if the agent core never reported one - its probe arms an alarm and waits on it, so
 * this fails whenever alarms are not reaching that core. Distinguishing that from "counters
 * read zero" is the difference between a broken timer and a disabled counter. */
bool     harness_agent_calibrated(void);

/*
 * 100 = the core executed instructions for the whole window (spinning)
 * 0   = the core executed almost nothing (asleep in WFE)
 * -1  = unavailable on this platform, or calibration failed
 */
int harness_occupancy_pct(uint32_t cycle_delta, uint64_t elapsed_us);

/* ---- setup ------------------------------------------------------------------------ */

/* Calibrates the spinner. Must be called from a task, after the scheduler starts.
 * The cycle calibration is separate and runs on the agent core. */
void harness_calibrate(void);
void harness_print_calibration(void);

#endif
