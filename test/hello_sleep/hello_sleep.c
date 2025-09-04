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
#include "hardware/structs/xip_ctrl.h"

#define SLEEP_TIME_S 2
#define SLEEP_TIME_MS SLEEP_TIME_S * 1000

#define RTC_GPIO 22 // must support clock input, see the GPIO function table in the datasheet.

bool repeater(repeating_timer_t *timer) {
    if (aon_timer_is_running()) {
        printf("  Repeating timer %d at %dms (aon: %dms)\n", *(uint32_t*)timer->user_data, to_ms_since_boot(get_absolute_time()), to_ms_since_boot(aon_timer_get_absolute_time()));
    } else {
        printf("  Repeating timer %d at %dms (aon: not running)\n", *(uint32_t*)timer->user_data, to_ms_since_boot(get_absolute_time()));
    }

    printf("Cache hits: %f\n", (float)xip_ctrl_hw->ctr_hit / (float)xip_ctrl_hw->ctr_acc);

    status_led_set_state(!status_led_get_state());
    return true;
}

#if HAS_POWMAN_TIMER
static bool came_from_pstate = false;
static char powman_last_pwrup[100];
static char powman_last_pstate[100];

int __persistent_data(my_number);
int my_other_numer = 12345;

void pstate_resume_func(pstate_bitset_t *pstate) {
    came_from_pstate = true;
    memset(powman_last_pwrup, 0, sizeof(powman_last_pwrup));
    memset(powman_last_pstate, 0, sizeof(powman_last_pstate));
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

    pstate_bitset_t default_pstate = pstate_bitset_none();
    low_power_persistent_pstate_get(&default_pstate);
    for (int i = 0; i < POWMAN_POWER_DOMAIN_COUNT; i++) {
        if (pstate_bitset_is_set(&default_pstate, i) && !pstate_bitset_is_set(pstate, i)) {
            strcat(powman_last_pstate, "PERSISTENT_DATA_OFF, ");
            my_number = 34567;
            break;
        }
    }
}
#endif

