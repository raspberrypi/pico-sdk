/*
 * Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_TIME_ADAPTER_H
#define _PICO_TIME_ADAPTER_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TA_NUM_TIMERS
#define TA_NUM_TIMERS 1
#endif

#ifndef TA_NUM_TIMER_ALARMS
#define TA_NUM_TIMER_ALARMS 4
#endif

void ta_clear_force_irq(alarm_pool_timer_t *timer, uint hardware_alarm_num);
void ta_clear_irq(alarm_pool_timer_t *timer, uint hardware_alarm_num);
void ta_force_irq(alarm_pool_timer_t *timer, uint hardware_alarm_num);
// This implementation compares full 64-bit microseconds. See TA_COMPARE_BITS in the rp2_common
// adapter for what this governs and why 32 is preferred where an adapter has the choice; 64 is
// used here only because the host has no 32-bit compare to match, and it is not racing two real
// cores.
#define TA_COMPARE_BITS 64

void ta_set_timeout(alarm_pool_timer_t *timer, uint hardware_alarm_num, int64_t target);
bool ta_wakes_up_on_or_before(alarm_pool_timer_t *timer, uint alarm_num, int64_t target);
void ta_enable_irq_handler(alarm_pool_timer_t *timer, uint hardware_alarm_num, void (*irq_handler)(void));
void ta_disable_irq_handler(alarm_pool_timer_t *timer, uint hardware_alarm_num, void (*irq_handler)(void));
void ta_hardware_alarm_claim(alarm_pool_timer_t *timer, uint hardware_alarm_num);
int ta_hardware_alarm_claim_unused(alarm_pool_timer_t *timer, bool required);
alarm_pool_timer_t *ta_from_current_irq(uint *alarm_num);
uint ta_timer_num(alarm_pool_timer_t *timer);
static inline uint64_t ta_time_us_64(__unused alarm_pool_timer_t *timer) {
    return time_us_64();
}
alarm_pool_timer_t *ta_timer_instance(uint instance_num);
alarm_pool_timer_t *ta_default_timer_instance(void);

#ifdef __cplusplus
}
#endif

#endif
