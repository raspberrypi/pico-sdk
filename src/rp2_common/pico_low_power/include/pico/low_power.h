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
 * In Pstate mode some power domains are switched off and don't retain state.
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
#if HAS_POWMAN_TIMER
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

#if HAS_POWMAN_TIMER
typedef void (*low_power_pstate_resume_func)(pstate_bitset_t *pstate);
#endif


// NOTE: Need to deinit usb before doing into any of these sleep states
// could keep usb clk_sys and clk_usb to usbctrl running during low_power_sleep. Although you'll get woken
// up pretty quickly
// Also you are plugged into a host so why bother?

// sleep is really just calling a __wfi() until an irq, with some optional clock gating in the sleep_en register. Activated once processor goes to sleep
// So can just do it for arbitrary interrupts. processors implement their own internal clock gating, just leaving the wakeup interrupt controller running during
// __wfi()

/*! \brief  Sleep until an interrupt occurs
 *  \ingroup pico_low_power
 * Sleep until any interrupt occurs. The clocks specified in keep_enabled will be kept enabled during sleep.
 *
 * \param keep_enabled The clocks to keep enabled during sleep.
 * \return 0 on success, non-zero on error.
 */
int low_power_sleep_until_irq(const clock_dest_set_t *keep_enabled);

// sleep until the given timer reaches the specified value; if the time passes then no sleep occurs
// keep_enabled defaults to none if NULL
// ** LIAM BLESSED **

/*! \brief  Sleep until time using timer
 *  \ingroup pico_low_power
 * Sleep until the given timer reaches the specified value. The clocks specified in keep_enabled will be kept enabled during sleep, along with clocks required
 * for the timer. If exclusive is true, only the timer interrupt will be listened for, otherwise other interrupts will be listened for.
 *
 * \param timer The timer to use.
 * \param until The time to sleep until.
 * \param keep_enabled The clocks to keep enabled during sleep.
 * \param exclusive Whether to only listen for the timer interrupt, or other interrupts.
 * \return 0 on success, non-zero on error.
 */
int low_power_sleep_until_timer(timer_hw_t *timer, absolute_time_t until, const clock_dest_set_t *keep_enabled, bool exclusive);
// Note bool above saying shall we only listen for timer irq or other irqs
// Need to defer handling of irqs to do clock setup etc

/*! \brief  Sleep until time using default timer
 *  \ingroup pico_low_power
 * See \ref low_power_sleep_until_timer for more information.
 *
 * \param until The time to sleep until.
 * \param keep_enabled The clocks to keep enabled during sleep.
 * \param exclusive Whether to only listen for the timer interrupt, or other interrupts.
 * \return 0 on success, non-zero on error.
 */
static inline int low_power_sleep_until_default_timer(absolute_time_t until, const clock_dest_set_t *keep_enabled, bool exclusive) {
    // Need to assert (or add) ticks block and timer clocks to the keep_enabled list
    return low_power_sleep_until_timer(PICO_DEFAULT_TIMER_INSTANCE(), until, keep_enabled, exclusive);
}


/*! \brief  Sleep until pin state changes
 *  \ingroup pico_low_power
 * Sleep until the given GPIO pin changes state. The clocks specified in keep_enabled will be kept enabled during sleep.
 * If exclusive is true, only the GPIO interrupt will be listened for, otherwise other interrupts will be listened for.
 *
 * \param gpio_pin The GPIO pin to use.
 * \param edge Whether to listen for edge or level.
 * \param high Whether to listen for the high/low level, or rising/falling edge.
 * \param keep_enabled The clocks to keep enabled during sleep.
 * \param exclusive Whether to only listen for the GPIO interrupt, or other interrupts.
 * \return 0 on success, non-zero on error.
 */
void low_power_sleep_until_pin_state(uint gpio_pin, bool edge, bool high, const clock_dest_set_t *keep_enabled, bool exclusive);


// ** LIAM BLESSED but we need to impl it correctly (note not blessed for RP2040, but we should do it anyway for orthogonality **
// Only works for RP2350 as every clock will be stopped on RP2040 (unless you provide a clock for the RTC)
// Easier to not support on RP2040 - might as well buy a 2350

// NOTE: Asserting that we will alway use rosc for dormant and simplifies the API
// Means if the user has sped up the rosc they should slow it down before going into dormant
// Need to re initialize clocks after this

