/**
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "low_power_test_common.h"

bool repeater(repeating_timer_t *timer) {
    printf("  Repeating timer at %dms\n", to_ms_since_boot(get_absolute_time()));
    status_led_set_state(!status_led_get_state());
    return true;
}
 
int main() {
    stdio_init_all();
    status_led_init();

    init_external_gpios();

#if HAS_POWMAN_TIMER
    // quick test for https://github.com/raspberrypi/pico-sdk/issues/2824
    powman_set_debug_power_request_ignored(true);
    hard_assert(powman_hw->dbg_pwrcfg & POWMAN_DBG_PWRCFG_IGNORE_BITS);
    powman_set_debug_power_request_ignored(false);
    hard_assert((powman_hw->dbg_pwrcfg & POWMAN_DBG_PWRCFG_IGNORE_BITS) == 0);
#endif

    low_power_set_external_clock_source(DORMANT_CLOCK_HZ_DEFAULT, RTC_GPIO_IN);

    int ret;
    static int __persistent_data(num_runs);
    for (num_runs++; num_runs < 5; num_runs++) {    // start at 1 to prove the persistent data is working
        printf("Run %d\n", num_runs);

        printf("Going to sleep for %dms\n", SLEEP_TIME_MS);
        gpio_put(SLEEP_MONITOR_PIN, 0);
        ret = low_power_sleep_for_ms(SLEEP_TIME_MS, NULL, true);
        gpio_put(SLEEP_MONITOR_PIN, 1);
        if (ret != PICO_OK) {
            printf("ERROR: low_power_sleep_for_ms returned %d\n", ret);
        } else {
            printf("Woken up\n");
        }

        printf("Going dormant for %dms\n", SLEEP_TIME_MS);
        gpio_put(SLEEP_MONITOR_PIN, 0);
        ret = low_power_dormant_for_ms(SLEEP_TIME_MS, DORMANT_CLOCK_SOURCE_DEFAULT, NULL);
        gpio_put(SLEEP_MONITOR_PIN, 1);
        if (ret != PICO_OK) {
            printf("ERROR: low_power_dormant_for_ms returned %d\n", ret);
        } else {
            printf("Woken up\n");
        }

#if HAS_POWMAN_TIMER
        printf("Going to Pstate for %dms\n", SLEEP_TIME_MS);
        // Setup ext_ctrl0 to output on the SLEEP_MONITOR_PIN
        init_powman_ext_ctrl();
        gpio_put(SLEEP_MONITOR_PIN, 0);
        ret = low_power_pstate_for_ms(SLEEP_TIME_MS, NULL, NULL);
        if (ret != PICO_OK) {
            printf("%d ERROR: low_power_pstate_for_ms returned\n", ret);
        } else {
            printf("ERROR: Woken up from Pstate\n");
        }
#endif
    }

    printf("PASSED\n");
    return 0;
}
