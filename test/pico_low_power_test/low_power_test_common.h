#ifndef LOW_POWER_TEST_COMMON_H
#define LOW_POWER_TEST_COMMON_H

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/low_power.h"
#include "pico/aon_timer.h"
#include "pico/status_led.h"
#include "hardware/structs/xip_ctrl.h"

#define SLEEP_TIME_S 2
#define SLEEP_TIME_MS SLEEP_TIME_S * 1000

// Use default I2C pins as that should be connected to the other board's I2C pins
#define SLEEP_MONITOR_PIN PICO_DEFAULT_I2C_SDA_PIN
#define WAKE_UP_PIN PICO_DEFAULT_I2C_SCL_PIN

// On RP2040 this must be a GPIO that supports clock input, see the GPIO function table in the datasheet.
#define RTC_GPIO_IN 22

// On RP2040 this must be a GPIO that supports clock output, see the GPIO function table in the datasheet.
#define RTC_GPIO_OUT 21

#define USED_PIN_MASK ((1u << SLEEP_MONITOR_PIN)|(1u << WAKE_UP_PIN)|(1u << PICO_DEFAULT_UART_TX_PIN)|(1u << PICO_DEFAULT_UART_RX_PIN))


static inline void init_external_gpios(void) {
    gpio_init(SLEEP_MONITOR_PIN);
    gpio_set_dir(SLEEP_MONITOR_PIN, GPIO_OUT);
    gpio_put(SLEEP_MONITOR_PIN, 1);
    gpio_init(WAKE_UP_PIN);
    gpio_pull_up(WAKE_UP_PIN);
}

#if HAS_POWMAN_TIMER
static inline void init_powman_ext_ctrl(void) {
    powman_hw->ext_ctrl[0] = POWMAN_EXT_CTRL0_LP_EXIT_STATE_BITS | POWMAN_EXT_CTRL0_INIT_STATE_BITS | SLEEP_MONITOR_PIN;
    hw_set_bits(&powman_hw->ext_ctrl[0], POWMAN_EXT_CTRL0_INIT_BITS);
}
#endif

#endif // LOW_POWER_TEST_COMMON_H