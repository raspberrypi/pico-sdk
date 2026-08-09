/*
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "pico/stdlib.h"
#ifdef __riscv
#include "hardware/regs/rvcsr.h"
#endif
#include "hardware/structs/systick.h"
#include "pico/platform/cpu_regs.h"

#include "interop_platform.h"
#include "interop_harness.h"

/* ---- result recording ------------------------------------------------------------- */

typedef struct {
    const char *id;
    result_kind_t kind;
    char detail[200];
} result_t;

static result_t results[MAX_RESULTS];
static uint result_count;

static const char *kind_name(result_kind_t k) {
    switch (k) {
        case RESULT_PASS:          return "PASS";
        case RESULT_FAIL:          return "FAIL";
        case RESULT_SKIP:          return "SKIP";
        case RESULT_EXPECTED_FAIL: return "XFAIL";
        default:                   return "INFO";
    }
}

void harness_record(const char *id, result_kind_t kind, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char detail[200];
    vsnprintf(detail, sizeof(detail), fmt, args);
    va_end(args);

    printf("  [%-5s] %-6s %s\n", kind_name(kind), id, detail);

    if (result_count < MAX_RESULTS) {
        results[result_count].id = id;
        results[result_count].kind = kind;
        strncpy(results[result_count].detail, detail, sizeof(results[0].detail) - 1);
        results[result_count].detail[sizeof(results[0].detail) - 1] = 0;
        result_count++;
    }
}

int harness_summary(void) {
    uint counts[5] = {0};
    for (uint i = 0; i < result_count; i++) {
        counts[results[i].kind]++;
    }
    printf("\n=== summary ===\n");
    printf("  %u pass, %u fail, %u skip, %u expected-fail, %u info\n",
           counts[RESULT_PASS], counts[RESULT_FAIL], counts[RESULT_SKIP],
           counts[RESULT_EXPECTED_FAIL], counts[RESULT_INFO]);

    if (counts[RESULT_FAIL]) {
        printf("\nfailures:\n");
        for (uint i = 0; i < result_count; i++) {
            if (results[i].kind == RESULT_FAIL) {
                printf("  %-6s %s\n", results[i].id, results[i].detail);
            }
        }
    }
    if (counts[RESULT_EXPECTED_FAIL]) {
        printf("\nexpected failures (known-broken, not counted against the run):\n");
        for (uint i = 0; i < result_count; i++) {
            if (results[i].kind == RESULT_EXPECTED_FAIL) {
                printf("  %-6s %s\n", results[i].id, results[i].detail);
            }
        }
    }
    return counts[RESULT_FAIL] != 0;
}

/* ---- latency ---------------------------------------------------------------------- */

void latency_reset(latency_t *l) {
    memset(l, 0, sizeof(*l));
    l->min_us = INT64_MAX;
    l->max_us = INT64_MIN;
}

void latency_add(latency_t *l, int64_t lateness_us) {
    l->count++;
    l->sum_us += lateness_us;
    if (lateness_us < l->min_us) l->min_us = lateness_us;
    if (lateness_us > l->max_us) l->max_us = lateness_us;
    if (lateness_us < 0) {
        l->early++;
        return;
    }
    uint b = 0;
    int64_t v = lateness_us;
    while (v >= 1 && b < LATENCY_BUCKETS - 1) {
        v >>= 1;
        b++;
    }
    l->bucket[b]++;
}

int64_t latency_mean_us(const latency_t *l) {
    return l->count ? l->sum_us / (int64_t)l->count : 0;
}

void latency_print(const latency_t *l, const char *name) {
    if (!l->count) {
        printf("    %s: no samples\n", name);
        return;
    }
    printf("    %s: n=%u min=%lldus mean=%lldus max=%lldus early=%u\n",
           name, l->count, (long long)l->min_us, (long long)latency_mean_us(l),
           (long long)l->max_us, l->early);
    /* only print the histogram when there is a tail worth looking at */
    if (l->max_us > 4 * (latency_mean_us(l) + 1)) {
        printf("      tail:");
        for (uint b = 0; b < LATENCY_BUCKETS; b++) {
            if (l->bucket[b]) printf(" <%lluus:%u", (unsigned long long)(1ull << b), l->bucket[b]);
        }
        printf("\n");
    }
}

