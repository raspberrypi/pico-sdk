/*
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_LOW_POWER_H_
#define _PICO_LOW_POWER_H_

#include "pico.h"
#include "hardware/timer.h"
#include "pico/time.h"
#include "pico/aon_timer.h"

#ifdef __cplusplus
extern "C" {
#endif

/** \file low_power.h
 *  \defgroup pico_low_power pico_low_power
 *
 * \brief Lower Power APIs
 *
 * There are three modes of operation: sleep, dormant, and Pstate, with the lowest power consumption being Pstate.
 *
 * \if rp2040_specific
 * NOTE: On RP2040, there is no Pstate mode.
 * \endif
 *
 * In sleep mode:
 * - Some clocks can be left running controlled by the SLEEP_EN registers in the clocks
 *   block. For example you could keep clk_rtc running.
 * - Some destinations (proc0 and proc1 wakeup logic) can't be stopped in sleep mode otherwise there wouldn't be enough logic to wake up again.
 * - You can wake up from sleep by any interrupt, provided the clock for that interrupt is kept enabled
 * - The sleep APIs will keep the clocks enabled for stdio during sleep, along with USB clocks when using tinyusb.
 *
 * In dormant mode:
 * - Both xosc and rosc are stopped, until the dormant clock source is started again by an external event.
 * - USB will be disabled before going into dormant, and re-enabled after waking up.
 * - You can only wake up from dormant using the AON timer, or a GPIO interrupt configured using \ref gpio_set_dormant_irq_enabled.
 *   All other interrupts will be disabled during dormant.
 *
 * \if (!rp2040_specific)
 * In Pstate mode:
 * - Some power domains are switched off and don't retain state (eg specified SRAMs). By default, only the power domains
 *   required to keep persistent data powered on will be kept on.
 * - You can only wake up from Pstate using the AON timer, or a GPIO wakeup configured using \ref powman_enable_gpio_wakeup.
 * - Waking up from Pstate will run your program from the start, optionally executing a resume_func during runtime init.
 *   All non-persistent data will be overwritten by crt0 when the program starts again.
 * - Variables can be marked as persistent using the __persistent_data macro. The location of the data can be set using the
 *   CMake function pico_set_persistent_data_loc. The persistent data can be stored in XIP_SRAM or main SRAM.
 * - The Pstate APIs will overwrite the last 2 powman scratch registers - the other scratch registers are not modified,
 *   so can be used for other persistent data.
 * \endif
 *
 */

// PICO_CONFIG: PARAM_ASSERTIONS_ENABLED_PICO_LOW_POWER, Enable/disable assertions in the pico_low_power module, type=bool, default=0, group=pico_low_power
#ifndef PARAM_ASSERTIONS_ENABLED_PICO_LOW_POWER
#define PARAM_ASSERTIONS_ENABLED_PICO_LOW_POWER 0
#endif

// PICO_CONFIG: PICO_LOW_POWER_MIN_AON_SLEEP_TIME_MS, Minimum supported time to spend in sleep using the AON timer in milliseconds, type=int, default=2000 on RP2040, 10 otherwise, group=pico_low_power
#ifndef PICO_LOW_POWER_MIN_AON_SLEEP_TIME_MS
#if PICO_RP2040
#define PICO_LOW_POWER_MIN_AON_SLEEP_TIME_MS 2000
#else
#define PICO_LOW_POWER_MIN_AON_SLEEP_TIME_MS 10
#endif
#endif

// PICO_CONFIG: PICO_LOW_POWER_MIN_DORMANT_TIME_MS, Minimum supported time to spend dormant in milliseconds, type=int, default=2000 on RP2040, 10 otherwise, group=pico_low_power
#ifndef PICO_LOW_POWER_MIN_DORMANT_TIME_MS
#if PICO_RP2040
#define PICO_LOW_POWER_MIN_DORMANT_TIME_MS 2000
#else
#define PICO_LOW_POWER_MIN_DORMANT_TIME_MS 10
#endif
#endif

// PICO_CONFIG: PICO_LOW_POWER_MIN_PSTATE_TIME_MS, Minimum supported time to spend in Pstate in milliseconds, type=int, default=10, group=pico_low_power
#ifndef PICO_LOW_POWER_MIN_PSTATE_TIME_MS
#define PICO_LOW_POWER_MIN_PSTATE_TIME_MS 10
#endif

