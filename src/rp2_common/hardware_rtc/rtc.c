/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico.h"

#include "hardware/irq.h"
#include "hardware/rtc.h"
#include "hardware/resets.h"
#include "hardware/clocks.h"

#define RANGE_CHECK_YEAR(t)  (t->year >= 0 && t->year <= 4095)
#define RANGE_CHECK_MONTH(t) (t->month >= 1 && t->month <= 12)
#define RANGE_CHECK_DAY(t)   (t->day >= 1 && t->day <= 32)
#define RANGE_CHECK_DOTW(t)  (t->dotw >= 0 && t->dotw <= 6)
#define RANGE_CHECK_HOUR(t)  (t->hour >= 0 && t->hour <= 23)
#define RANGE_CHECK_MIN(t)   (t->min >= 0 && t->min <= 59)
#define RANGE_CHECK_SEC(t)   (t->sec >= 0 && t->sec <= 59)


// Set this when setting an alarm
static rtc_callback_t _callback = NULL;
static uint8_t _seconds_increment = 1;

#define ADD_AND_ENABLE_REPEATABLE_SECOND(s) (RTC_IRQ_SETUP_1_SEC_ENA_BITS | ((((uint)s + _seconds_increment) % 60) << RTC_IRQ_SETUP_1_SEC_LSB))

typedef enum {
    NO_REPEAT                   =  0,
    CONTINUOUS_REPEAT           =  1,
    CONTINUOUS_REPEAT_EVERY_SEC =  2,
} repeat_type;

static repeat_type _alarm_repeats = NO_REPEAT;

bool rtc_running(void) {
    return (rtc_hw->ctrl & RTC_CTRL_RTC_ACTIVE_BITS);
}

void rtc_init(void) {
    // Get clk_rtc freq and make sure it is running
    uint rtc_freq = clock_get_hz(clk_rtc);
    assert(rtc_freq != 0);

    // Take rtc out of reset now that we know clk_rtc is running
    reset_block(RESETS_RESET_RTC_BITS);
    unreset_block_wait(RESETS_RESET_RTC_BITS);

    // Set up the 1 second divider.
    // If rtc_freq is 400 then clkdiv_m1 should be 399
    rtc_freq -= 1;

    // Check the freq is not too big to divide
    assert(rtc_freq <= RTC_CLKDIV_M1_BITS);

    // Write divide value
    rtc_hw->clkdiv_m1 = rtc_freq;
}

static bool is_valid_datetime(const datetime_t *t) {
    // Valid ranges taken from RTC doc. Note when setting an RTC alarm
    // these values are allowed to be -1 to say "don't match this value"
    return RANGE_CHECK_YEAR(t)
        && RANGE_CHECK_MONTH(t)
        && RANGE_CHECK_DAY(t)
        && RANGE_CHECK_DOTW(t)
        && RANGE_CHECK_HOUR(t)
        && RANGE_CHECK_MIN(t)
        && RANGE_CHECK_SEC(t);
}

// small helper without check for running rtc
static inline datetime_t _rtc_get_datetime(datetime_t *t) {
  // Note: RTC_0 should be read before RTC_1
  uint32_t rtc_val = rtc_hw->rtc_0;
  t->dotw  = (rtc_val & RTC_RTC_0_DOTW_BITS) >> RTC_RTC_0_DOTW_LSB;
  t->hour  = (rtc_val & RTC_RTC_0_HOUR_BITS) >> RTC_RTC_0_HOUR_LSB;
  t->min   = (rtc_val & RTC_RTC_0_MIN_BITS) >> RTC_RTC_0_MIN_LSB;
  t->sec   = (rtc_val & RTC_RTC_0_SEC_BITS) >> RTC_RTC_0_SEC_LSB;

  rtc_val = rtc_hw->rtc_1;
  t->year  = (rtc_val & RTC_RTC_1_YEAR_BITS) >> RTC_RTC_1_YEAR_LSB;
  t->month = (rtc_val & RTC_RTC_1_MONTH_BITS) >> RTC_RTC_1_MONTH_LSB;
  t->day   = (rtc_val & RTC_RTC_1_DAY_BITS) >> RTC_RTC_1_DAY_LSB;
}

