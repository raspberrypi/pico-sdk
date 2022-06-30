/*
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_CYW43_ARCH_ARCH_COMMON_H
#define _PICO_CYW43_ARCH_ARCH_COMMON_H

#include "pico.h"
#include "pico/time.h"
#include "hardware/gpio.h"
#include "pico/error.h"

#ifdef __cplusplus
extern "C" {
#endif

// Note, these are negated, because cyw43_driver negates them before returning!
#define CYW43_EPERM            (-PICO_ERROR_NOT_PERMITTED) // Operation not permitted
#define CYW43_EIO              (-PICO_ERROR_IO) // I/O error
#define CYW43_EINVAL           (-PICO_ERROR_INVALID_ARG) // Invalid argument
#define CYW43_ETIMEDOUT        (-PICO_ERROR_TIMEOUT) // Connection timed out

#define CYW43_NUM_GPIOS        CYW43_WL_GPIO_COUNT

#define cyw43_hal_pin_obj_t uint

// get the number of elements in a fixed-size array
#define CYW43_ARRAY_SIZE(a) count_of(a)

static inline uint32_t cyw43_hal_ticks_us(void) {
    return time_us_32();
}

static inline uint32_t cyw43_hal_ticks_ms(void) {
    return to_ms_since_boot(get_absolute_time());
}

static inline int cyw43_hal_pin_read(cyw43_hal_pin_obj_t pin) {
    return gpio_get(pin);
}

static inline void cyw43_hal_pin_low(cyw43_hal_pin_obj_t pin) {
    gpio_clr_mask(1 << pin);
}

static inline void cyw43_hal_pin_high(cyw43_hal_pin_obj_t pin) {
    gpio_set_mask(1 << pin);
}

#define CYW43_HAL_PIN_MODE_INPUT           (GPIO_IN)
#define CYW43_HAL_PIN_MODE_OUTPUT          (GPIO_OUT)

#define CYW43_HAL_PIN_PULL_NONE            (0)
#define CYW43_HAL_PIN_PULL_UP              (1)
#define CYW43_HAL_PIN_PULL_DOWN            (2)

static inline void cyw43_hal_pin_config(cyw43_hal_pin_obj_t pin, uint32_t mode, uint32_t pull, __unused uint32_t alt) {
    assert((mode == CYW43_HAL_PIN_MODE_INPUT || mode == CYW43_HAL_PIN_MODE_OUTPUT) && alt == 0);
    gpio_set_dir(pin, mode);
    gpio_set_pulls(pin, pull == CYW43_HAL_PIN_PULL_UP, pull == CYW43_HAL_PIN_PULL_DOWN);
}

void cyw43_hal_get_mac(int idx, uint8_t buf[6]);

void cyw43_hal_generate_laa_mac(int idx, uint8_t buf[6]);

#ifdef __cplusplus
}
#endif

#endif