#include "hardware/clocks.h"
#if HAS_RP2040_RTC
#include "hardware/rtc.h"
#endif
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

#if PICO_RP2040
#define DORMANT_CLOCK_SOURCE_DEFAULT DORMANT_CLOCK_SOURCE_XOSC
#else
#define DORMANT_CLOCK_SOURCE_DEFAULT DORMANT_CLOCK_SOURCE_LPOSC
#endif

#if PICO_RP2040
#define DORMANT_CLOCK_HZ_DEFAULT RTC_CLOCK_FREQ_HZ
#else
#define DORMANT_CLOCK_HZ_DEFAULT 0  // ignored
#endif

#if HAS_POWMAN_TIMER
typedef void (*low_power_pstate_resume_func)(pstate_bitset_t *pstate);
#endif


/*! \brief  Sleep until an interrupt occurs
 *  \ingroup pico_low_power
 *
 * Sleep until any interrupt occurs. The clocks specified in keep_enabled will be kept enabled during sleep.
 *
 * \param keep_enabled The clocks to keep enabled during sleep.
 * \return 0 on success, non-zero on error.
 */
int low_power_sleep_until_irq(const clock_dest_bitset_t *keep_enabled);

/*! \brief  Sleep until time using timer
 *  \ingroup pico_low_power
 *
 * Sleep until the given timer reaches the specified value. The clocks specified in keep_enabled will be kept enabled during sleep, along with clocks required
 * for the timer. If exclusive is true, only the timer interrupt will be listened for, otherwise other interrupts will also be listened for.
 *
 * \param timer The timer to use.
 * \param until The time to sleep until.
 * \param keep_enabled The clocks to keep enabled during sleep.
 * \param exclusive Whether to only listen for the timer interrupt, or other interrupts.
 * \return 0 on success, non-zero on error.
 */
int low_power_sleep_until_timer(timer_hw_t *timer, absolute_time_t until, const clock_dest_bitset_t *keep_enabled, bool exclusive);

/*! \brief  Sleep until time using default timer
 *  \ingroup pico_low_power
 *
 * See \ref low_power_sleep_until_timer for more information.
 *
 * \param until The time to sleep until.
 * \param keep_enabled The clocks to keep enabled during sleep.
 * \param exclusive Whether to only listen for the timer interrupt, or other interrupts.
 * \return 0 on success, non-zero on error.
 */
static inline int low_power_sleep_until_default_timer(absolute_time_t until, const clock_dest_bitset_t *keep_enabled, bool exclusive) {
    return low_power_sleep_until_timer(PICO_DEFAULT_TIMER_INSTANCE(), until, keep_enabled, exclusive);
}

/*! \brief  Sleep until time using AON timer
 *  \ingroup pico_low_power
 *
 * Sleep until the AON timer reaches the specified value. The clocks specified in keep_enabled will be kept enabled during sleep, along with clocks required
 * for the AON timer. If exclusive is true, only the AON timer interrupt will be listened for, otherwise other interrupts will also be listened for.
 *
 * \param until The time to sleep until.
 * \param keep_enabled The clocks to keep enabled during sleep.
 * \param exclusive Whether to only listen for the AON timer interrupt, or other interrupts.
 * \return 0 on success, non-zero on error.
 */
int low_power_sleep_until_aon_timer(absolute_time_t until, const clock_dest_bitset_t *keep_enabled, bool exclusive);


/*! \brief  Sleep until GPIO pin state changes
 *  \ingroup pico_low_power
 *
 * Sleep until the given GPIO pin changes state. The clocks specified in keep_enabled will be kept enabled during sleep.
 * If exclusive is true, only the GPIO interrupt will be listened for, otherwise other interrupts will also be listened for.
 *
 * \param gpio_pin The GPIO pin to use.
 * \param edge Whether to listen for edge or level.
 * \param high Whether to listen for high level / rising edge (true), or  low level / falling edge (false).
 * \param keep_enabled The clocks to keep enabled during sleep.
 * \param exclusive Whether to only listen for the GPIO interrupt, or other interrupts.
 * \return 0 on success, non-zero on error.
 */
int low_power_sleep_until_gpio_pin_state(uint gpio_pin, bool edge, bool high, const clock_dest_bitset_t *keep_enabled, bool exclusive);

