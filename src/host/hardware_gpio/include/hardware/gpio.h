/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _HARDWARE_GPIO_H
#define _HARDWARE_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "pico.h"
#include "hardware/irq.h"

typedef enum gpio_function {
    GPIO_FUNC_XIP = 0,
    GPIO_FUNC_XIP_CS1 = 0,
    GPIO_FUNC_SPI = 1,
    GPIO_FUNC_UART = 2,
    GPIO_FUNC_I2C = 3,
    GPIO_FUNC_PWM = 4,
    GPIO_FUNC_SIO = 5,
    GPIO_FUNC_PIO0 = 6,
    GPIO_FUNC_PIO1 = 7,
    GPIO_FUNC_GPCK = 8,
    GPIO_FUNC_USB = 9,
    GPIO_FUNC_HSTX = 10,
    GPIO_FUNC_PIO2 = 11,
    GPIO_FUNC_CORESIGHT_TRACE = 12,
    GPIO_FUNC_UART_AUX = 13,
    GPIO_FUNC_NULL = 0x1f,
}gpio_function_t;

enum gpio_dir {
    GPIO_OUT = 1u, ///< set GPIO to output
    GPIO_IN = 0u,  ///< set GPIO to input
};

enum gpio_irq_level {
    GPIO_IRQ_LEVEL_LOW = 0x1u,  ///< IRQ when the GPIO pin is a logical 0
    GPIO_IRQ_LEVEL_HIGH = 0x2u, ///< IRQ when the GPIO pin is a logical 1
    GPIO_IRQ_EDGE_FALL = 0x4u,  ///< IRQ when the GPIO has transitioned from a logical 1 to a logical 0
    GPIO_IRQ_EDGE_RISE = 0x8u,  ///< IRQ when the GPIO has transitioned from a logical 0 to a logical 1
};

typedef void (*gpio_irq_callback_t)(uint gpio, uint32_t event_mask);

enum gpio_override {
    GPIO_OVERRIDE_NORMAL = 0,      ///< peripheral signal selected via \ref gpio_set_function
    GPIO_OVERRIDE_INVERT = 1,      ///< invert peripheral signal selected via \ref gpio_set_function
    GPIO_OVERRIDE_LOW = 2,         ///< drive low/disable output
    GPIO_OVERRIDE_HIGH = 3,        ///< drive high/enable output
};

enum gpio_slew_rate {
    GPIO_SLEW_RATE_SLOW = 0,  ///< Slew rate limiting enabled
    GPIO_SLEW_RATE_FAST = 1   ///< Slew rate limiting disabled
};

enum gpio_drive_strength {
    GPIO_DRIVE_STRENGTH_2MA = 0, ///< 2 mA nominal drive strength
    GPIO_DRIVE_STRENGTH_4MA = 1, ///< 4 mA nominal drive strength
    GPIO_DRIVE_STRENGTH_8MA = 2, ///< 8 mA nominal drive strength
    GPIO_DRIVE_STRENGTH_12MA = 3 ///< 12 mA nominal drive strength
};

void check_gpio_param(uint gpio);

// ----------------------------------------------------------------------------
// Pad Controls + IO Muxing
// ----------------------------------------------------------------------------
// Declarations for gpio.c

void gpio_set_function(uint gpio, gpio_function_t fn);
void gpio_set_function_masked(uint32_t gpio_mask, gpio_function_t fn);
void gpio_set_function_masked64(uint64_t gpio_mask, gpio_function_t fn);

gpio_function_t gpio_get_function(uint gpio);

void gpio_set_pulls(uint gpio, bool up, bool down);

void gpio_pull_up(uint gpio);

bool gpio_is_pulled_up(uint gpio);

void gpio_pull_down(uint gpio);

bool gpio_is_pulled_down(uint gpio);

void gpio_disable_pulls(uint gpio);

void gpio_set_irqover(uint gpio, uint value);

void gpio_set_outover(uint gpio, uint value);

void gpio_set_inover(uint gpio, uint value);