/* ---- spinner ---------------------------------------------------------------------- */

static uint32_t cal_spin_idle_per_ms;   /* spinner rate with the core otherwise free */
static uint32_t cal_spin_busy_per_ms;   /* spinner rate with the core hogged */
static bool     spinner_calibrated;

uint32_t harness_spinner_count(void) {
    return plat_spinner_count();
}

void harness_spinner_start(void) {
    plat_spinner_start();
}

int harness_yield_pct(uint32_t spinner_delta, uint32_t window_ms) {
    if (!spinner_calibrated || !window_ms) return -1;
    uint32_t idle = cal_spin_idle_per_ms * window_ms;
    uint32_t busy = cal_spin_busy_per_ms * window_ms;
    if (idle <= busy) return -1;
    if (spinner_delta <= busy) return 0;
    if (spinner_delta >= idle) return 100;
    return (int)(((uint64_t)(spinner_delta - busy) * 100u) / (idle - busy));
}

/* ---- cycle counter -----------------------------------------------------------------
 *
 * Only ever used to judge the *bare-SDK* core's occupancy (D1.4, D2.7), which matters,
 * because on Arm the FreeRTOS ports use SysTick for the tick - so SysTick may only be
 * commandeered on a core that is not running FreeRTOS. SysTick is a per-core Cortex-M
 * peripheral, so the bare-SDK core's is free even while FreeRTOS runs on the other one.
 * Hence the calibration runs on the agent core too, not here.
 */

#ifdef __riscv
#define HARNESS_HAS_CYCLE_COUNTER 1
#define HARNESS_CYCLE_BITS 32
uint32_t harness_cycles(void) {
    uint32_t c;
    __asm volatile ("csrr %0, mcycle" : "=r" (c));
    return c;
}
static void cycles_enable(void) {
    /* mcycle does NOT run by default on Hazard3: RVCSR_MCOUNTINHIBIT resets to 0x5, i.e. the
     * CY (cycle) and IR (instret) inhibit bits are both set. Clear CY so mcycle advances,
     * otherwise every reading is 0 and the calibration correctly declares it unusable. */
    __asm volatile ("csrci %0, %1" :: "i" (RVCSR_MCOUNTINHIBIT_OFFSET),
                                      "i" (RVCSR_MCOUNTINHIBIT_CY_BITS));
}
static uint32_t cycle_delta(uint32_t before, uint32_t after) { return after - before; }
uint32_t harness_sleep_counter(void) { return 0; }   /* no DWT on RISC-V */
bool harness_sleep_counter_present(void) { return false; }

#elif !defined(__ARM_ARCH_6M__)
#define HARNESS_HAS_CYCLE_COUNTER 1
#define HARNESS_CYCLE_BITS 32
/* Armv7-M/Armv8-M DWT, which FreeRTOS does not use, so this is safe on any core.
 * Note CYCCNT is present across Armv7-M/Armv8-M, but the *profiling* counters (SLEEPCNT et
 * al) are Mainline-only - hence harness_sleep_counter_present() uses the narrower predicate. */
#define HARNESS_HAS_DWT 1
#define DWT_CTRL     (*(volatile uint32_t *)0xE0001000u)
#define DWT_CYCCNT   (*(volatile uint32_t *)0xE0001004u)
#define DWT_SLEEPCNT (*(volatile uint32_t *)0xE0001010u)
#define SCB_DEMCR    (*(volatile uint32_t *)0xE000EDFCu)
uint32_t harness_cycles(void) { return DWT_CYCCNT; }

