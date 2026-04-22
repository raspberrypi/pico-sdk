/**
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/status_led.h"
#include "pico/low_power.h"

#define SLEEP_TIME_S 2
#define SLEEP_TIME_MS SLEEP_TIME_S * 1000

#define RTC_GPIO 22 // must support clock input, see the GPIO function table in the datasheet.

bool repeater(repeating_timer_t *timer) {
    printf("  Repeating timer at %dms\n", to_ms_since_boot(get_absolute_time()));
    status_led_set_state(!status_led_get_state());
    return true;
}
 
int main() {
    stdio_init_all();
    status_led_init();

    low_power_set_external_clock_source(DORMANT_CLOCK_HZ_DEFAULT, RTC_GPIO);

    static int __persistent_data(num_runs);
    for (num_runs++; num_runs < 5; num_runs++) {    // start at 1 to prove the persistent data is working
        printf("Run %d\n", num_runs);

        printf("Going to sleep for %d seconds\n", SLEEP_TIME_S);
        low_power_sleep_for_ms(SLEEP_TIME_MS, NULL, true);
        printf("Woken up\n");

        // printf("Going dormant for %d seconds\n", SLEEP_TIME_S);
        // low_power_dormant_for_ms(SLEEP_TIME_MS, DORMANT_CLOCK_SOURCE_DEFAULT, NULL);
        // printf("Woken up\n");

#if HAS_POWMAN_TIMER
        printf("Going to Pstate for %d seconds\n", SLEEP_TIME_S);
        int ret = low_power_pstate_for_ms(SLEEP_TIME_MS, NULL, NULL);
        printf("%d ERROR: low_power_pstate_for_ms returned\n", ret);
#endif
    }

    printf("SUCCESS\n");
    return 0;
}
