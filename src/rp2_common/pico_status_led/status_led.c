/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/status_led.h"

#if defined(CYW43_WL_GPIO_LED_PIN)
#include "pico/cyw43_driver.h"
#include "pico/async_context_threadsafe_background.h"
#endif

#ifdef PICO_DEFAULT_WS2812_PIN
#include <hardware/pio.h>
#include "ws2812.pio.h"

// PICO_CONFIG: PICO_STATUS_LED_WS2812_FREQ, Frequency per bit for the WS2812 status led pio, type=int, default=800000, group=pico_status_led
#ifndef PICO_STATUS_LED_WS2812_FREQ
#define PICO_STATUS_LED_WS2812_FREQ 800000
#endif

static PIO pio;
static uint sm;
static int offset = -1;
static uint32_t ws2812_on_value = PICO_STATUS_LED_COLOR_ON_DEFAULT;
static bool ws2812_led_on;

// Extract from 0xWWRRGGBB
#define RED(C) ((C >> 16) & 0xff)
#define GREEN(C) ((C >> 8) & 0xff)
#define BLUE(C) ((C >> 0) & 0xff)
#define WHITE(C) ((C >> 24) && 0xff)

bool set_ws2812(uint32_t value) {
    if (offset > -1) {
#if PICO_STATUS_LED_WS2812_WRGB
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
#endif // PICO_STATUS_LED_WS2812_PIN

bool pico_status_led_color_set_on_value(uint32_t value) {
#ifdef PICO_DEFAULT_WS2812_PIN
    ws2812_on_value = value;
    if (ws2812_led_on) {
        return set_ws2812(ws2812_on_value);
    }
#else
    (void)value;
#endif // PICO_DEFAULT_WS2812_PIN
    return false;
}

uint32_t pico_status_led_color_get_on_value(void) {
#ifdef PICO_DEFAULT_WS2812_PIN
    return ws2812_on_value;
#else
    return 0x0;
#endif
}

bool pico_status_led_color_set(bool led_on) {
#ifdef PICO_DEFAULT_WS2812_PIN
    bool success = true;
    if (led_on && !ws2812_led_on) {
        success = set_ws2812(ws2812_on_value);
    } else if (!led_on && ws2812_led_on) {
        success = set_ws2812(PICO_STATUS_LED_COLOR_OFF);
    }
    ws2812_led_on = led_on;
    return success;
#else
    (void)led_on;
    return false;
#endif
}

bool pico_status_led_color_get(void) {
#ifdef PICO_DEFAULT_WS2812_PIN
    return ws2812_led_on;
#else
    return false;
#endif
}

#if defined(CYW43_WL_GPIO_LED_PIN)
static async_context_threadsafe_background_t status_led_context;
#endif

bool pico_status_led_init(struct async_context *context) {
#if defined(PICO_DEFAULT_LED_PIN)
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
#elif defined(CYW43_WL_GPIO_LED_PIN)
    assert(!status_led_context.core.type);
    if (!context) {
        async_context_threadsafe_background_config_t config = async_context_threadsafe_background_default_config();
        if (!async_context_threadsafe_background_init(&status_led_context, &config)) {
            return false;
        }
        if (!cyw43_driver_init(&status_led_context.core)) {
            async_context_deinit(&status_led_context.core);
            return false;
        }
    }
#endif
#if PICO_DEFAULT_WS2812_PIN
    if (pio_claim_free_sm_and_add_program_for_gpio_range(&ws2812_program, &pio, &sm, &offset, PICO_DEFAULT_WS2812_PIN, 1, true)) {
        ws2812_program_init(pio, sm, offset, PICO_DEFAULT_WS2812_PIN, PICO_STATUS_LED_WS2812_FREQ, PICO_STATUS_LED_WS2812_WRGB);
    } else {
        pico_status_led_deinit(context);
        return false;
    }
#ifdef PICO_DEFAULT_WS2812_POWER_PIN
    gpio_init(PICO_DEFAULT_WS2812_POWER_PIN);
    gpio_set_dir(PICO_DEFAULT_WS2812_POWER_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_WS2812_POWER_PIN, true);
#endif
#endif
    (void)context;
    return true;
}

void pico_status_led_deinit(struct async_context *context) {
    // Note: We cannot deinit cyw43 in case it has other users
#if defined(PICO_DEFAULT_LED_PIN)
    gpio_deinit(PICO_DEFAULT_LED_PIN);
#elif defined(CYW43_WL_GPIO_LED_PIN)
    assert((context && !status_led_context.core.type) || (!context && status_led_context.core.type));
    if (!context) {
        cyw43_driver_deinit(&status_led_context.core);
        async_context_deinit(&status_led_context.core);
    }
#endif
#if PICO_DEFAULT_WS2812_PIN
    if (offset >= 0) {
        pio_remove_program_and_unclaim_sm(&ws2812_program, pio, sm, offset);
        offset = -1;
    }
#ifdef PICO_DEFAULT_WS2812_POWER_PIN
    gpio_put(PICO_DEFAULT_WS2812_POWER_PIN, false);
    gpio_deinit(PICO_DEFAULT_WS2812_POWER_PIN);
#endif
#endif
    (void)context;
}
