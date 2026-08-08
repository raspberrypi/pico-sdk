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
/* 1 only where a DWT sleep counter exists at all (Armv7-M/Armv8-M); 0 on RISC-V and
 * Cortex-M0+, where harness_sleep_counter() is a constant 0. */
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
/* Measure this core's busy and asleep cycle rates; run on the core to be judged. */
void     harness_calibrate_cycles_local(uint32_t *busy_per_us, uint32_t *sleep_per_us);
/* Install a calibration measured elsewhere (i.e. on the agent core). */
void     harness_set_cycle_calibration(uint32_t busy_per_us, uint32_t sleep_per_us);

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