/*
 * DWT_CYCCNT counts straight through WFE, so it cannot tell sleeping from spinning; that is
 * what DWT_SLEEPCNT is for, and the RP2350 M33 does not implement it (DWT_CTRL.NOPRFCNT
 * reads 1). Future parts are expected to have it, so detect and report support here - the
 * calibration line then says plainly whether a direct sleep signal is available.
 *
 * To use it when it arrives: DWT_SLEEPCNT is only *8 bits* and it WRAPS (it does not
 * saturate), so it cannot be polled for a total - it is designed to be read via ITM overflow
 * packets. A before/after comparison must therefore be read ASYMMETRICALLY:
 *
 *     changed   => the core definitely slept. The counter cannot move otherwise, so there
 *                  are no false positives.
 *     unchanged => INCONCLUSIVE, not "did not sleep". The final value is
 *                  (initial + slept) % 256, so any sleep of an exact multiple of 256 cycles
 *                  reads back unchanged. That is ~1/256 per measurement for an arbitrary
 *                  duration, and - importantly - a longer wait does NOT reduce it.
 *
 * So it can confirm sleeping but never refute it; the wait-loop iteration count remains the
 * criterion. If a stronger signal is ever wanted, repeat the measurement: N independent waits
 * all reading unchanged is (1/256)^N.
 *
 * Note also that the PPB is per core, so this must be read on the core being judged.
 */
uint32_t harness_sleep_counter(void) { return DWT_SLEEPCNT & 0xffu; }
bool harness_sleep_counter_present(void) { return HARNESS_EXPECT_SLEEP_COUNTER; }
static void cycles_enable(void) {
    SCB_DEMCR |= (1u << 24);   /* TRCENA */
    DWT_CYCCNT = 0;
    DWT_CTRL |= 1u;            /* CYCCNTENA */
    /* SLEEPEVTENA *enables the DWT_SLEEPCNT counter* (not merely packet emission), so it
     * must be set before SLEEPCNT means anything - otherwise the runtime check below would
     * false-negative on a part that does implement it. Harmless where it does not. */
    DWT_CTRL |= ARM_CPU_PREFIXED(DWT_CTRL_SLEEPEVTENA_BITS);
}
static uint32_t cycle_delta(uint32_t before, uint32_t after) { return after - before; }

#else
#define HARNESS_HAS_CYCLE_COUNTER 1
#define HARNESS_CYCLE_BITS (ARM_CPU_PREFIXED(SYST_CVR_CURRENT_MSB) + 1)
/* RP2040's Cortex-M0+ has no DWT, so use SysTick clocked from the processor clock. It is a
 * 24-bit DOWN counter, so deltas need sign-extending from 24 bits (cf. double_benchmark.c,
 * which shifts by 32-MSB; 32-(MSB+1) is the correct width for a 24-bit field, and matters
 * here because our windows are far longer than that benchmark's). */
uint32_t harness_cycles(void) { return systick_hw->cvr; }
static void cycles_enable(void) {
    systick_hw->csr = 0;
    systick_hw->rvr = ARM_CPU_PREFIXED(SYST_RVR_RELOAD_BITS);
    systick_hw->csr = ARM_CPU_PREFIXED(SYST_CSR_CLKSOURCE_BITS) | ARM_CPU_PREFIXED(SYST_CSR_ENABLE_BITS);
}
uint32_t harness_sleep_counter(void) { return 0; }   /* no DWT on Cortex-M0+ */
bool harness_sleep_counter_present(void) { return false; }
static uint32_t cycle_delta(uint32_t before, uint32_t after) {
    static_assert(ARM_CPU_PREFIXED(SYST_CVR_CURRENT_LSB) == 0, "");
    const uint32_t shift = 32 - HARNESS_CYCLE_BITS;
    /* counts down, so `before` is the larger value; -1 for the cost of the second read */
    int32_t d = (((int32_t)((before << shift) - (after << shift))) >> shift) - 1;
    return d > 0 ? (uint32_t)d : 0;
}
#endif

/* The agent core's measurement, and nothing else: see harness_cal_t. */
static harness_cal_t agent_cal;
static bool          cycles_ok;
/* False until the agent core actually reports. Without this, an agent that never answered is
 * indistinguishable from one whose counters read zero - and the two have completely different
 * causes (a dead probe on the agent core vs a disabled counter). */
static bool          agent_cal_valid;

void harness_cycles_enable_this_core(void) {
    cycles_enable();
}

uint32_t harness_cycle_delta(uint32_t before, uint32_t after) {
    return cycle_delta(before, after);
}

void harness_set_agent_calibration(const harness_cal_t *cal) {
    agent_cal = *cal;
    agent_cal_valid = true;
    cycles_ok = cal->busy_per_us > cal->sleep_per_us * 2 + 1;
}

