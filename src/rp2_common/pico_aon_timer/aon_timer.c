/*
 * Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/aon_timer.h"
#include "hardware/irq.h"
#include "hardware/sync.h"

static aon_timer_alarm_handler_t aon_timer_alarm_handler;

#if HAS_RP2040_RTC
#include "hardware/rtc.h"
#include "pico/util/datetime.h"
#include "pico/time.h"
#include "hardware/clocks.h"

#elif HAS_POWMAN_TIMER
#include "hardware/powman.h"

static void powman_timer_irq_handler(void) {
    uint irq_num = aon_timer_get_irq_num();
    // we are one-shot, so remove ourselves
    irq_set_enabled(irq_num, false);
    irq_remove_handler(irq_num, powman_timer_irq_handler);
    if (aon_timer_alarm_handler) aon_timer_alarm_handler();
}

static bool ts_to_tm(const struct timespec *ts, struct tm *tm) {
    return pico_localtime_r(&ts->tv_sec, tm) != NULL;
}
#endif

static bool tm_to_ts(const struct tm *tm, struct timespec *ts) {
    struct tm tm_clone = *tm;
    ts->tv_sec = pico_mktime(&tm_clone);
    ts->tv_nsec = 0;
    return ts->tv_sec != -1;
}

bool aon_timer_set_time(const struct timespec *ts) {
#if HAS_RP2040_RTC
    struct tm tm;
    bool ok = pico_localtime_r(&ts->tv_sec, &tm);
    if (ok) aon_timer_set_time_calendar(&tm);
    return ok;
#elif HAS_POWMAN_TIMER
    powman_timer_set_ms(timespec_to_ms(ts));
    return true;
#else
    panic_unsupported();
#endif
}

bool aon_timer_set_time_calendar(const struct tm *tm) {
#if HAS_RP2040_RTC
    datetime_t dt;
    tm_to_datetime(tm, &dt);
    rtc_set_datetime(&dt);

    // Writing to the RTC will take 2 clk_rtc clock periods to arrive
    uint rtc_freq = clock_get_hz(clk_rtc);
    busy_wait_us(((1000000 + rtc_freq - 1) / rtc_freq) * 2);
    return true;
#elif HAS_POWMAN_TIMER
    struct timespec ts;
    if (tm_to_ts(tm, &ts)) {
        return aon_timer_set_time(&ts);
    }
    return false;
#else
    panic_unsupported();
#endif
}

bool aon_timer_get_time(struct timespec *ts) {
#if HAS_RP2040_RTC
    struct tm tm;
    bool ok = aon_timer_get_time_calendar(&tm);
    return ok && tm_to_ts(&tm, ts);
#elif HAS_POWMAN_TIMER
    ms_to_timespec(powman_timer_get_ms(), ts);
    return true;
#else
    panic_unsupported();
#endif
}

bool aon_timer_get_time_calendar(struct tm *tm) {
#if HAS_RP2040_RTC
    datetime_t dt;
    rtc_get_datetime(&dt);
    datetime_to_tm(&dt, tm);
    return true;
#elif HAS_POWMAN_TIMER
    struct timespec ts;
    aon_timer_get_time(&ts);
    return ts_to_tm(&ts, tm);
#else
    panic_unsupported();
#endif
}

aon_timer_alarm_handler_t aon_timer_enable_alarm(const struct timespec *ts, aon_timer_alarm_handler_t handler, bool wakeup_from_low_power) {
#if HAS_RP2040_RTC
    struct tm tm;
    // adjust to after the target time
    struct timespec ts_adjusted = *ts;
    if (ts_adjusted.tv_nsec) ts_adjusted.tv_sec++;
    if (!pico_localtime_r(&ts_adjusted.tv_sec, &tm)) {
        return (aon_timer_alarm_handler_t)PICO_ERROR_INVALID_ARG;
    }
    return aon_timer_enable_alarm_calendar(&tm, handler, wakeup_from_low_power);
#elif HAS_POWMAN_TIMER
    uint32_t save = save_and_disable_interrupts();
    aon_timer_alarm_handler_t old_handler = aon_timer_alarm_handler;
    struct timespec ts_adjusted = *ts;
    uint irq_num = aon_timer_get_irq_num();
    powman_timer_disable_alarm();
    // adjust to after the target time
    ts_adjusted.tv_nsec += 999999;
    if (ts_adjusted.tv_nsec > 1000000000) {
       ts_adjusted.tv_nsec -= 1000000000;
       ts_adjusted.tv_sec++;
    }
    if (ts_adjusted.tv_nsec) ts_adjusted.tv_sec++;
    if (wakeup_from_low_power) {
        powman_enable_alarm_wakeup_at_ms(timespec_to_ms(ts));
    } else {
        powman_disable_alarm_wakeup();
        powman_timer_enable_alarm_at_ms(timespec_to_ms(ts));
    }
    if (handler) {
        irq_set_exclusive_handler(irq_num, powman_timer_irq_handler);
        irq_set_enabled(irq_num, true);
    }
    aon_timer_alarm_handler = handler;
    restore_interrupts_from_disabled(save);
    return old_handler;
#else
    panic_unsupported();
#endif
}

aon_timer_alarm_handler_t aon_timer_enable_alarm_calendar(const struct tm *tm, aon_timer_alarm_handler_t handler, bool wakeup_from_low_power) {
#if HAS_RP2040_RTC
    ((void)wakeup_from_low_power); // don't have a choice
    uint32_t save = save_and_disable_interrupts();
    aon_timer_alarm_handler_t old_handler = aon_timer_alarm_handler;
    datetime_t dt;
    tm_to_datetime(tm, &dt);
    rtc_set_alarm(&dt, handler);
    aon_timer_alarm_handler = handler;
    restore_interrupts_from_disabled(save);
    return old_handler;
#elif HAS_POWMAN_TIMER
    struct timespec ts;
    if (!tm_to_ts(tm, &ts)) {
        return (aon_timer_alarm_handler_t)PICO_ERROR_INVALID_ARG;
    }
    return aon_timer_enable_alarm(&ts, handler, wakeup_from_low_power);
#else
    panic_unsupported();
#endif
}

void aon_timer_disable_alarm(void) {
    irq_set_enabled(aon_timer_get_irq_num(), false);
#if HAS_RP2040_RTC
    rtc_disable_alarm();
#elif HAS_POWMAN_TIMER
    powman_timer_disable_alarm();
#else
    panic_unsupported();
#endif
}

void aon_timer_start_with_timeofday(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct timespec ts;
    ts.tv_sec = tv.tv_sec;
    ts.tv_nsec = tv.tv_usec * 1000;
    aon_timer_start(&ts);
}

bool aon_timer_start(const struct timespec *ts) {
#if HAS_RP2040_RTC
    rtc_init();
    return aon_timer_set_time(ts);
#elif HAS_POWMAN_TIMER
    // todo how best to allow different configurations; this should just be the default
    powman_timer_set_1khz_tick_source_xosc();
    bool ok = aon_timer_set_time(ts);
    if (ok) {
        powman_timer_set_ms(timespec_to_ms(ts));
        powman_timer_start();
    }
    return ok;
#else
    panic_unsupported();
#endif
}

bool aon_timer_start_calendar(const struct tm *tm) {
#if HAS_RP2040_RTC
    rtc_init();
    return aon_timer_set_time_calendar(tm);
#elif HAS_POWMAN_TIMER
    // todo how best to allow different configurations; this should just be the default
    powman_timer_set_1khz_tick_source_xosc();
    bool ok = aon_timer_set_time_calendar(tm);
    if (ok) {
        powman_timer_start();
    }
    return ok;
#else
    panic_unsupported();
#endif
}

void aon_timer_stop(void) {
#if HAS_RP2040_RTC
    hw_clear_bits(&rtc_hw->ctrl, RTC_CTRL_RTC_ENABLE_BITS);
#elif HAS_POWMAN_TIMER
    powman_timer_stop();
#else
    panic_unsupported();
#endif
}

void aon_timer_get_resolution(struct timespec *ts) {
#if HAS_RP2040_RTC
    ts->tv_sec = 1;
    ts->tv_nsec = 0;
#elif HAS_POWMAN_TIMER
    ts->tv_sec = 0;
    ts->tv_nsec = 1000000000 / 1000;
#else
    panic_unsupported();
#endif
}

bool aon_timer_is_running(void) {
#if HAS_RP2040_RTC
    return rtc_running();
#elif HAS_POWMAN_TIMER
    return powman_timer_is_running();
#else
    panic_unsupported();
#endif
}