/*! \brief  Set the external clock source for the AON timer
 *  \ingroup pico_low_power
 *
 * Set the external clock source for the AON timer. This is only used on RP2040.
 *
 * \param src_hz The frequency of the external clock source.
 * \param gpio_pin The GPIO pin to use for the external clock source.
 * \return 0 on success, non-zero on error.
 */
#if PICO_RP2040
int low_power_set_external_clock_source(uint src_hz, uint gpio_pin);
#else
static inline int low_power_set_external_clock_source(__unused uint src_hz, __unused uint gpio_pin) {
    return PICO_OK;
}
#endif

/*! \brief  Go dormant until time using AON timer
 *  \ingroup pico_low_power
 *
 * Go dormant until the given AON timer reaches the specified value.
 * The clocks specified in keep_enabled will be kept enabled during dormant, but XOSC and ROSC will be stopped.
 *
 * \if rp2040_specific
 * This requires an external clock source to be set using \ref low_power_set_external_clock_source before calling this function.
 * If the external clock source is not set, or it is not running, this will return PICO_ERROR_PRECONDITION_NOT_MET.
 * \endif
 *
 * \if (!rp2040_specific)
 * If the clock source is set to DORMANT_CLOCK_SOURCE_LPOSC, clk_sys will be switched to the ROSC while dormant so
 * it can be stopped, while clk_ref will be run from the LPOSC so that it continues running for the timer.
 * \endif
 *
 * \param until The time to go dormant until.
 * \param dormant_clock_source The clock source to use for dormant. Must be DORMANT_CLOCK_SOURCE_LPOSC on RP2350.
 * \param keep_enabled The clocks to keep enabled during dormant.
 * \return 0 on success, non-zero on error.
 */
int low_power_dormant_until_aon_timer(absolute_time_t until, dormant_clock_source_t dormant_clock_source, const clock_dest_bitset_t *keep_enabled);

/*! \brief  Go dormant until GPIO pin state changes
 *  \ingroup pico_low_power
 *
 * Go dormant until the given GPIO pin changes state.
 * The clocks specified in keep_enabled will be kept enabled during dormant, but XOSC and ROSC will be stopped.
 *
 * \if (!rp2040_specific)
 * If the clock source is set to DORMANT_CLOCK_SOURCE_LPOSC, clk_sys will be run from the ROSC while dormant so
 * it can be stopped, while clk_ref will be run from the LPOSC. For the lowest power consumption, you should use
 * DORMANT_CLOCK_SOURCE_ROSC instead, as the GPIO interrupt does not require a clock.
 * \endif
 *
 * \param gpio_pin The GPIO pin to use.
 * \param edge Whether to listen for edge or level.
 * \param high Whether to listen for high level / rising edge (true), or  low level / falling edge (false).
 * \param dormant_clock_source The clock source to use for dormant.
 * \param keep_enabled The clocks to keep enabled during dormant.
 * \return 0 on success, non-zero on error.
 */
int low_power_dormant_until_gpio_pin_state(uint gpio_pin, bool edge, bool high, dormant_clock_source_t dormant_clock_source, const clock_dest_bitset_t *keep_enabled);

#if HAS_POWMAN_TIMER
/*! \brief  Go to Pstate until time using AON timer
 *  \ingroup pico_low_power
 *
 * Go to Pstate until the given AON timer reaches the specified value. The function specified in resume_func will be called on reboot,
 * with the low power Pstate passed to it.
 *
 * If pstate is NULL, it will go to the minimum Pstate that will keep persistent data powered on.
 *
 * To also wake up from a GPIO, configure that using \ref powman_enable_gpio_wakeup before calling this function.
 *
 * NOTE: This function will overwrite the last 2 powman scratch registers - the other scratch registers are not modified.
 *
 * \param until The time to go to Pstate until.
 * \param pstate The Pstate to use. If NULL, the Pstate will keep persistent data powered on.
 * \param resume_func The function to call on reboot.
 * \return 0 on success, non-zero on error.
 */
int low_power_pstate_until_aon_timer(absolute_time_t until, pstate_bitset_t *pstate, low_power_pstate_resume_func resume_func);

