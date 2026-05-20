/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/status_led.h"

#include "hardware/sync/spin_lock.h"

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
#include "hardware/pio.h"
#include "pico/time.h"
#include "ws2812.pio.h"

// PICO_CONFIG: PICO_COLORED_STATUS_LED_WS2812_FREQ, Frequency per bit for the WS2812 colored status LED, type=int, default=800000, group=pico_status_led
#ifndef PICO_COLORED_STATUS_LED_WS2812_FREQ
#define PICO_COLORED_STATUS_LED_WS2812_FREQ 800000
#endif

// PICO_CONFIG: PICO_COLORED_STATUS_LED_RESET_DELAY_US, Required reset delay in microseconds for the WS2812 colored status LED, type=int, default=50, group=pico_status_led
#ifndef PICO_COLORED_STATUS_LED_RESET_DELAY_US
#define PICO_COLORED_STATUS_LED_RESET_DELAY_US 50
#endif

#ifndef PICO_COLORED_STATUS_LED_USE_DEFAULT_ALARM_POOL
#define PICO_COLORED_STATUS_LED_USE_DEFAULT_ALARM_POOL !PICO_TIME_DEFAULT_ALARM_POOL_DISABLED
#endif

static PIO pio;
static uint sm;
static uint offset;
static uint32_t next_value;
static uint64_t next_safe_set_time;
#if PICO_COLORED_STATUS_LED_USE_DEFAULT_ALARM_POOL
static alarm_id_t alarm_id;
static int8_t alarm_pending;
#else
#define alarm_pending false
#endif

// 2 + is empirical and allows WS2812 reset delay of 50us to work 
#define COLORED_STATUS_LED_UPDATE_TIME_US (2 + (1000000 * (PICO_COLORED_STATUS_LED_USES_WRGB ? 32 : 24)) / PICO_COLORED_STATUS_LED_WS2812_FREQ)

// Extract from 0xWWRRGGBB
#define RED(c) (((c) >> 16) & 0xff)
#define GREEN(c) (((c) >> 8) & 0xff)
#define BLUE(c) (((c) >> 0) & 0xff)
#define WHITE(c) (((c) >> 24) && 0xff)

static void unsafe_set_ws2812(uint32_t value, uint64_t now) {
    if (pio) {
        pio_sm_drain_tx_fifo(pio, sm); // want to jump past any previous queued values
#if PICO_COLORED_STATUS_LED_USES_WRGB
        // Convert to 0xWWGGRRBB
        pio_sm_put_blocking(pio, sm, WHITE(value) << 24 | GREEN(value) << 16 | RED(value) << 8 | BLUE(value));
#else
        // Convert to 0xGGRRBB00
        pio_sm_put_blocking(pio, sm, GREEN(value) << 24 | RED(value) << 16 | BLUE(value) << 8);
#endif
        next_safe_set_time = now + COLORED_STATUS_LED_UPDATE_TIME_US + PICO_COLORED_STATUS_LED_RESET_DELAY_US;
    }
}

#if PICO_COLORED_STATUS_LED_USE_DEFAULT_ALARM_POOL
static int64_t deferred_set_ws2812(__unused alarm_id_t id, __unused  void *user_data) {
    spin_lock_t *spin_lock = spin_lock_instance(PICO_SPINLOCK_ID_ATOMIC);
    uint32_t save = spin_lock_blocking(spin_lock);
    unsafe_set_ws2812(next_value, time_us_64());
    alarm_id = 0;
    alarm_pending--;
    spin_unlock(spin_lock, save);
    return 0;
}
#endif

static void set_ws2812(uint32_t value) {
    spin_lock_t *spin_lock = spin_lock_instance(PICO_SPINLOCK_ID_ATOMIC);
    uint32_t save = spin_lock_blocking(spin_lock);
    next_value = value;
    while (true) {
#if !PICO_COLORED_STATUS_LED_USE_DEFAULT_ALARM_POOL
        uint64_t now = time_us_64();
        if (now >= next_safe_set_time) {
            unsafe_set_ws2812(value, now);
            spin_unlock(spin_lock, save);
            break;
        } else {
            spin_unlock(spin_lock, save);
            busy_wait_until(from_ms_since_boot(next_safe_set_time));
            save = spin_lock_blocking(spin_lock);
        }
#else
        if (alarm_pending) {
            // we defer the set to the already waiting alarm
            spin_unlock(spin_lock, save);
            break;
        } else {
            uint64_t now = time_us_64();
            if (now >= next_safe_set_time) {
                unsafe_set_ws2812(value, now);
                spin_unlock(spin_lock, save);
                break;
            } else {
                // we want to defer the set until it is safe to do so
                //
                // note we use alarm_pending separate from alarm_id, as alarm_id may be returned even if the
                // alarm fires during the add_alarm_at. and don't use a boolean because if we fail
                // to add the alarm, we don't know what has happened in between since we unlock the spin lock
                // before adding the alarm since that is a slowish call
                alarm_pending++;
                spin_unlock(spin_lock, save);
                alarm_id = add_alarm_at(from_ms_since_boot(next_safe_set_time), deferred_set_ws2812, NULL, true);
                if (alarm_id > 0) break;
                busy_wait_until(from_ms_since_boot(next_safe_set_time));
                save = spin_lock_blocking(spin_lock);
                alarm_pending--;
            }
        }
#endif
    }
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
    if (colored_status_led_supported()) {
#if COLORED_STATUS_LED_USING_WS2812_PIO
        if (led_on) {
            // Turn the LED "on" even if it was already on, as the color might have changed
            set_ws2812(colored_status_led_on_color);
        } else if (colored_status_led_on) {
            set_ws2812(0);
        }
        colored_status_led_on = led_on;
        return true;
#endif
    }
    ((void)led_on);
    return false;
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
#if PICO_COLORED_STATUS_LED_USE_DEFAULT_ALARM_POOL
    if (alarm_id > 0) {
        cancel_alarm(alarm_id);
        alarm_id = 0;
    }
    alarm_pending = 0;
#endif
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
