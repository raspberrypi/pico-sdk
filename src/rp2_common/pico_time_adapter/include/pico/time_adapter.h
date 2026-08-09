/*
 * Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_TIME_ADAPTER_H
#define _PICO_TIME_ADAPTER_H

#include "hardware/irq.h"
#include "hardware/timer.h"
#include "pico/assert.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TA_NUM_TIMERS NUM_GENERIC_TIMERS
#define TA_NUM_TIMER_ALARMS NUM_ALARMS

#define timer_hw_from_timer(t) ((timer_hw_t *)(t))

static inline void ta_force_irq(alarm_pool_timer_t *timer, uint alarm_num) {
    hw_set_bits(&timer_hw_from_timer(timer)->intf, 1u << alarm_num);
}

static inline void ta_clear_force_irq(alarm_pool_timer_t *timer, uint alarm_num) {
    hw_clear_bits(&timer_hw_from_timer(timer)->intf, 1u << alarm_num);
}

static inline void ta_clear_irq(alarm_pool_timer_t *timer, uint alarm_num) {
    timer_hw_from_timer(timer)->intr = 1u << alarm_num;
}

static inline alarm_pool_timer_t *ta_from_current_irq(uint *alarm_num) {
    uint irq_num = __get_current_exception() - VTABLE_FIRST_IRQ;
    alarm_pool_timer_t *timer = timer_get_instance(TIMER_NUM_FROM_IRQ(irq_num));
    *alarm_num = TIMER_ALARM_NUM_FROM_IRQ(irq_num);
    return timer;
}

// How many bits of a target this adapter actually compares. The hardware alarm compares only
// the low 32 bits of the timer, so a target is honoured to that resolution and no better: an
// alarm armed for low word L fires at any time whose low word is L.
//
// Callers that record a target for later comparison should record exactly this many bits.
// Keeping more buys nothing - the extra bits describe a precision the timer cannot act on - and
// 32 is positively preferable to 64 where an adapter has the choice: a 32-bit aligned access is
// atomic on every core we target, so such a record can be shared between cores without locks,
// barriers or tearing, whereas a 64-bit one can be torn on a 32-bit core. An adapter backed by
// something other than the RP2 timer should prefer 32 unless it genuinely compares more.
//
// See best_effort_wfe_or_timeout() in pico_time, which records last_added at this width.
#define TA_COMPARE_BITS 32

static inline void ta_set_timeout(alarm_pool_timer_t *timer, uint alarm_num, int64_t target) {
    // We never want to set the timeout to be later than our current one.
    uint32_t current = timer_time_us_32(timer_hw_from_timer(timer));
    uint32_t time_til_target = (uint32_t) target - current;
    uint32_t time_til_alarm = timer_hw_from_timer(timer)->alarm[alarm_num] - current;
    // Note: we are only dealing with the low 32 bits of the timer values,
    // so there is some opportunity to make wrap-around errors.
    //
    // 1. If we just passed the alarm time, then time_til_alarm will be high, meaning we will
    //    likely do the update, but this is OK since the alarm will have just fired
    // 2. If we just passed the target time, then time_til_target will be high, meaning we will
    //    likely not do the update, but this is OK since the caller who has the full 64 bits
    //    must check if the target time has passed when we return anyway to avoid races.
    // 3. We should never leave here without an alarm being armed
    if (time_til_target < time_til_alarm || (timer_hw_from_timer(timer)->armed & (1 << alarm_num)) == 0) {
        timer_hw_from_timer(timer)->alarm[alarm_num] = (uint32_t) target;
    }
}

static inline bool ta_wakes_up_on_or_before(alarm_pool_timer_t *timer, uint alarm_num, int64_t target) {
    int64_t current = (int64_t)timer_time_us_64(timer_hw_from_timer(timer));
    int64_t time_til_target = target - current;
    uint32_t time_til_alarm = timer_hw_from_timer(timer)->alarm[alarm_num] - (uint32_t)current;
    return time_til_alarm <= time_til_target;
}

static inline uint64_t ta_time_us_64(alarm_pool_timer_t *timer) {
    return timer_time_us_64(timer_hw_from_timer(timer));
}

static inline void ta_enable_irq_handler(alarm_pool_timer_t *timer, uint alarm_num, irq_handler_t irq_handler) {
    // disarm the timer
    uint irq_num = timer_hardware_alarm_get_irq_num(timer, alarm_num);
    timer_hw_from_timer(timer)->armed = 1u << alarm_num;
    irq_set_exclusive_handler(irq_num, irq_handler);
    irq_set_enabled(irq_num, true);
    hw_set_bits(&timer_hw_from_timer(timer)->inte, 1u << alarm_num);
}

static inline void ta_disable_irq_handler(alarm_pool_timer_t *timer, uint alarm_num, irq_handler_t irq_handler) {
    uint irq_num = timer_hardware_alarm_get_irq_num(timer, alarm_num);
    timer_hw_from_timer(timer)->armed = 1u << alarm_num; // disarm the timer
    hw_clear_bits(&timer_hw_from_timer(timer)->inte, 1u << alarm_num);
    irq_set_enabled(irq_num, true);
    irq_remove_handler(irq_num, irq_handler);
    timer_hardware_alarm_unclaim(timer, alarm_num);
}

static inline void ta_hardware_alarm_claim(alarm_pool_timer_t *timer, uint hardware_alaram_num) {
    timer_hardware_alarm_claim(timer_hw_from_timer(timer), hardware_alaram_num);
}

static inline int ta_hardware_alarm_claim_unused(alarm_pool_timer_t *timer, bool required) {
    return timer_hardware_alarm_claim_unused(timer, required);
}

static inline alarm_pool_timer_t *ta_timer_instance(uint timer_num) {
    return timer_get_instance(timer_num);
}

static inline uint ta_timer_num(alarm_pool_timer_t *timer) {
    return timer_get_index(timer_hw_from_timer(timer));
}

static inline alarm_pool_timer_t *ta_default_timer_instance(void) {
    return PICO_DEFAULT_TIMER_INSTANCE();
}

#ifdef __cplusplus
}
#endif

#endif