/*! \brief  Go to Pstate until GPIO pin state changes
 *  \ingroup pico_low_power
 *
 * Go to Pstate until the given GPIO pin changes state. The function specified in resume_func will be called on reboot,
 * with the low power Pstate passed to it.
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
int low_power_pstate_until_gpio_pin_state(uint gpio_pin, bool edge, bool high, pstate_bitset_t *pstate, low_power_pstate_resume_func resume_func);

/*! \brief  Get Pstate which keeps persistent data powered on
 *  \ingroup pico_low_power
 *
 * \param pstate Pointer to the Pstate to write the result to.
 * \return The Pstate.
 */
pstate_bitset_t *low_power_persistent_pstate_get(pstate_bitset_t *pstate);
#endif


// Convenience functions to avoid needing the correct absolute_time_t

/*! \brief  Start the AON timer at a specific time in milliseconds
 *  \ingroup pico_low_power
 *
 * See \ref aon_timer_start for more information.
 *
 * If the AON timer is already running, this function will restart it
 * from the specified time.
 *
 * \param ms The time in milliseconds to start the AON timer at.
 * \return true on success, false on failure.
 */
static inline bool low_power_start_aon_timer_at_time_ms(uint64_t ms) {
    struct timespec ts;
    ms_to_timespec(ms, &ts);
    return aon_timer_start(&ts);
}

/*! \brief  Start the AON timer at the current system time
 *  \ingroup pico_low_power
 *
 * See \ref aon_timer_start for more information.
 *
 * If the AON timer is already running, this function will not restart it.
 *
 * \return true on success, false on failure.
 */
static inline bool low_power_start_aon_timer(void) {
    if (aon_timer_is_running()) return true;
    return low_power_start_aon_timer_at_time_ms(0);
}

/*! \brief  Sleep for a number of microseconds
 *  \ingroup pico_low_power
 *
 * See \ref low_power_sleep_until_default_timer for more information.
 *
 * \param us The number of microseconds to sleep.
 * \param keep_enabled The clocks to keep enabled during sleep.
 * \param exclusive Whether to only listen for the timer interrupt, or other interrupts.
 * \return 0 on success, non-zero on error.
 */
static inline int low_power_sleep_for_us(timer_hw_t *timer, uint64_t us, const clock_dest_bitset_t *keep_enabled, bool exclusive) {
    return low_power_sleep_until_timer(timer, make_timeout_time_us(us), keep_enabled, exclusive);
}

/*! \brief  Sleep for a number of milliseconds
 *  \ingroup pico_low_power
 *
 * See \ref low_power_sleep_until_default_timer for more information.
 *
 * \param ms The number of milliseconds to sleep.
 * \param keep_enabled The clocks to keep enabled during sleep.
 * \param exclusive Whether to only listen for the timer interrupt, or other interrupts.
 * \return 0 on success, non-zero on error.
 */
static inline int low_power_sleep_for_ms(uint32_t ms, const clock_dest_bitset_t *keep_enabled, bool exclusive) {
    return low_power_sleep_until_default_timer(make_timeout_time_ms(ms), keep_enabled, exclusive);
}

/*! \brief  Go dormant for a number of milliseconds

 *  \ingroup pico_low_power
 *
 * See \ref low_power_dormant_until_aon_timer for more information.
 *
 * \param ms The number of milliseconds to go dormant for.
 * \param dormant_clock_source The clock source to use for dormant.
 * \param keep_enabled The clocks to keep enabled during dormant.
 * \return 0 on success, non-zero on error.
 */
static inline int low_power_dormant_for_ms(uint32_t ms, dormant_clock_source_t dormant_clock_source, const clock_dest_bitset_t *keep_enabled) {
    low_power_start_aon_timer();
    return low_power_dormant_until_aon_timer(aon_timer_make_timeout_time_ms(ms), dormant_clock_source, keep_enabled);
}

#if HAS_POWMAN_TIMER
/*! \brief  Go to Pstate for a number of milliseconds
 *  \ingroup pico_low_power
 *
 * See \ref low_power_pstate_until_aon_timer for more information.
 *
 * \param ms The number of milliseconds to go to Pstate for.
 * \param pstate The Pstate to use.
 * \param resume_func The function to call on reboot.
 * \return 0 on success, non-zero on error.
 */
static inline int low_power_pstate_for_ms(uint32_t ms, pstate_bitset_t *pstate, low_power_pstate_resume_func resume_func) {
    low_power_start_aon_timer();
    return low_power_pstate_until_aon_timer(aon_timer_make_timeout_time_ms(ms), pstate, resume_func);
}
#endif

#ifdef __cplusplus
}
#endif

#endif
