/*
 * Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_AON_TIMER_H
#define _PICO_AON_TIMER_H

#include "pico.h"
#include <time.h>
#include "pico/util/datetime.h"
#include "hardware/regs/intctrl.h"

/** \file pico/aon_timer.h
 *  \defgroup pico_aon_timer pico_aon_timer
 *
 * \brief High Level "Always on Timer" Abstraction
 *
 * \if rp2040_specific
 * This library uses the RTC on RP2040.
 * \endif
 * \if rp2350_specific
 * This library uses the Powman Timer on RP2350.
 * \endif
 *
 * This library supports both `aon_timer_xxx_calendar()` methods which take a calendar date/time (as struct tm),
 * and `aon_timer_xxx()` methods which take a time since epoch (as struct timespec)
 *
 * \if rp2040_specific
 * NOTE: On RP2040 the non date/time methods must convert the struct timespec to a date/time value which is handled via
 * the \ref pico_localtime_r method. By default, this pulls in the C library local_time_r method which can lead to a big increase in binary size.
 * `pico_localtime_r` is weak, so can be overridden if a better/smaller alternative is available, otherwise you might consider
 * using the `aon_timer_xxx_calendar()` variants on RP2040
 * \endif
 *
 * \if rp2350_specific
 * NOTE: On RP2350 the date/time methods must convert the struct tm to a struct timespec value which is handled via
 * the \ref pico_mktime method. By default, this pulls in the C library mktime method which can lead to a big increase in binary size.
 * `pico_mktime` is weak, so can be overridden if a better/smaller alternative is available, otherwise you might consider
 * using the `aon_timer_xxx()` variants on RP2350
 * \endif
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \def AON_TIMER_IRQ_NUM()
 * \ingroup pico_aon_timer
 * \hideinitializer
 * \brief Returns the \ref irq_num_t for interrupts for the actual hardware backing the AON timer abstraction
 *
 * Note this macro is intended to resolve at compile time, and does no parameter checking
 */
#ifndef AON_TIMER_IRQ_NUM
#if HAS_RP2040_RTC
#define AON_TIMER_IRQ_NUM() RTC_IRQ
#elif HAS_POWMAN_TIMER
#define AON_TIMER_IRQ_NUM() POWMAN_IRQ_TIMER
#endif
#endif

typedef void (*aon_timer_alarm_handler_t)(void);

/**
 * \brief Start the AON timer running using the result from the gettimeofday() function as the current time
 * \ingroup pico_aon_timer
 */
void aon_timer_start_with_timeofday(void);

/**
 * \brief Start the AON timer running using the specified timespec as the current time
 * \ingroup pico_aon_timer
 * \param ts the current time
 */
void aon_timer_start(const struct timespec *ts);
void aon_timer_start_calendar(const struct tm *tm);

/**
 * \brief Stop the AON timer
 * \ingroup pico_aon_timer
 */
void aon_timer_stop(void);

/**
 * \brief Update the current time of the AON timer
 * \ingroup pico_aon_timer
 * \param ts the new current time
 */
bool aon_timer_set_time(const struct timespec *ts);
bool aon_timer_set_time_calendar(const struct tm *tm);

/**
 * \brief Get the current time of the AON timer
 * \ingroup pico_aon_timer
 * \param ts out value for the current time
 */
bool aon_timer_get_time(struct timespec *ts);
bool aon_timer_get_time_calendar(struct tm *tm);

/**
 * \brief Get the resolution of the AON timer
 * \ingroup pico_aon_timer
 * \param ts out value for the resolution of the AON timer
 */
void aon_timer_get_resolution(struct timespec *ts);

/**
 * \brief Enable an AON timer alarm for a specified time
 * \ingroup pico_aon_timer
 *
 * \if rp2350_specific
 * On RP2350 the alarm will fire if it is in the past
 * \endif
 * \if rp2040_specific
 * On RP2040 the alarm will not fire if it is in the past.
 * \endif
 *
 * \param ts the alarm time
 * \param handler a callback to call when the timer fires (can be NULL for wakeup_from_low_power = true)
 * \param wakeup_from_low_power true if the AON timer is to be used to wake up from a DORMANT state
 * \return on success the old handler (or NULL if there was none)
 *         on failure, PICO_ERROR_INVALID_ARG
 * \sa pico_localtime_r
 */
aon_timer_alarm_handler_t aon_timer_enable_alarm(const struct timespec *ts, aon_timer_alarm_handler_t handler, bool wakeup_from_low_power);
aon_timer_alarm_handler_t aon_timer_enable_alarm_calendar(const struct tm *tm, aon_timer_alarm_handler_t handler, bool wakeup_from_low_power);

/**
 * \brief Disable the currently enabled AON timer alarm if any
 * \ingroup pico_aon_timer
 */
void aon_timer_disable_alarm(void);

/**
 * \brief Disable the currently enabled AON timer alarm if any
 * \ingroup pico_aon_timer
 * \return true if the AON timer is running
 */
bool aon_timer_is_running(void);

static inline uint aon_timer_get_irq_num(void) {
    return AON_TIMER_IRQ_NUM();
}

#ifdef __cplusplus
}
#endif

#endif
