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

// Set this when setting an alarm
static rtc_callback_t _callback = NULL;

typedef enum {
    NO_REPEAT                =  0,
    CONTINUOUS_REPEAT        =  1,
    CONTINUOUS_REPEAT_ON_SEC =  2,
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

static bool valid_datetime(const datetime_t *t) {
    // Valid ranges taken from RTC doc. Note when setting an RTC alarm
    // these values are allowed to be -1 to say "don't match this value"
    if (!(t->year >= 0 && t->year <= 4095)) return false;
    if (!(t->month >= 1 && t->month <= 12)) return false;
    if (!(t->day >= 1 && t->day <= 31)) return false;
    if (!(t->dotw >= 0 && t->dotw <= 6)) return false;
    if (!(t->hour >= 0 && t->hour <= 23)) return false;
    if (!(t->min >= 0 && t->min <= 59)) return false;
    if (!(t->sec >= 0 && t->sec <= 59)) return false;
    return true;
}

bool rtc_set_datetime(const datetime_t *t) {
    if (!valid_datetime(t)) {
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

    // Note: RTC_0 should be read before RTC_1
    uint32_t rtc_0 = rtc_hw->rtc_0;
    uint32_t rtc_1 = rtc_hw->rtc_1;

    t->dotw  = (rtc_0 & RTC_RTC_0_DOTW_BITS ) >> RTC_RTC_0_DOTW_LSB;
    t->hour  = (rtc_0 & RTC_RTC_0_HOUR_BITS ) >> RTC_RTC_0_HOUR_LSB;
    t->min   = (rtc_0 & RTC_RTC_0_MIN_BITS  ) >> RTC_RTC_0_MIN_LSB;
    t->sec   = (rtc_0 & RTC_RTC_0_SEC_BITS  ) >> RTC_RTC_0_SEC_LSB;
    t->year  = (rtc_1 & RTC_RTC_1_YEAR_BITS ) >> RTC_RTC_1_YEAR_LSB;
    t->month = (rtc_1 & RTC_RTC_1_MONTH_BITS) >> RTC_RTC_1_MONTH_LSB;
    t->day   = (rtc_1 & RTC_RTC_1_DAY_BITS  ) >> RTC_RTC_1_DAY_LSB;

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

static void rtc_irq_handler(void) {
    // Always disable the alarm to clear the current IRQ.
    // Even if it is a repeatable alarm, we don't want it to keep firing.
    // If it matches on a second it can keep firing for that second.
    rtc_disable_alarm();

    if (_alarm_repeats) {
        // If it is a repeatable alarm, re enable the alarm.
        if(_alarm_repeats == -1) {
          datetime_t t;
          rtc_get_datetime(&t);
          rtc_hw->irq_setup_1 = RTC_IRQ_SETUP_1_SEC_ENA_BITS | ((((uint)t.sec + 1) % 60) << RTC_IRQ_SETUP_1_SEC_LSB);
        }
        rtc_enable_alarm();
    }

    // Call user callback function
    if (_callback) {
        _callback();
    }
}

// return != 0 on repeat, -1 on repeat every second
static int8_t rtc_alarm_repeats(const datetime_t *t) {
  // If any value is set to -1 then we don't match on that value
  // hence the alarm will eventually repeat
    if((t->year & t->month & t->day & t->dotw & t->hour & t->min & t->sec) < 0) return CONTINUOUS_REPEAT_ON_SEC;

    return (t->year < 0 || t->month < 0 || t->day < 0 || t->dotw < 0
            || t->hour < 0 || t->min < 0 || t->sec < 0)
        ? CONTINUOUS_REPEAT : NO_REPEAT;
}

bool rtc_set_alarm(const datetime_t *t, rtc_callback_t user_callback) {
    rtc_disable_alarm();

    uint32_t s0 = 0, s1 = 0;

    // Does it repeat? I.e. do we not match on any of the bits
    _alarm_repeats = rtc_alarm_repeats(t);

    if( (!valid_datetime(t) && !_alarm_repeats) || !user_callback)	// none of the parameters is valid
        return false;

    // Set the match enable bits for things we care about
    if (t->year  >= 0) s0 |= RTC_IRQ_SETUP_0_YEAR_ENA_BITS  | (((uint)t->year)  << RTC_IRQ_SETUP_0_YEAR_LSB );
    if (t->month >= 1) s0 |= RTC_IRQ_SETUP_0_MONTH_ENA_BITS | (((uint)t->month) << RTC_IRQ_SETUP_0_MONTH_LSB);
    if (t->day   >= 1) s0 |= RTC_IRQ_SETUP_0_DAY_ENA_BITS   | (((uint)t->day)   << RTC_IRQ_SETUP_0_DAY_LSB  );
    if (t->dotw  >= 0) s1 |= RTC_IRQ_SETUP_1_DOTW_ENA_BITS  | (((uint)t->dotw)  << RTC_IRQ_SETUP_1_DOTW_LSB);
    if (t->hour  >= 0) s1 |= RTC_IRQ_SETUP_1_HOUR_ENA_BITS  | (((uint)t->hour)  << RTC_IRQ_SETUP_1_HOUR_LSB);
    if (t->min   >= 0) s1 |= RTC_IRQ_SETUP_1_MIN_ENA_BITS   | (((uint)t->min)   << RTC_IRQ_SETUP_1_MIN_LSB);
    if (t->sec   >= 0) s1 |= RTC_IRQ_SETUP_1_SEC_ENA_BITS   | (((uint)t->sec)   << RTC_IRQ_SETUP_1_SEC_LSB);
    else if (_alarm_repeats == CONTINUOUS_REPEAT_ON_SEC) {
        // repeatable every second! All entries are -1
        datetime_t tNew;
        rtc_get_datetime(&tNew);
        s1 = RTC_IRQ_SETUP_1_SEC_ENA_BITS | ((((uint)tNew.sec + 1) % 60) << RTC_IRQ_SETUP_1_SEC_LSB);
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
