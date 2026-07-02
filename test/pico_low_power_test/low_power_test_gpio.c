/**
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "low_power_test_common.h"

bool repeater(repeating_timer_t *timer) {
    if (aon_timer_is_running()) {
        printf("  Repeating timer at %dms (aon: %dms)\n", to_ms_since_boot(get_absolute_time()), to_ms_since_boot(aon_timer_get_absolute_time()));
    } else {
        printf("  Repeating timer at %dms (aon: not running)\n", to_ms_since_boot(get_absolute_time()));
    }
    status_led_set_state(!status_led_get_state());
    return true;
}

void chars_available_callback(__unused void *param) {
    char buf[16] = {0};
    while (stdio_get_until(buf, sizeof(buf), make_timeout_time_us(10)) > 0) {
        printf("Chars available callback: %s\n", buf);
    }
}

#if HAS_POWMAN_TIMER
static bool came_from_pstate = false;
static char powman_last_pwrup[100];
static char powman_last_pstate[100];

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
}
#endif

int main() {
    stdio_init_all();
    status_led_init();
    printf("Hello Sleep!\n");
    init_external_gpios();
    // use a repeating timer; it should be gated
    // during our sleep (todo not sure how it affects power!)
    repeating_timer_t repeat;
    add_repeating_timer_ms(500, repeater, NULL, &repeat);

    // test stdio_set_chars_available_callback
    stdio_set_chars_available_callback(chars_available_callback, NULL);

#if HAS_POWMAN_TIMER
    if (came_from_pstate) {
        printf("Came from powerup %s with (%s) memory kept on - skipping to end\n", powman_last_pwrup, powman_last_pstate);
        goto post_pstate_gpio;
    }
#endif

    printf("Waiting %d seconds\n", SLEEP_TIME_S); // so we can see some repeat printfs
    busy_wait_ms(SLEEP_TIME_MS);

    int ret;

    printf("Going to sleep until GPIO wakeup\n");

    gpio_put(SLEEP_MONITOR_PIN, 0);
    low_power_sleep_until_gpio_pin_state(WAKE_UP_PIN, true, false, NULL, true);
    gpio_put(SLEEP_MONITOR_PIN, 1);
    printf("Doing %d second pause to prove timer running\n", SLEEP_TIME_S);
    busy_wait_ms(SLEEP_TIME_MS);

    printf("Going to non-exclusive sleep until GPIO wakeup\n");

    // need to keep the timer running
    clock_dest_bitset_t keep_enabled = clock_dest_bitset_none();
#if PICO_RP2040
    clock_dest_bitset_add(&keep_enabled, CLK_DEST_SYS_TIMER);
#else
    clock_dest_bitset_add(&keep_enabled, CLK_DEST_SYS_TIMER0);
    clock_dest_bitset_add(&keep_enabled, CLK_DEST_REF_TICKS);
#endif

    gpio_put(SLEEP_MONITOR_PIN, 0);
    low_power_sleep_until_gpio_pin_state(WAKE_UP_PIN, true, false, &keep_enabled, false);
    gpio_put(SLEEP_MONITOR_PIN, 1);
    printf("Doing %d second pause to prove timer running\n", SLEEP_TIME_S);
    busy_wait_ms(SLEEP_TIME_MS);

    // Skip this test as it requires stdin characters
    // printf("Going to sleep until any wakeup (expecting stdin characters)\n");

    // gpio_put(SLEEP_MONITOR_PIN, 0);
    // low_power_sleep_until_irq(NULL);
    // gpio_put(SLEEP_MONITOR_PIN, 1);
    // printf("Doing %d second pause to prove timer running\n", SLEEP_TIME_S);
    // busy_wait_ms(SLEEP_TIME_MS);

    low_power_start_aon_timer_at_time_ms(0);

    printf("Going DORMANT until GPIO wakeup\n");

    gpio_put(SLEEP_MONITOR_PIN, 0);
    low_power_dormant_until_gpio_pin_state(WAKE_UP_PIN, true, false, DORMANT_CLOCK_SOURCE_ROSC, NULL);
    gpio_put(SLEEP_MONITOR_PIN, 1);
    printf("Doing %d second pause to prove timer running\n", SLEEP_TIME_S);
    busy_wait_ms(SLEEP_TIME_MS);

#if HAS_POWMAN_TIMER
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
