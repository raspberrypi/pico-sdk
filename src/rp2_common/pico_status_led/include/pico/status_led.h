/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** \file pico/status_led.h
 *  \defgroup pico_status_led pico_status_led
 *
 * \brief Enables access to the on-board status leds
 *
 * Boards usually have access to an on-board status leds which are configured via the board header (\see PICO_DEFAULT_LED_PIN and \see PICO_DEFAULT_WS2812_PIN)
 * This library hides the details so you can use the status leds for all boards without changing your code.
 */

#ifndef _PICO_STATUS_LED_H
#define _PICO_STATUS_LED_H

#include "hardware/gpio.h"

#if defined(CYW43_WL_GPIO_LED_PIN)
#include "cyw43.h"
#endif

struct async_context;

#ifdef __cplusplus
extern "C" {
#endif

// PICO_CONFIG: PICO_STATUS_LED_WS2812_WRGB, Indicate if the colored status led supports WRGB, type=bool, default=0, group=pico_status_led
#ifndef PICO_STATUS_LED_WS2812_WRGB
#define PICO_STATUS_LED_WS2812_WRGB 0
#endif

// PICO_CONFIG: PICO_STATUS_LED_COLOR_ONLY, Indicate if only the colored status led should be used. Only true by default if a WS2812 pin is defined and no led pin is defined, type=bool, group=pico_status_led
#ifdef PICO_DEFAULT_WS2812_PIN
#ifndef PICO_STATUS_LED_COLOR_ONLY
#define PICO_STATUS_LED_COLOR_ONLY !(defined PICO_DEFAULT_LED_PIN || defined CYW43_WL_GPIO_LED_PIN)
#endif
#else
// Force this off if PICO_DEFAULT_WS2812_PIN is not defined
#undef PICO_STATUS_LED_COLOR_ONLY
#define PICO_STATUS_LED_COLOR_ONLY 0
#endif

/*! \brief Generate an RGB colour value for /ref pico_status_led_color_set_on_value
 *  \ingroup pico_status_led
 */
#ifndef PICO_STATUS_LED_RGB
#define PICO_STATUS_LED_RGB(R, G, B) (((R) << 16) | ((G) << 8) | (B))
#endif

/*! \brief Generate an WRGB colour value for \ref pico_status_led_color_set_on_value
 *
 *  \note: If your hardware does not support a white pixel, the white component is ignored
 *
 *  \ingroup pico_status_led
 */
#ifndef PICO_STATUS_LED_WRGB
#define PICO_STATUS_LED_WRGB(W, R, G, B) (((W) << 24) | ((R) << 16) | ((G) << 8) | (B))
#endif

// PICO_CONFIG: PICO_STATUS_LED_COLOR_ON_DEFAULT, the default pixel color value of the coloured status led when it is on, type=int, group=pico_status_led
#ifndef PICO_STATUS_LED_COLOR_ON_DEFAULT
#if PICO_STATUS_LED_WS2812_WRGB
#define PICO_STATUS_LED_COLOR_ON_DEFAULT PICO_STATUS_LED_WRGB(0xaa, 0, 0, 0)
#else
#define PICO_STATUS_LED_COLOR_ON_DEFAULT PICO_STATUS_LED_RGB(0xaa, 0xaa, 0xaa)
#endif
#endif

// PICO_CONFIG: PICO_STATUS_LED_COLOR_OFF, the pixel color value of the coloured status led when it is off, type=int, group=pico_status_led
#ifndef PICO_STATUS_LED_COLOR_OFF
#if PICO_STATUS_LED_WS2812_WRGB
#define PICO_STATUS_LED_COLOR_OFF PICO_STATUS_LED_WRGB(0, 0, 0, 0)
#else
#define PICO_STATUS_LED_COLOR_OFF PICO_STATUS_LED_RGB(0, 0, 0)
#endif
#endif

/*! \brief Initialise the status leds
 *  \ingroup pico_status_led
 *
 *  Initialise the status leds and the resources they need before use.
 *
 * \note: You must call this function before using any other status led functions.
 *
 * \param context An async context is needed to control the led on some devices (e.g. Pico W).
 * You can usually only have one async context. Pass your async context into the function or if you don't have one, pass NULL to get the function to just create a context for it's own use as and if required.
 * \return Returns true if the led was initialised successfully, otherwise false on failure
 */
bool pico_status_led_init(struct async_context *context);

/*! \brief Set the color used for the status led when it is on
 *  \ingroup pico_status_led
 *
 * \note: If your hardware does not support a colored status led (\see PICO_DEFAULT_WS2812_PIN), this function does nothing and returns false.
 *
 * \param value The color to use for the colored status led when it is on, in 0xWWRRGGBB format
 * \param True if the coloured status led could be set, otherwise false on failure
 */
bool pico_status_led_color_set_on_value(uint32_t value);

/*! \brief Get the color used for the status led value when it is on
 *  \ingroup pico_status_led
 *
 * \note: If your hardware does not support a colored status led (\see PICO_DEFAULT_WS2812_PIN), this function always returns 0x0.
 *
 * \return The color used for the colored status led when it is on, in 0xWWRRGGBB format
 */
uint32_t pico_status_led_color_get_on_value(void);

/*! \brief Set the colored status led on or off
 *  \ingroup pico_status_led
 *
 * \note: If your hardware does not support a colored status led (\see PICO_DEFAULT_WS2812_PIN), this function does nothing and returns false.
 *
 * \param led_on True to turn the colored led on. Pass False to turn the colored led off
 * \param True if the colored status led could be set, otherwise false
 */
bool pico_status_led_color_set(bool led_on);

/*! \brief Get the state of the colored status led
 *  \ingroup pico_status_led
 *
 * \note: If your hardware does not support a colored status led (\see PICO_DEFAULT_WS2812_PIN), this function returns false.
 *
 * \return True if the colored status led is on, or False if the coloured status led is off
 */
bool pico_status_led_color_get(void);

/*! \brief Set the status led on or off
 *  \ingroup pico_status_led
 *
 * \note: If your hardware does not support a status led (\see PICO_DEFAULT_LED_PIN), this function does nothing and returns false.
 *
 * \param led_on True to turn the led on. Pass False to turn the led off
 * \param True if the status led could be set, otherwise false
 */
static inline bool pico_status_led_set(bool led_on) {
#if PICO_STATUS_LED_COLOR_ONLY
    return pico_status_led_color_set(led_on);
#elif defined PICO_DEFAULT_LED_PIN
#if defined(PICO_DEFAULT_LED_PIN_INVERTED) && PICO_DEFAULT_LED_PIN_INVERTED
    gpio_put(PICO_DEFAULT_LED_PIN, !led_on);
#else
    gpio_put(PICO_DEFAULT_LED_PIN, led_on);
#endif
    return true;
#elif defined CYW43_WL_GPIO_LED_PIN
    cyw43_gpio_set(&cyw43_state, CYW43_WL_GPIO_LED_PIN, led_on);
    return true;
#else
    return false;
#endif
}

/*! \brief Get the state of the status led
 *  \ingroup pico_status_led
 *
 * \note: If your hardware does not support a status led (\see PICO_DEFAULT_LED_PIN), this function always returns false.
 *
 * \return True if the status led is on, or False if the status led is off
 */
static inline bool pico_status_led_get(void) {
#if PICO_STATUS_LED_COLOR_ONLY
    return pico_status_led_color_get();
#elif defined PICO_DEFAULT_LED_PIN
#if defined(PICO_DEFAULT_LED_PIN_INVERTED) && PICO_DEFAULT_LED_PIN_INVERTED
    return !gpio_get(PICO_DEFAULT_LED_PIN);
#else
    return gpio_get(PICO_DEFAULT_LED_PIN);
#endif
#elif defined CYW43_WL_GPIO_LED_PIN
    bool value = false;
    cyw43_gpio_get(&cyw43_state, CYW43_WL_GPIO_LED_PIN, &value);
    return value;
#else
    return false;
#endif
}

/*! \brief Deinitialise the status leds
 *  \ingroup pico_status_led
 *
 * Deinitialises the status leds when they are no longer needed.
 *
 * \param context The async context to be used. This should be the same as the value passed into pico_status_led_init
 */
void pico_status_led_deinit(struct async_context *context);

#ifdef __cplusplus
}
#endif

#endif
