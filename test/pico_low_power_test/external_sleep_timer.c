/**
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/sync.h"
#include "hardware/clocks.h"

#include "low_power_test_common.h"

#if PICO_RP2040
#define TOLERANCE_MS 1000   // The resolution of the AON timer is 1s on RP2040
#elif PICO_RISCV
#define TOLERANCE_MS 200    // Can take longer to wake up that Arm
#else
#define TOLERANCE_MS 100
#endif
#define MIN_SLEEP_TIME_MS SLEEP_TIME_MS - TOLERANCE_MS
#define MAX_SLEEP_TIME_MS SLEEP_TIME_MS + TOLERANCE_MS

uint32_t wakeup_time_ms = 0;
uint32_t sleep_time_ms = 0;
bool good_sleep_done = false;

int64_t wake_up_gpio(__unused alarm_id_t id, __unused void *param) {
    gpio_put(WAKE_UP_PIN, 0);
    return 0;
}

static alarm_id_t wake_up_alarm_id;

void gpio_callback(uint gpio, uint32_t events) {
    if (events & GPIO_IRQ_EDGE_RISE) {
        wakeup_time_ms = to_ms_since_boot(get_absolute_time());
        printf("Woke up at %dms\n", wakeup_time_ms);
        gpio_put(WAKE_UP_PIN, 1);
    } else if (events & GPIO_IRQ_EDGE_FALL) {
        sleep_time_ms = to_ms_since_boot(get_absolute_time());
        printf("Went to sleep at %dms\n", sleep_time_ms);
        wake_up_alarm_id = add_alarm_in_ms(SLEEP_TIME_MS, wake_up_gpio, NULL, false);
    }
    if (wakeup_time_ms > sleep_time_ms) {
        uint32_t diff = wakeup_time_ms - sleep_time_ms;
        if (good_sleep_done && (diff < MIN_SLEEP_TIME_MS || diff > MAX_SLEEP_TIME_MS)) {
            printf("ERROR: Was asleep for %dms, expected between %dms and %dms\n", diff, MIN_SLEEP_TIME_MS, MAX_SLEEP_TIME_MS);
        } else if (diff >= MIN_SLEEP_TIME_MS && diff <= MAX_SLEEP_TIME_MS){
            printf("Was asleep for %dms\n", diff);
            good_sleep_done = true;
        }
    }
}


int main() {
    stdio_init_all();

#if PICO_RP2040
    printf("Outputting RTC clock to GPIO %d\n", RTC_GPIO_OUT);
    clock_gpio_init(RTC_GPIO_OUT, CLOCKS_CLK_GPOUT3_CTRL_AUXSRC_VALUE_CLK_RTC, 1);
#endif

    printf("Monitoring for sleep events on GPIO %d\n", SLEEP_MONITOR_PIN);
    gpio_init(SLEEP_MONITOR_PIN);
    gpio_pull_up(SLEEP_MONITOR_PIN);
    gpio_set_irq_enabled_with_callback(SLEEP_MONITOR_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    printf("Waking up device on GPIO %d\n", WAKE_UP_PIN);
    gpio_init(WAKE_UP_PIN);
    gpio_set_dir(WAKE_UP_PIN, GPIO_OUT);
    gpio_put(WAKE_UP_PIN, 1);

    while (true) __wfi();

    return 0;
}