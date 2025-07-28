/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** \file pico/status_led.h
 *  \defgroup pico_status_led pico_status_led
 *
 * \brief Enables access to the on-board status LED(s)
 *
 * Boards usually have access to one or two on-board status LEDs which are configured via the board header (PICO_DEFAULT_LED_PIN, CYW43_WL_GPIO_LED_PIN and/or PICO_DEFAULT_WS2812_PIN).
 * This library hides the low-level details so you can use the status LEDs for all boards without changing your code.
 * \note If your board has both a single-color LED and a colored LED, you can independently control the single-color LED with the `status_led_` APIs, and the colored LED with the `colored_status_led_` APIs
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

// PICO_CONFIG: PICO_STATUS_LED_AVAILABLE, Indicate whether a single-color status LED is available, type=bool, default=1 if PICO_DEFAULT_LED_PIN or CYW43_WL_GPIO_LED_PIN is defined; may be set by the user to 0 to not use either even if they are available, group=pico_status_led
#ifndef PICO_STATUS_LED_AVAILABLE
#if defined(PICO_DEFAULT_LED_PIN) || defined(CYW43_WL_GPIO_LED_PIN)
#define PICO_STATUS_LED_AVAILABLE 1
#else
#define PICO_STATUS_LED_AVAILABLE 0
#endif
#endif

// PICO_CONFIG: PICO_COLORED_STATUS_LED_AVAILABLE, Indicate whether a colored status LED is available, type=bool, default=1 if PICO_DEFAULT_WS2812_PIN is defined; may be set by the user to 0 to not use the colored status LED even if available, group=pico_status_led
#ifndef PICO_COLORED_STATUS_LED_AVAILABLE
#ifdef PICO_DEFAULT_WS2812_PIN
#define PICO_COLORED_STATUS_LED_AVAILABLE 1
#else
#define PICO_COLORED_STATUS_LED_AVAILABLE 0
#endif
#endif    

// PICO_CONFIG: PICO_STATUS_LED_VIA_COLORED_STATUS_LED, Indicate if the colored status LED should be used for both status_led and colored_status_led APIs, type=bool, default=1 if PICO_COLORED_STATUS_LED_AVAILABLE is 1 and PICO_STATUS_LED_AVAILABLE is 0, group=pico_status_led
#ifndef PICO_STATUS_LED_VIA_COLORED_STATUS_LED
#define PICO_STATUS_LED_VIA_COLORED_STATUS_LED (PICO_COLORED_STATUS_LED_AVAILABLE && !PICO_STATUS_LED_AVAILABLE)
#endif

// PICO_CONFIG: PICO_COLORED_STATUS_LED_USES_WRGB, Indicate if the colored status LED supports WRGB, type=bool, default=0, group=pico_status_led
#ifndef PICO_COLORED_STATUS_LED_USES_WRGB
#define PICO_COLORED_STATUS_LED_USES_WRGB 0
#endif

/*! \brief Generate an RGB color value for /ref colored_status_led_set_on_with_color
 *  \ingroup pico_status_led
 */
#ifndef PICO_COLORED_STATUS_LED_COLOR_FROM_RGB
#define PICO_COLORED_STATUS_LED_COLOR_FROM_RGB(r, g, b) (((r) << 16) | ((g) << 8) | (b))
#endif

/*! \brief Generate an WRGB color value for \ref colored_status_led_set_on_with_color
 *  \ingroup pico_status_led
 *
 *  \note If your hardware does not support a white pixel, the white component is ignored
 */
#ifndef PICO_COLORED_STATUS_LED_COLOR_FROM_WRGB
#define PICO_COLORED_STATUS_LED_COLOR_FROM_WRGB(w, r, g, b) (((w) << 24) | ((r) << 16) | ((g) << 8) | (b))
#endif

// PICO_CONFIG: PICO_DEFAULT_COLORED_STATUS_LED_ON_COLOR, the default pixel color value of the colored status LED when it is on, type=int, group=pico_status_led
#ifndef PICO_DEFAULT_COLORED_STATUS_LED_ON_COLOR
#if PICO_COLORED_STATUS_LED_USES_WRGB
#define PICO_DEFAULT_COLORED_STATUS_LED_ON_COLOR PICO_COLORED_STATUS_LED_COLOR_FROM_WRGB(0xaa, 0, 0, 0)
#else
#define PICO_DEFAULT_COLORED_STATUS_LED_ON_COLOR PICO_COLORED_STATUS_LED_COLOR_FROM_RGB(0xaa, 0xaa, 0xaa)
#endif
#endif

/*! \brief Initialize the status LED(s)
 * \ingroup pico_status_led
 *
 * Initialize the status LED(s) and the resources they need before use. On some devices (e.g. Pico W, Pico 2 W) accessing
 * the status LED requires talking to the WiFi chip, which requires an \ref async_context.
 * This method will create an async_context for you.
 *
 * However an application should only use a single \ref async_context instance to talk to the WiFi chip.
 * If the application already has an async context (e.g. created by cyw43_arch_init) you should use \ref
 * status_led_init_with_context instead and pass it the \ref async_context already created by your application
 *
 * \note You must call this function (or \ref status_led_init_with_context) before using any other pico_status_led functions.
 *
 * \return Returns true if the LED was initialized successfully, otherwise false on failure
 * \sa status_led_init_with_context
 */
bool status_led_init(void);

/*! \brief Initialise the status LED(s)
 *  \ingroup pico_status_led
 *
 * Initialize the status LED(s) and the resources they need before use.
 *
 * \note You must call this function (or \ref status_led_init) before using any other pico_status_led functions.
 *
 * \param context An \ref async_context used to communicate with the status LED (e.g. on Pico W or Pico 2 W)
 * \return Returns true if the LED was initialized successfully, otherwise false on failure
 * \sa status_led_init_with_context
 */
