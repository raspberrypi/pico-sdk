/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/status_led.h"
#include "pico/sync.h"
#include "hardware/clocks.h"

#define RTC_GPIO 21

bool repeater(repeating_timer_t *timer) {
    printf("  Repeating timer at %dms\n", to_ms_since_boot(get_absolute_time()));
    status_led_set_state(!status_led_get_state());
    return true;
}

int main() {
    stdio_init_all();
    status_led_init();

    clock_gpio_init(RTC_GPIO, CLOCKS_CLK_GPOUT3_CTRL_AUXSRC_VALUE_CLK_RTC, 1);

    repeating_timer_t repeat;
    add_repeating_timer_ms(500, repeater, NULL, &repeat);

    while (true) __wfi();

    return 0;
}