/*! \brief  Go dormant until time using AON timer
 *  \ingroup pico_low_power
 * Go dormant until the given AON timer reaches the specified value.
 * The clocks specified in keep_enabled will be kept enabled during dormant, but XOSC and ROSC will be stopped.
 *
 * \param until The time to go dormant until.
 * \param dormant_clock_source The clock source to use for dormant. Must be DORMANT_CLOCK_SOURCE_LPOSC on RP2350.
 * \param src_hz The frequency of the clock source on RP2040. Ignored on RP2350.
 * \param gpio_pin The GPIO pin to use for the RTC on RP2040. Ignored on RP2350.
 * \param keep_enabled The clocks to keep enabled during dormant.
 * \return 0 on success, non-zero on error.
 */
int low_power_dormant_until_aon_timer(absolute_time_t until, dormant_clock_source_t dormant_clock_source, uint src_hz, uint gpio_pin, const clock_dest_set_t *keep_enabled);

// ** LIAM BLESSED but we need to impl it correctly (note not blessed for RP2040, but we should do it anyway for orthogonality **
// This works on both
// Need to re initialize clocks after this

/*! \brief  Go dormant until pin state changes
 *  \ingroup pico_low_power
 * Go dormant until the given GPIO pin changes state.
 * The clocks specified in keep_enabled will be kept enabled during dormant, but XOSC and ROSC will be stopped.
 *
 * \param gpio_pin The GPIO pin to use.
 * \param edge Whether to listen for edge or level.
 * \param high Whether to listen for the high/low level, or rising/falling edge.
 * \param dormant_clock_source The clock source to use for dormant.
 * \param keep_enabled The clocks to keep enabled during dormant.
 */
void low_power_dormant_until_pin_state(uint gpio_pin, bool edge, bool high, dormant_clock_source_t dormant_clock_source, const clock_dest_set_t *keep_enabled);

#if HAS_POWMAN_TIMER
// pstate functions should return to the pstate you were in

// pass resume_func which will be called on reboot by runtime_init_low_power_reboot_check

/*! \brief  Go to Pstate until time using AON timer
 *  \ingroup pico_low_power
 * Go to Pstate until the given AON timer reaches the specified value. The function specified in resume_func will be called on reboot.
 *
 * If pstate is NULL, it will go to the minimum Pstate that will keep persistent data powered on.
 *
 * NOTE: This function will overwrite the last 2 powman scratch registers - the other scratch registers are not modified.
 *
 * To also wake up from a GPIO, configure that using \ref powman_enable_gpio_wakeup before calling this function.
 *
 * \param until The time to go to Pstate until.
 * \param pstate The Pstate to use. If NULL, the Pstate will keep persistent data powered on.
 * \param resume_func The function to call on reboot.
 * \return 0 on success, non-zero on error.
 */
int low_power_pstate_until_aon_timer(absolute_time_t until, pstate_bitset_t *pstate, low_power_pstate_resume_func resume_func);

/*! \brief  Go to Pstate until pin state changes
 *  \ingroup pico_low_power
 * Go to Pstate until the given GPIO pin changes state. The function specified in resume_func will be called on reboot.
 *
 * If pstate is NULL, it will go to the minimum Pstate that will keep persistent data powered on.
 *
 * NOTE: This function will overwrite the last 2 powman scratch registers - the other scratch registers are not modified.
 *
 * \param gpio_pin The GPIO pin to use.
 * \param edge Whether to listen for edge or level.
 * \param high Whether to listen for the high/low level, or rising/falling edge.
 * \param pstate The Pstate to use. If NULL, the Pstate will keep persistent data powered on.
 * \param resume_func The function to call on reboot.
 * \return 0 on success, non-zero on error.
 */
int low_power_pstate_until_pin_state(uint gpio_pin, bool edge, bool high, pstate_bitset_t *pstate, low_power_pstate_resume_func resume_func);

// Or a function saying how did I boot?
// Would like to make it easy to get back to main after going to sleep

// Two configs:
// - Everything off apart from AON, ram needs zeroing on boot
// - Switched core off (args are which rams you want to keep on)
// Switched core, XIP cache + bootram, SRAM0 bank, SRAM1 bank + SCRATCH

/*! \brief  Get Pstate which keeps persistent data powered on
 *  \ingroup pico_low_power
 *
 * \param pstate Pointer to the Pstate to write the result to.
 * \return The Pstate.
 */
pstate_bitset_t *low_power_persistent_pstate_get(pstate_bitset_t *pstate);
#endif

#ifdef __cplusplus
}
#endif

#endif
