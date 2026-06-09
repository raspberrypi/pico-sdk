/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/status_led.h"

static bool status_led_on;
static uint32_t colored_status_led_on_color = PICO_DEFAULT_COLORED_STATUS_LED_ON_COLOR;
static bool colored_status_led_on;

bool status_led_set_state(bool led_on) {
    bool success = false;
    if (status_led_supported()) {
        success = true;
    }
    if (success) status_led_on = led_on;
    return success;
}

bool status_led_get_state(void) {
    return status_led_on;
}

bool colored_status_led_set_on_with_color(uint32_t color) {
    colored_status_led_on_color = color;
    return colored_status_led_set_state(true);
}

uint32_t colored_status_led_get_on_color(void) {
    return colored_status_led_on_color;
}

bool colored_status_led_set_state(bool led_on) {
    bool success = false;
    if (colored_status_led_supported()) {
        success = true;
    }
    if (success) colored_status_led_on = led_on;
    return success;
}

bool colored_status_led_get_state(void) {
    return colored_status_led_on;
}

bool status_led_init(void) {
    return true;
}

bool status_led_init_with_context(__unused struct async_context *context) {
    return true;
}

void status_led_deinit(void) {
}
