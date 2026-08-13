/**
* Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>

#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "pico/test.h"

// The overhead adjust busy-waits the last few us of every sleep, landing each one exactly on its
// deadline and hiding everything this test measures - per-sleep cost, the difference between the
// pool's core and the other, and the cost of contention all read as zero with it enabled.
static_assert(PICO_TIME_SLEEP_OVERHEAD_ADJUST_US == 0, "the busy-wait tail would mask what this measures");


#define DELAY_US_MIN 0
#define DELAY_US_MAX 50
#define SLEEPS_PER_DELAY 20
/*
 * How far a sleep may overshoot, per sleep. Uncontended the cost is flat - a steady 1-3us, worst
 * at the shortest delays where the surrounding work exceeds the sleep being asked for, and the
 * same within a few tenths across Arm, RISC-V and both spin lock types. The allowances come from
 * the worst measured: 6.4us on RP2040, 3.9us on RP2350 over five configurations.
 *
 * Contended, collisions between the two sweeps reach 10-18us and move between runs of the same
 * build, so that section reports rather than asserts.
 */
#if PICO_RP2040
#define PER_SLEEP_ALLOWANCE_US 8
#else
#define PER_SLEEP_ALLOWANCE_US 6
#endif
#define CONTENDED_FLAG_US     15    /* reported only */

bool allow_slower_core0;

/*
 * The concurrent section covers one interaction nothing else does: core 1's sleeps take the
 * cross-core add path, so the pool's core re-enters its handler to serve them while itself asleep
 * in WFE. What is asserted there is that no sleep returns early; the overshoot is reported only,
 * concurrency cost being scheduling jitter rather than a contract.
 */
static bool enforce_cumulative = true;
static volatile bool core0_sweeping;    /* core 1 keeps sweeping while this is set */
static bool sweep_reverse;              /* core 1 sweeps the other way when concurrent, so a
                                         * short sleep meets a long one rather than its twin */
static bool results_held[NUM_CORES];    /* record the first sweep of a section, not the last */

// Recorded during the sweep and printed afterwards: the cores share stdio behind a mutex, so
// printing inline would land in the other core's sleep timing.
static int r_tdelta[NUM_CORES][DELAY_US_MAX];
static int r_allowed[NUM_CORES][DELAY_US_MAX];   /* overshoot allowed per sleep, us; 0 = not run */

static void dump_sleeps(uint core_num) {
    int worst_ovh_tenths = -100000, worst_ovh_delay = -1;
    int tightest_margin = 100000, tightest_delay = -1;
    for (int delay = DELAY_US_MIN; delay < DELAY_US_MAX; delay++) {
        int tdelta = r_tdelta[core_num][delay], allowed = r_allowed[core_num][delay];
        if (!allowed) continue;   /* never reached this delay: the sweep stopped early */
        int ideal = delay * SLEEPS_PER_DELAY;
        int ovh_tenths = (tdelta - ideal) * 10 / SLEEPS_PER_DELAY;
        if (ovh_tenths > worst_ovh_tenths) {
            worst_ovh_tenths = ovh_tenths;
            worst_ovh_delay = delay;
        }
        if (allowed * 10 - ovh_tenths < tightest_margin) {
            tightest_margin = allowed * 10 - ovh_tenths;
            tightest_delay = delay;
        }
        printf("  core %u delay %2d: %5dus for %d sleeps (ideal %4d) -> %d.%dus/sleep, "
               "allowed %dus%s\n",
               core_num, delay, tdelta, SLEEPS_PER_DELAY, ideal,
               ovh_tenths / 10, ovh_tenths % 10, allowed,
               ovh_tenths > allowed * 10 ? "  OVER" : "");
    }
    printf("core %u: worst overhead %d.%dus/sleep at delay %d, tightest margin %d.%dus/sleep "
           "at delay %d\n",
           core_num, worst_ovh_tenths / 10, worst_ovh_tenths % 10, worst_ovh_delay,
           tightest_margin / 10, tightest_margin < 0 ? -tightest_margin % 10 : tightest_margin % 10,
           tightest_delay);
}