void gpio_set_oeover(uint gpio, uint value);

void gpio_set_input_enabled(uint gpio, bool enabled);

void gpio_set_input_hysteresis_enabled(uint gpio, bool enabled);

bool gpio_is_input_hysteresis_enabled(uint gpio);

void gpio_set_slew_rate(uint gpio, enum gpio_slew_rate slew);

enum gpio_slew_rate gpio_get_slew_rate(uint gpio);

void gpio_set_drive_strength(uint gpio, enum gpio_drive_strength drive);

enum gpio_drive_strength gpio_get_drive_strength(uint gpio);

void gpio_set_irq_enabled(uint gpio, uint32_t event_mask, bool enabled);

void gpio_set_irq_callback(gpio_irq_callback_t callback);

void gpio_set_irq_enabled_with_callback(uint gpio, uint32_t event_mask, bool enabled, gpio_irq_callback_t callback);

void gpio_set_dormant_irq_enabled(uint gpio, uint32_t event_mask, bool enabled);

uint32_t gpio_get_irq_event_mask(uint gpio);

void gpio_acknowledge_irq(uint gpio, uint32_t event_mask);

void gpio_add_raw_irq_handler_with_order_priority_masked(uint32_t gpio_mask, irq_handler_t handler, uint8_t order_priority);

void gpio_add_raw_irq_handler_with_order_priority_masked64(uint64_t gpio_mask, irq_handler_t handler, uint8_t order_priority);

void gpio_add_raw_irq_handler_with_order_priority(uint gpio, irq_handler_t handler, uint8_t order_priority);

void gpio_add_raw_irq_handler_masked(uint32_t gpio_mask, irq_handler_t handler);

void gpio_add_raw_irq_handler_masked64(uint64_t gpio_mask, irq_handler_t handler);

void gpio_add_raw_irq_handler(uint gpio, irq_handler_t handler);

void gpio_remove_raw_irq_handler_masked(uint32_t gpio_mask, irq_handler_t handler);

void gpio_remove_raw_irq_handler_masked64(uint64_t gpio_mask, irq_handler_t handler);

void gpio_remove_raw_irq_handler(uint gpio, irq_handler_t handler);

// Configure a GPIO for direct input/output from software
void gpio_init(uint gpio);

void gpio_deinit(uint gpio);

void gpio_init_mask(uint32_t gpio_mask);
void gpio_init_mask64(uint64_t gpio_mask);

// ----------------------------------------------------------------------------
// Input
// ----------------------------------------------------------------------------

// Get the value of a single GPIO
bool gpio_get(uint gpio);

// Get raw value of all
uint32_t gpio_get_all(void);

uint64_t gpio_get_all64(void);

// ----------------------------------------------------------------------------
// Output
// ----------------------------------------------------------------------------

// Drive high every GPIO appearing in mask
void gpio_set_mask(uint32_t mask);
void gpio_set_mask64(uint64_t mask);
void gpio_set_mask_n(uint n, uint32_t mask);

void gpio_clr_mask(uint32_t mask);
void gpio_clr_mask64(uint64_t mask);
void gpio_clr_mask_n(uint n, uint32_t mask);

// Toggle every GPIO appearing in mask
void gpio_xor_mask(uint32_t mask);
void gpio_xor_mask64(uint64_t mask);
void gpio_xor_mask_n(uint n, uint32_t mask);


// For each 1 bit in "mask", drive that pin to the value given by
// corresponding bit in "value", leaving other pins unchanged.
// Since this uses the TOGL alias, it is concurrency-safe with e.g. an IRQ
// bashing different pins from the same core.
void gpio_put_masked(uint32_t mask, uint32_t value);
void gpio_put_masked64(uint64_t mask, uint64_t value);
void gpio_put_masked_n(uint n, uint32_t mask, uint32_t value);

// Drive all pins simultaneously
void gpio_put_all(uint32_t value);
void gpio_put_all64(uint64_t value);


// Drive a single GPIO high/low
void gpio_put(uint gpio, int value);

