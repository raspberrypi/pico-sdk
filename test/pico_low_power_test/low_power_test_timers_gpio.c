/**
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "low_power_test_common.h"

// regression test for https://github.com/raspberrypi/pico-sdk/issues/3115 - a timed dormant/Pstate
// sleep must disarm its wakeup alarm, otherwise a following GPIO dormant/Pstate wakes immediately

bool repeater(repeating_timer_t *timer) {
    if (aon_timer_is_running()) {
        printf("  Repeating timer at %dms (aon: %dms)\n", to_ms_since_boot(get_absolute_time()), to_ms_since_boot(aon_timer_get_absolute_time()));
    } else {
        printf("  Repeating timer at %dms (aon: not running)\n", to_ms_since_boot(get_absolute_time()));
    }
    status_led_set_state(!status_led_get_state());
    return true;
}

#if HAS_POWMAN_TIMER
static bool came_from_pstate = false;
static char powman_last_pwrup[100];
static char powman_last_pstate[100];

void pstate_resume_func(__unused pstate_bitset_t *pstate) {
    came_from_pstate = true;
    pstate_resume_func_common(pstate, powman_last_pwrup, powman_last_pstate);
}
#endif

int main() {
    stdio_init_all();
    status_led_init();
    printf("Hello Sleep!\n");
    init_external_gpios();
    low_power_set_external_clock_source(DORMANT_CLOCK_HZ_DEFAULT, RTC_GPIO_IN);

    // use a repeating timer; it should be gated
    // during our sleep (todo not sure how it affects power!)
    repeating_timer_t repeat;
    add_repeating_timer_ms(500, repeater, NULL, &repeat);

#if HAS_POWMAN_TIMER
    if (came_from_pstate) {
        printf("Came from powerup %s with (%s) memory kept on - skipping to end\n", powman_last_pwrup, powman_last_pstate);
        if (powman_hw->scratch[5] == 0) {
            goto pstate_gpio_test;
        }
        goto post_pstate_gpio;
    }
    powman_hw->scratch[5] = 0;
#endif

    printf("Waiting %d seconds\n", SLEEP_TIME_S); // so we can see some repeat printfs
    busy_wait_ms(SLEEP_TIME_MS);

    int ret;

    low_power_start_aon_timer_at_time_ms(0);

    printf("Going DORMANT for %d seconds via AON TIMER\n", SLEEP_TIME_S);

    gpio_put(SLEEP_MONITOR_PIN, 0);
    ret = low_power_dormant_for_ms(SLEEP_TIME_MS, DORMANT_CLOCK_SOURCE_DEFAULT, NULL);
    gpio_put(SLEEP_MONITOR_PIN, 1);
    if (ret != PICO_OK) {
        printf("ERROR: %d returned by low_power_dormant_for_ms\n", ret);
#if PICO_RP2040
        if (ret == PICO_ERROR_PRECONDITION_NOT_MET) {
            printf("ERROR: RTC clock source is not running - connect a device running external_sleep_timer to GPIO %d\n", RTC_GPIO_IN);
        }
#endif
    }
    printf("Doing %d second pause to prove timer running\n", SLEEP_TIME_S);
    busy_wait_ms(SLEEP_TIME_MS);

    printf("Going DORMANT until GPIO wakeup\n");

    gpio_put(SLEEP_MONITOR_PIN, 0);
    low_power_dormant_until_gpio_pin_state(WAKE_UP_PIN, true, false, DORMANT_CLOCK_SOURCE_ROSC, NULL);
    gpio_put(SLEEP_MONITOR_PIN, 1);
    printf("Doing %d second pause to prove timer running\n", SLEEP_TIME_S);
    busy_wait_ms(SLEEP_TIME_MS);

#if HAS_POWMAN_TIMER
    printf("Going to PSTATE for %d seconds via AON TIMER\n", SLEEP_TIME_S);

    // Setup ext_ctrl0 to output on the SLEEP_MONITOR_PIN
    init_powman_ext_ctrl();

    gpio_put(SLEEP_MONITOR_PIN, 0);
    low_power_set_pins_low_leakage_exclude_mask(USED_PIN_MASK);
    ret = low_power_pstate_for_ms(SLEEP_TIME_MS, NULL, pstate_resume_func);

    printf("ERROR: %d returned by low_power_pstate_for_ms\n", ret);
    while (true) {
        printf("Waiting\n");
        busy_wait_ms(1000);
    }

pstate_gpio_test:
    powman_hw->scratch[5] = 1;

    printf("Doing %d second pause to prove timer running\n", SLEEP_TIME_S);
    busy_wait_ms(SLEEP_TIME_MS);

    printf("Going to PSTATE until GPIO wakeup\n");

    // Setup ext_ctrl0 to output on the SLEEP_MONITOR_PIN
    init_powman_ext_ctrl();

    gpio_put(SLEEP_MONITOR_PIN, 0);
    low_power_set_pins_low_leakage_exclude_mask(USED_PIN_MASK);
    ret = low_power_pstate_until_gpio_pin_state(WAKE_UP_PIN, true, false, NULL, pstate_resume_func);

    printf("ERROR: %d returned by low_power_pstate_until_gpio_pin_state\n", ret);
    while (true) {
        printf("Waiting\n");
        busy_wait_ms(1000);
    }

post_pstate_gpio:

    printf("Doing %d second pause to prove timer running\n", SLEEP_TIME_S);
    busy_wait_ms(SLEEP_TIME_MS);
#endif

    printf("PASSED\n");

    return 0;
}