bool harness_agent_calibrated(void) { return agent_cal_valid; }

uint32_t harness_agent_dwt_ctrl(void) { return agent_cal.dwt_ctrl; }
uint32_t harness_agent_demcr(void)    { return agent_cal.demcr; }

uint32_t harness_sleep_delta(uint32_t before, uint32_t after) {
    return (after - before) & 0xffu;
}

uint32_t harness_cal_cycles_per_us(void) {
    return agent_cal.busy_per_us;
}

bool harness_sleep_counter_usable(void) {
    /* Observed movement across a known sleep is the only signal worth acting on. DWT_CTRL's
     * NOPRFCNT bit is not used: it governs the profiling counters as a group and reads clear
     * on the RP2350 M33 even though SLEEPCNT is not there, so it is a false positive here. */
    return agent_cal.sleepcnt_moved;
}

int harness_occupancy_pct(uint32_t cycle_delta_cycles, uint64_t elapsed_us) {
    if (!cycles_ok || !elapsed_us) return -1;
    /* a narrow counter wraps: refuse to guess rather than report a bogus figure */
    if (agent_cal.busy_per_us &&
        elapsed_us > ((1ull << HARNESS_CYCLE_BITS) * 4 / 5) / agent_cal.busy_per_us) {
        return -1;
    }
    uint32_t busy = (uint32_t)(agent_cal.busy_per_us * elapsed_us);
    uint32_t sleep = (uint32_t)(agent_cal.sleep_per_us * elapsed_us);
    if (busy <= sleep) return -1;
    if (cycle_delta_cycles <= sleep) return 0;
    if (cycle_delta_cycles >= busy) return 100;
    return (int)(((uint64_t)(cycle_delta_cycles - sleep) * 100u) / (busy - sleep));
}

/* ---- calibration ------------------------------------------------------------------ */

#define CAL_WINDOW_MS 100

static int64_t cal_wfe_alarm(__unused alarm_id_t id, __unused void *ud) {
    __sev();
    return 0;
}

void harness_calibrate_cycles_local(harness_cal_t *out) {
    cycles_enable();

    /* Read back what cycles_enable() actually achieved on THIS core, so that a counter which
     * reads zero can say whether it was never enabled or is enabled and simply not counting.
     * Guessing between those two cost a debugging session. */
#if HARNESS_HAS_DWT
    out->dwt_ctrl = DWT_CTRL;
    out->demcr    = SCB_DEMCR;
#else
    out->dwt_ctrl = 0;
    out->demcr    = 0;
#endif

    absolute_time_t t0 = get_absolute_time();
    uint32_t c0 = harness_cycles();
    uint32_t sb0 = harness_sleep_counter();
    busy_wait_ms(20);
    uint32_t sb1 = harness_sleep_counter();
    uint32_t c1 = harness_cycles();
    /* negative control: SLEEPCNT must NOT move while we are demonstrably awake. Without
     * this, a register that is unimplemented (0xE0001010 is omitted from m33.h, which
     * defines EXCCNT at 0x100c and LSUCNT at 0x1014) could read as changing noise and be
     * mistaken for a working sleep counter. */
    bool sleepcnt_moved_while_busy = (sb0 != sb1);
    int64_t us = absolute_time_diff_us(t0, get_absolute_time());
    out->busy_per_us = us > 0 ? (uint32_t)(cycle_delta(c0, c1) / (uint64_t)us) : 0;

    /* Asleep pole: WFE against an alarm 20ms out. add_alarm_in_us() forces the timer IRQ and
     * takes spin locks, all of which latch events, so drain once before measuring. No need
     * to disable interrupts around the arming - the alarm is 20ms away, so there is nothing
     * to race with, and deferring the forced IRQ would only tangle it with the drain. */
    add_alarm_in_us(20000, cal_wfe_alarm, NULL, true);
    __sev();
    __wfe();
    t0 = get_absolute_time();
    c0 = harness_cycles();
    uint32_t s0 = harness_sleep_counter();
    __wfe();
    uint32_t s1 = harness_sleep_counter();
    c1 = harness_cycles();
    /* Usable only if it moved across a known sleep AND stayed put while demonstrably busy. */
    out->sleepcnt_moved = (s0 != s1) && !sleepcnt_moved_while_busy;
    out->sleepcnt_moved_awake = sleepcnt_moved_while_busy;
    us = absolute_time_diff_us(t0, get_absolute_time());
    out->sleep_per_us = us > 0 ? (uint32_t)(cycle_delta(c0, c1) / (uint64_t)us) : UINT32_MAX;
}

