/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/low_power.h"
#include "pico/aon_timer.h"

bool repeater(repeating_timer_t *timer) {
    printf("Repeating timer at %dms\n", time_us_32() / 1000);
    return true;
}

int main() {
    stdio_init_all();
    printf("Hello Sleep!\n");
#if PICO_RP2350
    // use a second repeating timer on the other TIMER instance; it should be gated
    // during our sleep (todo not sure how it affects power!)
    alarm_pool_t *alarm_pool = alarm_pool_create_on_timer_with_unused_hardware_alarm(timer1_hw, 4);
    repeating_timer_t repeat;
    alarm_pool_add_repeating_timer_ms(alarm_pool, 500, repeater, NULL, &repeat);
    printf("Waiting 1 sec\n"); // so we can see some repeat printfs
    busy_wait_ms(1100);
#endif
    printf("Going to sleep for 5 seconds via TIMER\n");

    absolute_time_t start_time = get_absolute_time();
    absolute_time_t wakeup_time = delayed_by_ms(start_time, 5000);
    low_power_sleep_until_timer(timer_hw, wakeup_time, NULL);
    int64_t diff = absolute_time_diff_us(wakeup_time, get_absolute_time());
    printf("Woken up now @%dus since target\n", (int)diff);
    if (diff < 0) {
        printf("ERROR: Woke up too soon\n");
        return -1;
    }
    busy_wait_ms(3000);

    printf("Going to sleep for 5 seconds via AON TIMER\n");

    // todo, ah; we should start the aon timer; still have to decide what to do about keeping them in sync
    start_time = get_absolute_time();
    struct timespec ts;
    us_to_timespec(start_time, &ts);
    aon_timer_start(&ts);

    wakeup_time = delayed_by_ms(start_time, 5000);
    low_power_sleep_until_aon_timer(wakeup_time, NULL);
    diff = absolute_time_diff_us(get_absolute_time(), wakeup_time);
    // need to use the AON timer for checking time, since the other timer is unclocked
    diff = absolute_time_diff_us(wakeup_time, get_absolute_time());
    if (diff > -4000000) {
        printf("ERROR: doesn't seem like timer was stopped\n");
        return - 1;
    }
    aon_timer_get_time(&ts);
    uint64_t current_aon = timespec_to_us(&ts);
    diff = absolute_time_diff_us(wakeup_time, from_us_since_boot(current_aon));
    printf("Woken up now @%dus since target\n", (int)diff);
    if (diff < 0) {
        printf("WARNING: Woke up too soon - is this within the resolution of the aon timer?\n");
    }
    printf("5 second pause to prove timer still running\n");
    busy_wait_ms(5000);

    printf("Going DORMANT for 5 seconds via AON TIMER\n");

    // todo, ah; we should start the aon timer; still have to decide what to do about keeping them in sync
    start_time = get_absolute_time();
    us_to_timespec(start_time, &ts);
    aon_timer_start(&ts);

    wakeup_time = delayed_by_ms(start_time, 5000);
    low_power_dormant_until_aon_timer(wakeup_time, DORMANT_CLOCK_SOURCE_LPOSC, XOSC_KHZ * 1000,
                                      0, // gpio pin (unused with powman)
                                      NULL);
    low_power_sleep_until_aon_timer(wakeup_time, NULL);
    diff = absolute_time_diff_us(get_absolute_time(), wakeup_time);
    // need to use the AON timer for checking time, since the other timer is unclocked
    diff = absolute_time_diff_us(wakeup_time, get_absolute_time());
    if (diff > -4000000) {
        printf("ERROR: doesn't seem like timer was stopped\n");
        return - 1;
    }
    aon_timer_get_time(&ts);
    current_aon = timespec_to_us(&ts);
    diff = absolute_time_diff_us(wakeup_time, from_us_since_boot(current_aon));
    printf("Woken up now @%dus since target\n", (int)diff);
    if (diff < 0) {
        printf("WARNING: Woke up too soon - is this within the resolution of the aon timer?\n");
    }
    printf("Final 5 second pause to prove timer still running\n");
    busy_wait_ms(5000);

    printf("SUCCESS\n");

    return 0;
}