// Determine whether a GPIO is currently driven high or low
bool gpio_get_out_level(uint gpio);

// ----------------------------------------------------------------------------
// Direction
// ----------------------------------------------------------------------------

// Switch all GPIOs in "mask" to output
void gpio_set_dir_out_masked(uint32_t mask);
void gpio_set_dir_out_masked64(uint64_t mask);

// Switch all GPIOs in "mask" to input
void gpio_set_dir_in_masked(uint32_t mask);
void gpio_set_dir_in_masked64(uint64_t mask);

// For each 1 bit in "mask", switch that pin to the direction given by
// corresponding bit in "value", leaving other pins unchanged.
// E.g. gpio_set_dir_masked(0x3, 0x2); -> set pin 0 to input, pin 1 to output,
// simultaneously.
void gpio_set_dir_masked(uint32_t mask, uint32_t value);
void gpio_set_dir_masked64(uint64_t mask, uint64_t value);

// Set direction of all pins simultaneously.
// For each bit in value,
// 1 = out
// 0 = in
void gpio_set_dir_all_bits(uint32_t value);
void gpio_set_dir_all_bits64(uint64_t values);

// Set a single GPIO to input/output.
// true = out
// 0 = in
void gpio_set_dir(uint gpio, bool out);

// Check if a specific GPIO direction is OUT
bool gpio_is_dir_out(uint gpio);

// Get a specific GPIO direction
// 1 = out
// 0 = in
uint gpio_get_dir(uint gpio);

#if PICO_SECURE
void gpio_assign_to_ns(uint gpio, bool ns);
#endif
extern void gpio_debug_pins_init(void);

#ifdef __cplusplus
}
#endif


// PICO_CONFIG: PICO_DEBUG_PIN_BASE, First pin to use for debug output (if enabled), min=0, max=31 on RP2350B, 29 otherwise, default=19, group=hardware_gpio
#ifndef PICO_DEBUG_PIN_BASE
#define PICO_DEBUG_PIN_BASE 19u
#endif

// PICO_CONFIG: PICO_DEBUG_PIN_COUNT, Number of pins to use for debug output (if enabled), min=1, max=32 on RP2350B, 30 otherwise, default=3, group=hardware_gpio
#ifndef PICO_DEBUG_PIN_COUNT
#define PICO_DEBUG_PIN_COUNT 3u
#endif

#ifndef __cplusplus
// note these two macros may only be used once per and only apply per compilation unit (hence the CU_)
#define CU_REGISTER_DEBUG_PINS(...) enum __unused DEBUG_PIN_TYPE { _none = 0, __VA_ARGS__ }; static enum DEBUG_PIN_TYPE __selected_debug_pins;
#define CU_SELECT_DEBUG_PINS(x) static enum DEBUG_PIN_TYPE __selected_debug_pins = (x);
#define DEBUG_PINS_ENABLED(p) (__selected_debug_pins == (p))
#else
#define CU_REGISTER_DEBUG_PINS(p...) \
    enum DEBUG_PIN_TYPE { _none = 0, p }; \
    template <enum DEBUG_PIN_TYPE> class __debug_pin_settings { \
        public: \
            static inline bool enabled() { return false; } \
    };
#define CU_SELECT_DEBUG_PINS(x) template<> inline bool __debug_pin_settings<x>::enabled() { return true; };
#define DEBUG_PINS_ENABLED(p) (__debug_pin_settings<p>::enabled())
#endif
#define DEBUG_PINS_SET(p, v) if (DEBUG_PINS_ENABLED(p)) gpio_set_mask((unsigned)(v)<<PICO_DEBUG_PIN_BASE)
#define DEBUG_PINS_CLR(p, v) if (DEBUG_PINS_ENABLED(p)) gpio_clr_mask((unsigned)(v)<<PICO_DEBUG_PIN_BASE)
#define DEBUG_PINS_XOR(p, v) if (DEBUG_PINS_ENABLED(p)) gpio_xor_mask((unsigned)(v)<<PICO_DEBUG_PIN_BASE)

#endif