bool status_led_init_with_context(struct async_context *context);

/*! \brief Determine if the `colored_status_led_` APIs are supported (i.e. if there is a colored status LED, and its
 *         use isn't disabled via \ref PICO_COLORED_STATUS_LED_AVAILABLE being set to 0
 *  \ingroup pico_status_led
 * \return true if the colored status LED API is available and expected to produce visible results
 * \sa PICO_COLORED_STATUS_LED_AVAILABLE
 */
static inline bool colored_status_led_supported(void) {
    return PICO_COLORED_STATUS_LED_AVAILABLE;
}

/*! \brief Determine if the colored status LED is being used for the single-color `status_led_` APIs
 *  \ingroup pico_status_led
 * \return true if the colored status LED is being used for the single-color `status_led_` API
 * \sa PICO_STATUS_LED_VIA_COLORED_STATUS_LED
 */
static inline bool status_led_via_colored_status_led(void) {
    return PICO_STATUS_LED_VIA_COLORED_STATUS_LED;
}

/*! \brief Determine if the single-color `status_led_` APIs are supported (i.e. if there is a regular LED, and its
 *         use isn't disabled via \ref PICO_STATUS_LED_AVAILABLE being set to 0, or if the colored status LED is being used for
 *         the single-color `status_led_` APIs
 *  \ingroup pico_status_led
 * \return true if the single-color status LED API is available and expected to produce visible results
 * \sa PICO_STATUS_LED_AVAILABLE
 * \sa PICO_STATUS_LED_VIA_COLORED_STATUS_LED
 */
static inline bool status_led_supported(void) {
    if (status_led_via_colored_status_led()) {
        return colored_status_led_supported();
    }
    return PICO_STATUS_LED_AVAILABLE;
}

/*! \brief Set the colored status LED on or off
 *  \ingroup pico_status_led
 *
 * \note If your hardware does not support a colored status LED (PICO_DEFAULT_WS2812_PIN), this function does nothing and returns false.
 *
 * \param led_on true to turn the colored LED on. Pass false to turn the colored LED off
 * \return true if the colored status LED could be set, otherwise false
 */
bool colored_status_led_set_state(bool led_on);

/*! \brief Get the state of the colored status LED
 *  \ingroup pico_status_led
 *
 * \note If your hardware does not support a colored status LED (PICO_DEFAULT_WS2812_PIN), this function returns false.
 *
 * \return true if the colored status LED is on, or false if the colored status LED is off
 */
bool colored_status_led_get_state(void);
    
/*! \brief Ensure the colored status LED is on, with the specified color
 *  \ingroup pico_status_led
 *
 * \note If your hardware does not support a colored status LED (PICO_DEFAULT_WS2812_PIN), this function does nothing and returns false.
 *
 * \param color The color to use for the colored status LED when it is on, in 0xWWRRGGBB format
 * \return true if the colored status LED could be set, otherwise false on failure
 */
bool colored_status_led_set_on_with_color(uint32_t color);

/*! \brief Get the color used for the status LED value when it is on
 *  \ingroup pico_status_led
 *
 * \note If your hardware does not support a colored status LED (PICO_DEFAULT_WS2812_PIN), this function always returns 0x0.
 *
* \return The color used for the colored status LED when it is on, in 0xWWRRGGBB format
*/
uint32_t colored_status_led_get_on_color(void);

/*! \brief Set the status LED on or off
*  \ingroup pico_status_led
*
* \note If your hardware does not support a status LED, this function does nothing and returns false.
*
* \param led_on true to turn the LED on. Pass false to turn the LED off
* \return true if the status LED could be set, otherwise false
*/
static inline bool status_led_set_state(bool led_on) {
    if (status_led_via_colored_status_led()) {
        return colored_status_led_set_state(led_on);
    } else if (status_led_supported()) {
#if defined(PICO_DEFAULT_LED_PIN)
    #if PICO_DEFAULT_LED_PIN_INVERTED
        gpio_put(PICO_DEFAULT_LED_PIN, !led_on);
    #else
        gpio_put(PICO_DEFAULT_LED_PIN, led_on);
    #endif
        return true;
#elif defined(CYW43_WL_GPIO_LED_PIN)
        cyw43_gpio_set(&cyw43_state, CYW43_WL_GPIO_LED_PIN, led_on);
        return true;
#endif
    }
    return false;
}

/*! \brief Get the state of the status LED
 *  \ingroup pico_status_led
 *
 * \note If your hardware does not support a status LED, this function always returns false.
 *
 * \return true if the status LED is on, or false if the status LED is off
 */
static inline bool status_led_get_state() {
    if (status_led_via_colored_status_led()) {
        return colored_status_led_get_state();
    } else if (status_led_supported()) {
#if defined(PICO_DEFAULT_LED_PIN)
    #if PICO_DEFAULT_LED_PIN_INVERTED
        return !gpio_get(PICO_DEFAULT_LED_PIN);
    #else
        return gpio_get(PICO_DEFAULT_LED_PIN);
    #endif
#elif defined CYW43_WL_GPIO_LED_PIN
        bool value = false;
        cyw43_gpio_get(&cyw43_state, CYW43_WL_GPIO_LED_PIN, &value);
        return value;
#endif
    }
    return false;
}

/*! \brief De-initialize the status LED(s)
 *  \ingroup pico_status_led
 *
 * De-initializes the status LED(s) when they are no longer needed.
 */
void status_led_deinit();

#ifdef __cplusplus
}
#endif

#endif