int do_sleeps() {
    uint core_num = get_core_num();
    uint32_t tstart = time_us_32();
    bool reverse = sweep_reverse && core_num;
    for (int step = 0; step < DELAY_US_MAX - DELAY_US_MIN; step++) {
        int delay = reverse ? (DELAY_US_MAX - 1 - step) : (DELAY_US_MIN + step);
        uint32_t t0 = time_us_32();
        uint32_t t2;
        for (int count = 0; count < SLEEPS_PER_DELAY; count++) {
            uint32_t t1 = time_us_32();
            sleep_us(delay);
            t2 = time_us_32();
            // do minimum time check per sleep as we should never wake up too soon
            int tdelta = (int)(t2 - t1);
            if (tdelta < delay) {
                printf("Core %u: tdelta for sleep %d x %d: %d < expected min %d\n", core_num, delay, SLEEPS_PER_DELAY, tdelta, delay);
                return -1;
            }
        }
        // Bound the overshoot per sleep rather than the elapsed total, so the check is in the
        // units the cost has and reads the same at a 1us delay as at a 49us one.
        int tdelta = (int)(t2 - t0);
        int ovh_tenths = (tdelta - delay * SLEEPS_PER_DELAY) * 10 / SLEEPS_PER_DELAY;
        int allowed = (allow_slower_core0 && !core_num) ? CONTENDED_FLAG_US
                                                        : PER_SLEEP_ALLOWANCE_US;
        if (!results_held[core_num]) {
            r_tdelta[core_num][delay] = tdelta;
            r_allowed[core_num][delay] = allowed;
        }
        if (ovh_tenths > allowed * 10 && enforce_cumulative) {
            printf("Core %u: sleep %d x %d overshot by %d.%dus/sleep, allowed %dus\n",
                   core_num, delay, SLEEPS_PER_DELAY, ovh_tenths / 10, ovh_tenths % 10, allowed);
            return -1;
        }
    }
    uint32_t tend = time_us_32();
    printf("Core %u: total time %dus\n", core_num, (int)(tend - tstart));
    return 0;
}

semaphore_t core1_sem;
int core1_picotest_error_code;

void core1_do_sleeps() {
    do {
        core1_picotest_error_code = do_sleeps();
        // keep the window contended for as long as the other core is in it; without this the
        // tail of core 0's sweep runs alone and reports numbers for a condition it is no
        // longer under
        results_held[1] = true;
    } while (!core1_picotest_error_code && core0_sweeping);
    sem_release(&core1_sem);
}

int main() {
    stdio_init_all();
    sem_init(&core1_sem, 0, 1);

    PICOTEST_MODULE_NAME("short_sleep_test", "short_sleep test harness");
    PICOTEST_START();
    PICOTEST_START_SECTION("sleeps core 0");
    picotest_error_code = do_sleeps();
    dump_sleeps(0);
    PICOTEST_END_SECTION();
    PICOTEST_START_SECTION("sleeps core 1");
    multicore_launch_core1(core1_do_sleeps);
    sem_acquire_blocking(&core1_sem);
    picotest_error_code = core1_picotest_error_code;
    dump_sleeps(1);
    PICOTEST_END_SECTION();
    PICOTEST_START_SECTION("sleeps core 0 & 1");
    // overshoot reported, not enforced - see enforce_cumulative above; rows that would have
    // failed are still marked OVER
    allow_slower_core0 = true;
    enforce_cumulative = false;
    sweep_reverse = true;
    results_held[0] = results_held[1] = false;
    memset(r_tdelta, 0, sizeof(r_tdelta));
    memset(r_allowed, 0, sizeof(r_allowed));
    core0_sweeping = true;
    multicore_reset_core1();
    multicore_launch_core1(core1_do_sleeps);
    picotest_error_code = do_sleeps();
    core0_sweeping = false;
    sem_acquire_blocking(&core1_sem);
    if (!picotest_error_code) picotest_error_code = core1_picotest_error_code;
    dump_sleeps(0);
    dump_sleeps(1);
    PICOTEST_END_SECTION();
    PICOTEST_END_TEST();
}