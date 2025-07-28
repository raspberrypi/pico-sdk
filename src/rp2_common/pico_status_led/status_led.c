/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/status_led.h"

#if PICO_STATUS_LED_AVAILABLE && defined(CYW43_WL_GPIO_LED_PIN) && !defined(PICO_DEFAULT_LED_PIN)
#define STATUS_LED_USING_WL_GPIO 1
#else
#define STATUS_LED_USING_WL_GPIO 0
#endif

#if PICO_STATUS_LED_AVAILABLE && defined(PICO_DEFAULT_LED_PIN) && !STATUS_LED_USING_WL_GPIO
#define STATUS_LED_USING_GPIO 1
#else
#define STATUS_LED_USING_GPIO 0
#endif

#if PICO_COLORED_STATUS_LED_AVAILABLE && defined(PICO_DEFAULT_WS2812_PIN)
#define COLORED_STATUS_LED_USING_WS2812_PIO 1
#else
#define COLORED_STATUS_LED_USING_WS2812_PIO 0
#endif

#if STATUS_LED_USING_WL_GPIO
#include "pico/cyw43_driver.h"
#include "pico/async_context_threadsafe_background.h"
#endif

static uint32_t colored_status_led_on_color = PICO_DEFAULT_COLORED_STATUS_LED_ON_COLOR;
static bool colored_status_led_on;

#if COLORED_STATUS_LED_USING_WS2812_PIO
#include <hardware/pio.h>
#include "ws2812.pio.h"

// PICO_CONFIG: PICO_COLORED_STATUS_LED_WS2812_FREQ, Frequency per bit for the WS2812 colored status LED, type=int, default=800000, group=pico_status_led
#ifndef PICO_COLORED_STATUS_LED_WS2812_FREQ
#define PICO_COLORED_STATUS_LED_WS2812_FREQ 800000
#endif

static PIO pio;
static uint sm;
static uint offset;

// Extract from 0xWWRRGGBB
#define RED(c) (((c) >> 16) & 0xff)
#define GREEN(c) (((c) >> 8) & 0xff)
#define BLUE(c) (((c) >> 0) & 0xff)
#define WHITE(c) (((c) >> 24) && 0xff)

bool set_ws2812(uint32_t value) {
    if (pio) {
#if PICO_COLORED_STATUS_LED_USES_WRGB
        // Convert to 0xWWGGRRBB
        pio_sm_put_blocking(pio, sm, WHITE(value) << 24 | GREEN(value) << 16 | RED(value) << 8 | BLUE(value));
#else
        // Convert to 0xGGRRBB00
        pio_sm_put_blocking(pio, sm, GREEN(value) << 24 | RED(value) << 16 | BLUE(value) << 8);
#endif
        return true;
    }
    return false;
}
#endif

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
#if COLORED_STATUS_LED_USING_WS2812_PIO
        success = true;
        if (led_on && !colored_status_led_on) {
            success = set_ws2812(colored_status_led_on_color);
        } else if (!led_on && colored_status_led_on) {
            success = set_ws2812(0);
        }
#endif
    }
    if (success) colored_status_led_on = led_on;
    return success;
}

bool colored_status_led_get_state(void) {
    return colored_status_led_on;
}

#if STATUS_LED_USING_WL_GPIO
static async_context_threadsafe_background_t status_led_owned_context;
static struct async_context *status_led_context;
#endif

static bool status_led_init_internal(__unused struct async_context *context) {
    bool success = false;
    // ---- regular status LED ----
#if STATUS_LED_USING_GPIO
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    success = true;
#elif STATUS_LED_USING_WL_GPIO
    assert(!status_led_context);
    if (!context) {
        // for CYW43 init, we defer to the context method for the real work
        async_context_threadsafe_background_config_t config = async_context_threadsafe_background_default_config();
        if (async_context_threadsafe_background_init(&status_led_owned_context, &config)) {
            if (cyw43_driver_init(&status_led_owned_context.core)) {
                context = &status_led_owned_context.core;
            } else {
                async_context_deinit(&status_led_owned_context.core);
                return false;
            }
        }
    }
    status_led_context = context;
    success = true;
#endif

    // ---- colored status LED ----
#if COLORED_STATUS_LED_USING_WS2812_PIO
    if (pio_claim_free_sm_and_add_program_for_gpio_range(&ws2812_program, &pio, &sm, &offset, PICO_DEFAULT_WS2812_PIN, 1, true)) {
        ws2812_program_init(pio, sm, offset, PICO_DEFAULT_WS2812_PIN, PICO_COLORED_STATUS_LED_WS2812_FREQ, PICO_COLORED_STATUS_LED_USES_WRGB);
    } else {
        status_led_deinit();
        return false;
    }
#ifdef PICO_DEFAULT_WS2812_POWER_PIN
    gpio_init(PICO_DEFAULT_WS2812_POWER_PIN);
    gpio_set_dir(PICO_DEFAULT_WS2812_POWER_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_WS2812_POWER_PIN, true);
#endif
    success = true;
#endif
    return success;
}

bool status_led_init(void) {
    return status_led_init_internal(NULL);
}

bool status_led_init_with_context(struct async_context *context) {
    assert(context);
    return status_led_init_internal(context);
}

void status_led_deinit(void) {
#if STATUS_LED_USING_GPIO
    gpio_deinit(PICO_DEFAULT_LED_PIN);
#elif STATUS_LED_USING_WL_GPIO
    // Note: We only deinit if we created it
    if (status_led_context == &status_led_owned_context.core) {
        cyw43_driver_deinit(status_led_context);
        async_context_deinit(status_led_context);
    }
    status_led_context = NULL;
#endif
#if COLORED_STATUS_LED_USING_WS2812_PIO
    if (pio) {
        pio_remove_program_and_unclaim_sm(&ws2812_program, pio, sm, offset);
        pio = NULL;
    }
#ifdef PICO_DEFAULT_WS2812_POWER_PIN
    gpio_put(PICO_DEFAULT_WS2812_POWER_PIN, false);
    gpio_deinit(PICO_DEFAULT_WS2812_POWER_PIN);
#endif
#endif
}