bool rtc_set_datetime(const datetime_t *t) {
    bool check_params = is_valid_datetime(t);
    valid_params_if(RTC, check_params);

    if (!check_params) {
        return false;
    }

    // Disable RTC
    rtc_hw->ctrl = 0;
    // Wait while it is still active
    while (rtc_running()) {
        tight_loop_contents();
    }

    // Write to setup registers
    rtc_hw->setup_0 = (((uint)t->year)  << RTC_SETUP_0_YEAR_LSB ) |
                      (((uint)t->month) << RTC_SETUP_0_MONTH_LSB) |
                      (((uint)t->day)   << RTC_SETUP_0_DAY_LSB);
    rtc_hw->setup_1 = (((uint)t->dotw)  << RTC_SETUP_1_DOTW_LSB) |
                      (((uint)t->hour)  << RTC_SETUP_1_HOUR_LSB) |
                      (((uint)t->min)   << RTC_SETUP_1_MIN_LSB)  |
                      (((uint)t->sec)   << RTC_SETUP_1_SEC_LSB);

    // Load setup values into rtc clock domain
    rtc_hw->ctrl = RTC_CTRL_LOAD_BITS;

    // Enable RTC and wait for it to be running
    rtc_hw->ctrl = RTC_CTRL_RTC_ENABLE_BITS;
    while (!rtc_running()) {
        tight_loop_contents();
    }

    return true;
}

bool rtc_get_datetime(datetime_t *t) {
    // Make sure RTC is running
    if (!rtc_running()) {
        return false;
    }

    _rtc_get_datetime(t);
    return true;
}

void rtc_enable_alarm(void) {
    // Set matching and wait for it to be enabled
    hw_set_bits(&rtc_hw->irq_setup_0, RTC_IRQ_SETUP_0_MATCH_ENA_BITS);
    while(!(rtc_hw->irq_setup_0 & RTC_IRQ_SETUP_0_MATCH_ACTIVE_BITS)) {
        tight_loop_contents();
    }
}

void rtc_disable_alarm(void) {
    // Disable matching and wait for it to stop being active
    hw_clear_bits(&rtc_hw->irq_setup_0, RTC_IRQ_SETUP_0_MATCH_ENA_BITS);
    while (rtc_hw->irq_setup_0 & RTC_IRQ_SETUP_0_MATCH_ACTIVE_BITS) {
        tight_loop_contents();
    }
}

static void __no_inline_not_in_flash_func(rtc_irq_handler)(void) {
    // Always disable the alarm to clear the current IRQ.
    // Even if it is a repeatable alarm, we don't want it to keep firing.
    // If it matches on a second it can keep firing for that second.
    rtc_disable_alarm();

    if (_alarm_repeats) {
        if(_alarm_repeats == CONTINUOUS_REPEAT_EVERY_SEC) {
          // we need to modify the sec entry with the next valid RTC change and store it into irq_setup_1
          datetime_t t;
          rtc_get_datetime(&t);
          rtc_hw->irq_setup_1 = ADD_AND_ENABLE_REPEATABLE_SECOND(t.sec);
        }
    }

    // Call user callback function
    _callback();

  // If it is a repeatable alarm, re enable the alarm.
  if(_alarm_repeats) {
        rtc_enable_alarm();
    }
}

static repeat_type rtc_alarm_repeats(const datetime_t *t) {
  // If any value is set to -1 then we don't match on that value
  // hence the alarm will eventually repeat
    if (t->year < 0 && t->month < 0 && t->day < 0 && t->dotw < 0
        && t->hour < 0 && t->min < 0 && t->sec < 0) return CONTINUOUS_REPEAT_EVERY_SEC;

    return (t->year < 0 || t->month < 0 || t->day < 0 || t->dotw < 0
            || t->hour < 0 || t->min < 0 || t->sec < 0)
        ? CONTINUOUS_REPEAT : NO_REPEAT;
}