int main() {
    stdio_init_all();
    status_led_init();
    printf("Hello Sleep!\n");

    // use a repeating timer on the same TIMER instance; it should be disabled
    // during exclusive sleep (todo not sure how it affects power!)
    repeating_timer_t repeat;
    uint32_t repeater_id = 0;
    add_repeating_timer_ms(500, repeater, &repeater_id, &repeat);

#if HAS_POWMAN_TIMER
    // use a second repeating timer on the other TIMER instance; it should be gated
    // during our sleep (todo not sure how it affects power!)
    alarm_pool_t *alarm_pool = alarm_pool_create_on_timer_with_unused_hardware_alarm(timer1_hw, 4);
    repeating_timer_t repeat2;
    uint32_t repeater2_id = 1;
    alarm_pool_add_repeating_timer_ms(alarm_pool, 500, repeater, &repeater2_id, &repeat2);

    if (came_from_pstate) {
        printf("Came from powerup %s with (%s) memory kept on - skipping to end\n", powman_last_pwrup, powman_last_pstate);
        if (strstr(powman_last_pstate, "NONE") != NULL) {
            goto post_pstate_sram_off;
        } else {
            goto post_pstate_sram_on;
        }
    } else {
        // initialise my_number on first boot
        my_number = 12345;
    }

    pstate_bitset_t pstate;
#endif

    printf("Waiting %d seconds\n", SLEEP_TIME_S); // so we can see some repeat printfs
    busy_wait_ms(SLEEP_TIME_MS);

    absolute_time_t start_time;
    absolute_time_t wakeup_time;
    int64_t diff;
    struct timespec ts;
    int ret;



    // exclusive sleep
    printf("Going to sleep for %d seconds via TIMER\n", SLEEP_TIME_S);

    start_time = get_absolute_time();
    wakeup_time = delayed_by_ms(start_time, SLEEP_TIME_MS);
    low_power_sleep_until_timer(timer_hw, wakeup_time, NULL, true);
    diff = absolute_time_diff_us(wakeup_time, get_absolute_time());
    printf("Woken up now @%dus since target\n", (int)diff);
    if (diff < 0) {
        printf("ERROR: Woke up too soon\n");
        return -1;
    }
    printf("Doing %d second pause to prove timer running\n", SLEEP_TIME_S);
    busy_wait_ms(SLEEP_TIME_MS);



    // non-exclusive sleep
    printf("Going to non-exclusive sleep for %d seconds via TIMER\n", SLEEP_TIME_S);

    start_time = get_absolute_time();
    wakeup_time = delayed_by_ms(start_time, SLEEP_TIME_MS);
    low_power_sleep_until_timer(timer_hw, wakeup_time, NULL, false);
    diff = absolute_time_diff_us(wakeup_time, get_absolute_time());
    printf("Woken up now @%dus since target\n", (int)diff);
    if (diff < 0) {
        printf("ERROR: Woke up too soon\n");
        return -1;
    }
    printf("Doing %d second pause to prove timer running\n", SLEEP_TIME_S);
    busy_wait_ms(SLEEP_TIME_MS);



    // dormant
    printf("Going DORMANT for %d seconds via AON TIMER\n", SLEEP_TIME_S);

    // todo, ah; we should start the aon timer; still have to decide what to do about keeping them in sync
    start_time = get_absolute_time();
    us_to_timespec(start_time, &ts);
    aon_timer_start(&ts);

    wakeup_time = delayed_by_ms(start_time, SLEEP_TIME_MS);
    low_power_dormant_until_aon_timer(wakeup_time,
                                #if PICO_RP2040
                                      DORMANT_CLOCK_SOURCE_XOSC, 46875,
                                #else
                                      DORMANT_CLOCK_SOURCE_LPOSC, XOSC_HZ,
                                #endif
                                      RTC_GPIO, NULL);
    diff = absolute_time_diff_us(get_absolute_time(), wakeup_time);
    // need to use the AON timer for checking time, since the other timer is unclocked
    diff = absolute_time_diff_us(wakeup_time, get_absolute_time());
    if (diff > -1000000) {
        printf("ERROR: doesn't seem like timer was stopped\n");
        return - 1;
    }
    diff = absolute_time_diff_us(wakeup_time, aon_timer_get_absolute_time());
    printf("Woken up now @%dus since target\n", (int)diff);
    if (diff < 0) {
        printf("WARNING: Woke up too soon - is this within the resolution of the aon timer?\n");
    }
    printf("Doing %d second pause to prove timer running\n", SLEEP_TIME_S);
    busy_wait_ms(SLEEP_TIME_MS);



    // powman states
#if HAS_POWMAN_TIMER
    // pstate with sram0 on
    printf("Going to PSTATE with persistent data on for %d seconds\n", SLEEP_TIME_S);

    if (my_number != 12345) {
        printf("ERROR: my_number is %d not 12345 - initialisation issue?\n", my_number);
        return -1;
    }
    my_number = 67890;
    if (my_other_numer != 12345) {
        printf("ERROR: my_other_numer is %d not 12345 - initialisation issue?\n", my_other_numer);
        return -1;
    }
    my_other_numer = 67890;

    start_time = aon_timer_get_absolute_time();

    wakeup_time = delayed_by_ms(start_time, SLEEP_TIME_MS);
    powman_hw->scratch[0] = to_us_since_boot(wakeup_time) & 0xFFFFFFFF;
    powman_hw->scratch[1] = to_us_since_boot(wakeup_time) >> 32;
    ret = low_power_pstate_until_aon_timer(wakeup_time, NULL, pstate_resume_func);

    printf("%d low_power_pstate_until_aon_timer returned\n", ret);
    while (true) {
        printf("Waiting\n");
        busy_wait_ms(1000);
    }

post_pstate_sram_on:
    // restore from scratch
    wakeup_time = from_us_since_boot((uint64_t)powman_hw->scratch[1] << 32 | (uint64_t)powman_hw->scratch[0]);
    diff = absolute_time_diff_us(wakeup_time, aon_timer_get_absolute_time());
    printf("Woken up now @%dus since target\n", (int)diff);
    if (diff < 0) {
        printf("WARNING: Woke up too soon - is this within the resolution of the aon timer?\n");
    }

    if (my_number != 67890) {
        printf("ERROR: my_number is %d not 67890 - SRAM has been re-loaded\n", my_number);
        return -1;
    } else {
        printf("my_number in sram: %d\n", my_number);
    }
    if (my_other_numer != 12345) {
        printf("ERROR: my_other_numer is %d not 12345 - SRAM has not been re-loaded\n", my_other_numer);
        return -1;
    } else {
        printf("my_other_numer in sram: %d\n", my_other_numer);
    }

    printf("Doing %d second pause to prove timer running\n", SLEEP_TIME_S);
    busy_wait_ms(SLEEP_TIME_MS);



    // pstate with sram off
    printf("Going to PSTATE with SRAM off for %d seconds\n", SLEEP_TIME_S);

    start_time = aon_timer_get_absolute_time();

    wakeup_time = delayed_by_ms(start_time, SLEEP_TIME_MS);
    powman_hw->scratch[0] = to_us_since_boot(wakeup_time) & 0xFFFFFFFF;
    powman_hw->scratch[1] = to_us_since_boot(wakeup_time) >> 32;
    pstate = pstate_bitset_none();
    ret = low_power_pstate_until_aon_timer(wakeup_time, &pstate, pstate_resume_func);

    printf("%d low_power_pstate_until_aon_timer returned\n", ret);
    while (true) {
        printf("Waiting\n");
        busy_wait_ms(1000);
    }

post_pstate_sram_off:
    // restore from scratch
    wakeup_time = from_us_since_boot((uint64_t)powman_hw->scratch[1] << 32 | (uint64_t)powman_hw->scratch[0]);
    diff = absolute_time_diff_us(wakeup_time, aon_timer_get_absolute_time());
    printf("Woken up now @%dus since target\n", (int)diff);
    if (diff < 0) {
        printf("WARNING: Woke up too soon - is this within the resolution of the aon timer?\n");
    }

    if (my_number != 34567) {
        printf("ERROR: my_number is %d not 34567 - SRAM has not been re-loaded\n", my_number);
        return -1;
    } else {
        printf("my_number in sram: %d\n", my_number);
    }

    printf("Doing %d second pause to prove timer running\n", SLEEP_TIME_S);
    busy_wait_ms(SLEEP_TIME_MS);
#endif

    printf("SUCCESS\n");

    return 0;
}