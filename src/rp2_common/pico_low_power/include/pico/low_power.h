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
 * \subsection sleep_example Example
 * \addtogroup pico_sleep
 * \include hello_sleep.c
 */

// PICO_CONFIG: PARAM_ASSERTIONS_ENABLED_PICO_LOW_POWER, Enable/disable assertions in the pico_low_power module, type=bool, default=0, group=pico_low_power
#ifndef PARAM_ASSERTIONS_ENABLED_PICO_LOW_POWER
#define PARAM_ASSERTIONS_ENABLED_PICO_LOW_POWER 0
#endif

#include "hardware/clocks.h"

typedef enum {
    DORMANT_CLOCK_SOURCE_XOSC,
    DORMANT_CLOCK_SOURCE_ROSC,
#if !PICO_RP2040
    DORMANT_CLOCK_SOURCE_LPOSC,
#endif
} dormant_clock_source_t;


// sleep until the given timer reaches the specified value; if the time passes then no sleep occurs
// keep_enabled defaults to none if NULL
// ** LIAM BLESSED **
int low_power_sleep_until_timer(timer_hw_t *timer, absolute_time_t until, const clock_dest_set_t *keep_enabled);

static inline int low_power_deep_sleep_until_default_timer(absolute_time_t until, const clock_dest_set_t *keep_enabled) {
    return low_power_sleep_until_timer(PICO_DEFAULT_TIMER_INSTANCE(), until, keep_enabled);
}

// ** LIAM BLESSED this for RP2040; why not RP2350 **
void low_power_sleep_until_pin_state(uint gpio_pin, bool edge, bool high);

// ** LIAM SAYS THIS IS NO MORE USEFUL THAN SLEEP_UNTIL TIMER... **
int low_power_sleep_until_aon_timer(absolute_time_t until, const clock_dest_set_t *keep_enabled);

// ** LIAM BLESSED but we need to impl it correctly (note not blessed for RP2040, but we should do it anyway for orthogonality **
int low_power_dormant_until_aon_timer(absolute_time_t until,
                                      dormant_clock_source_t dormant_clock_source,
                                      uint src_hz, uint gpio_pin,
                                      const clock_dest_set_t *keep_enabled);

// ** LIAM BLESSED but we need to impl it correctly (note not blessed for RP2040, but we should do it anyway for orthogonality **
void low_power_dormant_until_pin_state(uint gpio_pin, bool edge, bool high,
                                       dormant_clock_source_t dormant_clock_source,
                                       const clock_dest_set_t *keep_enabled);

#if !PICO_RP2040
void low_power_pstate_until_aon_timer(void);
void low_power_pstate_until_pin_state(void);
#endif

void low_power_setup_clocks_for_dormant(dormant_clock_source_t dormant_source);
void low_power_wake_from_dormant(void);

#if 0
void sleep_run_from_dormant_source(dormant_clock_source_t dormant_source);
/*! \brief Send system to sleep until a leading high edge is detected on GPIO
 *  \ingroup pico_sleep
 *
 * One of the sleep_run_* functions must be called prior to this call
 *
 * \param gpio_pin The pin to provide the wake up
 */
static inline void sleep_goto_dormant_until_edge_high(uint gpio_pin) {
    sleep_goto_dormant_until_pin(gpio_pin, true, true);
}

/*! \brief Send system to sleep until a high level is detected on GPIO
 *  \ingroup pico_sleep
 *
 * One of the sleep_run_* functions must be called prior to this call
 *
 * \param gpio_pin The pin to provide the wake up
 */
static inline void sleep_goto_dormant_until_level_high(uint gpio_pin) {
    sleep_goto_dormant_until_pin(gpio_pin, false, true);
}

#endif

/*! \brief Reconfigure clocks to wake up properly from sleep/dormant mode
 *  \ingroup pico_sleep
 *
 * This must be called immediately after continuing execution when waking up from sleep/dormant mode
 *
 */
void sleep_power_up(void);

#ifdef __cplusplus
}
#endif

#endif