void harness_calibrate(void) {
    /* --- spinner: idle pole (we yield, so the spinner gets the core) --- */
    uint32_t before = plat_spinner_count();
    plat_delay_ms(CAL_WINDOW_MS);
    cal_spin_idle_per_ms = (plat_spinner_count() - before) / CAL_WINDOW_MS;

    /* --- spinner: busy pole (we hog the core at test priority) --- */
    before = plat_spinner_count();
    busy_wait_ms(CAL_WINDOW_MS);
    cal_spin_busy_per_ms = (plat_spinner_count() - before) / CAL_WINDOW_MS;

    spinner_calibrated = cal_spin_idle_per_ms > cal_spin_busy_per_ms * 2 + 1;
    /* cycle calibration happens on the agent core - see harness_calibrate_cycles_local() */
}

void harness_print_calibration(void) {
    printf("calibration:\n");
    if (INTEROP_HAS_SCHEDULER) {
        printf("  spinner: idle %u/ms, busy %u/ms -> %s\n",
               cal_spin_idle_per_ms, cal_spin_busy_per_ms,
               spinner_calibrated ? "usable" : "UNUSABLE (yield checks skipped)");
    } else {
        printf("  spinner: no scheduler, so yielding does not apply (yield checks skipped)\n");
    }
    /* Note: sleeping vs spinning is decided by the wait-loop iteration count, always. The
     * cycle counter is only ever a secondary note, so its absence skips nothing important. */
    if (!INTEROP_HAS_SDK_CORE) {
        printf("  cycles:  no bare-SDK core in this configuration, so not measured\n");
    } else if (!agent_cal_valid) {
        printf("  cycles:  the agent core never returned a calibration - its probe arms an"
               " alarm and waits on it, so alarms are not reaching that core\n");
    } else if (cycles_ok) {
        printf("  cycles (agent core, %d-bit): busy %u/us, asleep %u/us"
               " -> separates, will be reported as a secondary note\n",
               (int)HARNESS_CYCLE_BITS, agent_cal.busy_per_us, agent_cal.sleep_per_us);
    } else {
        printf("  cycles (agent core, %d-bit): busy %u/us, asleep %u/us"
               " -> does not separate, so the counter cannot see sleep; note omitted\n",
               (int)HARNESS_CYCLE_BITS, agent_cal.busy_per_us, agent_cal.sleep_per_us);
        if (agent_cal.dwt_ctrl || agent_cal.demcr) {
            printf("           agent core DWT_CTRL=0x%08x DEMCR=0x%08x (CYCCNTENA=%u"
                   " TRCENA=%u)\n", agent_cal.dwt_ctrl, agent_cal.demcr,
                   agent_cal.dwt_ctrl & 1u, (agent_cal.demcr >> 24) & 1u);
        }
    }
    printf("  sleep detection: wait-loop iteration count (<=%d means it blocked)\n",
           MAX_BLOCKING_WAITS);
    if (!harness_sleep_counter_present()) {
        printf("    DWT_SLEEPCNT: not present on this platform (no DWT)\n");
    } else if (harness_sleep_counter_usable()) {
        printf("    DWT_SLEEPCNT (agent core): moved across a known sleep and held still while"
               " busy - a direct sleep signal is available\n");
    } else if (agent_cal.sleepcnt_moved_awake) {
        printf("    DWT_SLEEPCNT (agent core): moved while demonstrably BUSY - not a sleep"
               " counter (0x%08x is not defined in m33.h); not usable\n", 0xE0001010u);
    } else {
        printf("    DWT_SLEEPCNT (agent core): did not move across a known sleep - not"
               " usable\n");
    }
}
