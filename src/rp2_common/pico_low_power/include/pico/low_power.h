/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_LOW_POWER_H_
#define _PICO_LOW_POWER_H_

#include "pico.h"
#include "hardware/timer.h"

#ifdef __cplusplus
extern "C" {
#endif

/** \file low_power.h
 *  \defgroup pico_low_power pico_low_power
 *
 * Lower Power Sleep APIs
 *
 * The difference between sleep and dormant is that ALL clocks are stopped in dormant mode,
 * until the source (either xosc or rosc) is started again by an external event.
 *
 * In sleep mode some clocks can be left running controlled by the SLEEP_EN registers in the clocks
 * block. For example you could keep clk_rtc running. Some destinations (proc0 and proc1 wakeup logic)
 * can't be stopped in sleep mode otherwise there wouldn't be enough logic to wake up again.
 *
 * In pstate mode some power domains are switched off and don't retain state.
 * 
 * \subsection sleep_example Example
 * \addtogroup pico_sleep
 * \include hello_sleep.c
 */

// PICO_CONFIG: PARAM_ASSERTIONS_ENABLED_PICO_LOW_POWER, Enable/disable assertions in the pico_low_power module, type=bool, default=0, group=pico_low_power
#ifndef PARAM_ASSERTIONS_ENABLED_PICO_LOW_POWER
#define PARAM_ASSERTIONS_ENABLED_PICO_LOW_POWER 0
#endif

#include "hardware/clocks.h"
#if !PICO_RP2040
#include "hardware/powman.h"
#endif

typedef enum {
    DORMANT_CLOCK_SOURCE_XOSC,
    DORMANT_CLOCK_SOURCE_ROSC,
#if !PICO_RP2040
    DORMANT_CLOCK_SOURCE_LPOSC,
#endif
    NUM_DORMANT_CLOCK_SOURCES
} dormant_clock_source_t;

#if !PICO_RP2040
typedef void (*low_power_pstate_resume_func)(pstate_bitset_t *pstate);
#endif


// NOTE: Need to deinit usb before doing into any of these sleep states
// could keep usb clk_sys and clk_usb to usbctrl running during low_power_sleep. Although you'll get woken
// up pretty quickly
// Also you are plugged into a host so why bother?

// sleep is really just calling a __wfi() until an irq, with some optional clock gating in the sleep_en register. Activated once processor goes to sleep
// So can just do it for arbitrary interrupts. processors implement their own internal clock gating, just leaving the wakeup interrupt controller running during
// __wfi()
int low_power_sleep_until_irq(const clock_dest_set_t *keep_enabled);

// sleep until the given timer reaches the specified value; if the time passes then no sleep occurs
// keep_enabled defaults to none if NULL
// ** LIAM BLESSED **
int low_power_sleep_until_timer(timer_hw_t *timer, absolute_time_t until, const clock_dest_set_t *keep_enabled, bool exclusive);
// Note bool above saying shall we only listen for timer irq or other irqs
// Need to defer handling of irqs to do clock setup etc

static inline int low_power_sleep_until_default_timer(absolute_time_t until, const clock_dest_set_t *keep_enabled, bool exclusive) {
    // Need to assert (or add) ticks block and timer clocks to the keep_enabled list
    return low_power_sleep_until_timer(PICO_DEFAULT_TIMER_INSTANCE(), until, keep_enabled, exclusive);
}

// ** LIAM BLESSED this for RP2040; why not RP2350 **
// This should work on both via io bank interrupts
void low_power_sleep_until_pin_state(uint gpio_pin, bool edge, bool high, const clock_dest_set_t *keep_enabled, bool exclusive);

#if 0
// ** LIAM SAYS THIS IS NO MORE USEFUL THAN SLEEP_UNTIL TIMER... **
// There isn't much advantage to using the aon timer here as system timers are more accurate and on anyway
// ** WILL AGREES WITH LIAM ON THIS **
int low_power_sleep_until_aon_timer(absolute_time_t until, const clock_dest_set_t *keep_enabled);
#endif

// ** LIAM BLESSED but we need to impl it correctly (note not blessed for RP2040, but we should do it anyway for orthogonality **
// Only works for RP2350 as every clock will be stopped on RP2040 (unless you provide a clock for the RTC)
// Easier to not support on RP2040 - might as well buy a 2350

// NOTE: Asserting that we will alway use rosc for dormant and simplifies the API
// Means if the user has sped up the rosc they should slow it down before going into dormant
// Need to re initialize clocks after this
int low_power_dormant_until_aon_timer(absolute_time_t until, dormant_clock_source_t dormant_clock_source, uint src_hz, uint gpio_pin, const clock_dest_set_t *keep_enabled);

// ** LIAM BLESSED but we need to impl it correctly (note not blessed for RP2040, but we should do it anyway for orthogonality **
// This works on both
// Need to re initialize clocks after this
void low_power_dormant_until_pin_state(uint gpio_pin, bool edge, bool high, dormant_clock_source_t dormant_clock_source, const clock_dest_set_t *keep_enabled);

#if !PICO_RP2040
// pstate functions should return to the pstate you were in

// pass resume_func which will be called on reboot by runtime_init_low_power_reboot_check
int low_power_pstate_until_aon_timer(absolute_time_t until, pstate_bitset_t *pstate, low_power_pstate_resume_func resume_func);
int low_power_pstate_until_pin_state(uint gpio_pin, bool edge, bool high, pstate_bitset_t *pstate, low_power_pstate_resume_func resume_func);

// Or a function saying how did I boot?
// Would like to make it easy to get back to main after going to sleep

// Two configs:
// - Everything off apart from AON, ram needs zeroing on boot
// - Switched core off (args are which rams you want to keep on)
// Switched core, XIP cache + bootram, SRAM0 bank, SRAM1 bank + SCRATCH

// Go to a pstate
// Doesn't support powering down switched core domain
int low_power_pstate_set(pstate_bitset_t *pstate);
pstate_bitset_t low_power_pstate_get(void);
#endif

#ifdef __cplusplus
}
#endif

#endif