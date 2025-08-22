/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/low_power.h"
#include "pico/aon_timer.h"
#include "pico/status_led.h"

#define SLEEP_TIME_S 2
#define SLEEP_TIME_MS SLEEP_TIME_S * 1000

bool repeater(repeating_timer_t *timer) {
    if (aon_timer_is_running()) {
        printf("  Repeating timer at %dms (aon: %dms)\n", to_ms_since_boot(get_absolute_time()), to_ms_since_boot(aon_timer_get_absolute_time()));
    } else {
        printf("  Repeating timer at %dms (aon: not running)\n", to_ms_since_boot(get_absolute_time()));
    }
    status_led_set_state(!status_led_get_state());
    return true;
}

#if !PICO_RP2040
static bool came_from_pstate = false;
static char powman_last_pwrup[100];
static char powman_last_pstate[100];

void pstate_resume_func(pstate_bitset_t *pstate) {
    came_from_pstate = true;
    switch (powman_hw->last_swcore_pwrup) {
        //               0 = chip reset, for the source of the last reset see
        case 1 << 0: strcpy(powman_last_pwrup, "Chip reset"); break;
        case 1 << 1: strcpy(powman_last_pwrup, "Pwrup0"); break;
        case 1 << 2: strcpy(powman_last_pwrup, "Pwrup1"); break;
        case 1 << 3: strcpy(powman_last_pwrup, "Pwrup2"); break;
        case 1 << 4: strcpy(powman_last_pwrup, "Pwrup3"); break;
        case 1 << 5: strcpy(powman_last_pwrup, "Coresight_pwrup"); break;
        case 1 << 6: strcpy(powman_last_pwrup, "Alarm_pwrup"); break;
        default: strcpy(powman_last_pwrup, "Unknown pwrup"); break;
    }

    if (pstate_bitset_is_set(pstate, POWMAN_POWER_DOMAIN_XIP_CACHE)) strcat(powman_last_pstate, "XIP_CACHE, ");
    if (pstate_bitset_is_set(pstate, POWMAN_POWER_DOMAIN_SRAM_BANK0)) strcat(powman_last_pstate, "SRAM_BANK0, ");
    if (pstate_bitset_is_set(pstate, POWMAN_POWER_DOMAIN_SRAM_BANK1)) strcat(powman_last_pstate, "SRAM_BANK1, ");
    if (pstate_bitset_none_set(pstate)) strcat(powman_last_pstate, "NONE, ");
}
#endif

int main() {
    stdio_init_all();
    status_led_init();
    printf("Hello Sleep!\n");
    // use a repeating timer; it should be gated
    // during our sleep (todo not sure how it affects power!)
    repeating_timer_t repeat;
    add_repeating_timer_ms(500, repeater, NULL, &repeat);

#if !PICO_RP2040
    if (came_from_pstate) {
        printf("Came from powerup %s with (%s) memory kept on - skipping to end\n", powman_last_pwrup, powman_last_pstate);
        goto post_pstate_gpio;
    }

    pstate_bitset_t pstate;
#endif

    printf("Waiting %d seconds\n", SLEEP_TIME_S); // so we can see some repeat printfs
    busy_wait_ms(SLEEP_TIME_MS);

    absolute_time_t start_time;
    struct timespec ts;
    int ret;

    printf("Going to sleep until GPIO wakeup\n");

    low_power_sleep_until_pin_state(PICO_DEFAULT_UART_RX_PIN, true, false, NULL, true);
    printf("Doing %d second pause to prove timer running\n", SLEEP_TIME_S);
    busy_wait_ms(SLEEP_TIME_MS);

    printf("Going to non-exclusive sleep until GPIO wakeup\n");

    // need to keep the timer running
    clock_dest_set_t keep_enabled = clock_dest_set_none();
#if PICO_RP2040
    clock_dest_set_add(&keep_enabled, CLK_DEST_SYS_TIMER);
#else
    clock_dest_set_add(&keep_enabled, CLK_DEST_SYS_TIMER0);
    clock_dest_set_add(&keep_enabled, CLK_DEST_REF_TICKS);
#endif

    low_power_sleep_until_pin_state(PICO_DEFAULT_UART_RX_PIN, true, false, &keep_enabled, false);
    printf("Doing %d second pause to prove timer running\n", SLEEP_TIME_S);
    busy_wait_ms(SLEEP_TIME_MS);

    // todo, ah; we should start the aon timer; still have to decide what to do about keeping them in sync
    start_time = get_absolute_time();
    us_to_timespec(start_time, &ts);
    aon_timer_start(&ts);

    printf("Going DORMANT until GPIO wakeup\n");

    low_power_dormant_until_pin_state(PICO_DEFAULT_UART_RX_PIN, true, false, DORMANT_CLOCK_SOURCE_ROSC, NULL);
    printf("Doing %d second pause to prove timer running\n", SLEEP_TIME_S);
    busy_wait_ms(SLEEP_TIME_MS);

#if !PICO_RP2040
    printf("Going to PSTATE until GPIO wakeup\n");

    pstate = pstate_bitset_none();
    ret = low_power_pstate_until_pin_state(PICO_DEFAULT_UART_RX_PIN, true, false, &pstate, pstate_resume_func);

    printf("%d low_power_pstate_until_pin_state returned\n", ret);
    while (true) {
        printf("Waiting\n");
        busy_wait_ms(1000);
    }

post_pstate_gpio:

    printf("Doing %d second pause to prove timer running\n", SLEEP_TIME_S);
    busy_wait_ms(SLEEP_TIME_MS);
#endif

    printf("SUCCESS\n");

    return 0;
}