bool rtc_set_alarm(const datetime_t *t, rtc_callback_t user_callback) {
    if (!rtc_running())
      return false;

    rtc_disable_alarm();

    uint32_t s0 = 0, s1 = 0;

    // Does it repeat? I.e. do we not match on any of the bits
    _alarm_repeats = rtc_alarm_repeats(t);

    bool check_params = (is_valid_datetime(t) || _alarm_repeats != NO_REPEAT) && user_callback;
    valid_params_if(RTC, check_params);
    if(!check_params)	// none of the parameters is valid
        return false;

    // Set the match enable bits for things we care about
    if(_alarm_repeats == CONTINUOUS_REPEAT_EVERY_SEC) {
        // repeatable every second! All entries are -1
        datetime_t new_dt;
        _rtc_get_datetime(&new_dt);
        _seconds_increment = (-t->sec) % 60;
        s1 = ADD_AND_ENABLE_REPEATABLE_SECOND(new_dt.sec);
    }
    else {
        if (RANGE_CHECK_YEAR(t))  s0 |= RTC_IRQ_SETUP_0_YEAR_ENA_BITS  | (((uint)t->year)  << RTC_IRQ_SETUP_0_YEAR_LSB);
        if (RANGE_CHECK_MONTH(t)) s0 |= RTC_IRQ_SETUP_0_MONTH_ENA_BITS | (((uint)t->month) << RTC_IRQ_SETUP_0_MONTH_LSB);
        if (RANGE_CHECK_DAY(t))   s0 |= RTC_IRQ_SETUP_0_DAY_ENA_BITS   | (((uint)t->day)   << RTC_IRQ_SETUP_0_DAY_LSB);
        if (RANGE_CHECK_DOTW(t))  s1 |= RTC_IRQ_SETUP_1_DOTW_ENA_BITS  | (((uint)t->dotw)  << RTC_IRQ_SETUP_1_DOTW_LSB);
        if (RANGE_CHECK_HOUR(t))  s1 |= RTC_IRQ_SETUP_1_HOUR_ENA_BITS  | (((uint)t->hour)  << RTC_IRQ_SETUP_1_HOUR_LSB);
        if (RANGE_CHECK_MIN(t))   s1 |= RTC_IRQ_SETUP_1_MIN_ENA_BITS   | (((uint)t->min)   << RTC_IRQ_SETUP_1_MIN_LSB);
        if (RANGE_CHECK_SEC(t))   s1 |= RTC_IRQ_SETUP_1_SEC_ENA_BITS   | (((uint)t->sec)   << RTC_IRQ_SETUP_1_SEC_LSB);

        if(!s0 && !s1) return false; // out of range datetime_t input
    }

    rtc_hw->irq_setup_0 = s0;
    rtc_hw->irq_setup_1 = s1;

    // Store function pointer we can call later
    _callback = user_callback;

    irq_set_exclusive_handler(RTC_IRQ, rtc_irq_handler);

    // Enable the IRQ at the peri
    rtc_hw->inte = RTC_INTE_RTC_BITS;

    // Enable the IRQ at the proc
    irq_set_enabled(RTC_IRQ, true);

    rtc_enable_alarm();
    return true;
}

void rtc_delete_alarm(void)
{
    if (_callback == NULL) {
        return; // rtc_set_alarm was not called or not successful
    }

    // first disable the alarm
    rtc_disable_alarm();

    // don't receive interrupts anymore
    rtc_hw->inte = RTC_INTE_RESET;

    // disable IRQ and remove handler
    irq_set_enabled(RTC_IRQ, false);
    irq_remove_handler(RTC_IRQ, _callback);

    _callback = NULL;
}
