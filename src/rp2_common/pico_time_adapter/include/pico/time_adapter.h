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

static inline void ta_set_timeout(alarm_pool_timer_t *timer, uint alarm_num, int64_t target) {
    timer_hw_from_timer(timer)->alarm[alarm_num] = (uint32_t) target;
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
    hw_clear_bits(&timer_hw_from_timer(timer)->inte, 1u << alarm_num);
    irq_set_enabled(irq_num, true);
    irq_remove_handler(irq_num, irq_handler);
    hardware_alarm_unclaim(alarm_num);
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
#endif
