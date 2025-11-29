/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "hardware/gpio.h"

PICO_WEAK_FUNCTION_DEF(gpio_set_function)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_function)(__unused uint gpio, __unused enum gpio_function fn) {

}

PICO_WEAK_FUNCTION_DEF(gpio_set_function_masked)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_function_masked)(__unused uint32_t gpio_mask, __unused gpio_function_t fn) {

}

PICO_WEAK_FUNCTION_DEF(gpio_set_function_masked64)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_function_masked64)(__unused uint64_t gpio_mask, __unused gpio_function_t fn) {

}

PICO_WEAK_FUNCTION_DEF(gpio_get_function)
gpio_function_t PICO_WEAK_FUNCTION_IMPL_NAME(gpio_get_function)(__unused uint gpio) {
    return GPIO_FUNC_NULL;
}

PICO_WEAK_FUNCTION_DEF(gpio_pull_up)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_pull_up)(__unused uint gpio) {

}

PICO_WEAK_FUNCTION_DEF(gpio_is_pulled_up)
bool PICO_WEAK_FUNCTION_IMPL_NAME(gpio_is_pulled_up)(__unused uint gpio) {
    return 0;
}

PICO_WEAK_FUNCTION_DEF(gpio_pull_down)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_pull_down)(__unused uint gpio) {

}

PICO_WEAK_FUNCTION_DEF(gpio_is_pulled_down)
bool PICO_WEAK_FUNCTION_IMPL_NAME(gpio_is_pulled_down)(__unused uint gpio) {
    return 0;
}

PICO_WEAK_FUNCTION_DEF(gpio_disable_pulls)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_disable_pulls)(__unused uint gpio) {

}

PICO_WEAK_FUNCTION_DEF(gpio_set_pulls)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_pulls)(__unused uint gpio, __unused bool up, __unused bool down) {

}

PICO_WEAK_FUNCTION_DEF(gpio_set_irqover)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_irqover)(__unused uint gpio, __unused uint value) {

}

PICO_WEAK_FUNCTION_DEF(gpio_set_outover)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_outover)(__unused uint gpio, __unused uint value) {

}

PICO_WEAK_FUNCTION_DEF(gpio_set_inover)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_inover)(__unused uint gpio, __unused uint value) {

}

PICO_WEAK_FUNCTION_DEF(gpio_set_oeover)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_oeover)(__unused uint gpio, __unused uint value) {

}

PICO_WEAK_FUNCTION_DEF(gpio_set_input_enabled)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_input_enabled)(__unused uint gpio, __unused bool enabled){

}

PICO_WEAK_FUNCTION_DEF(gpio_set_input_hysteresis_enabled)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_input_hysteresis_enabled)(__unused uint gpio, __unused bool enabled){

}

PICO_WEAK_FUNCTION_DEF(gpio_is_input_hysteresis_enabled)
bool PICO_WEAK_FUNCTION_IMPL_NAME(gpio_is_input_hysteresis_enabled)(__unused uint gpio){
    return true;
}

PICO_WEAK_FUNCTION_DEF(gpio_set_slew_rate)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_slew_rate)(__unused uint gpio, __unused enum gpio_slew_rate slew){

}

PICO_WEAK_FUNCTION_DEF(gpio_get_slew_rate)
enum gpio_slew_rate PICO_WEAK_FUNCTION_IMPL_NAME(gpio_get_slew_rate)(__unused uint gpio){
    return GPIO_SLEW_RATE_FAST;
}

PICO_WEAK_FUNCTION_DEF(gpio_set_drive_strength)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_drive_strength)(__unused uint gpio, __unused enum gpio_drive_strength drive){

}

PICO_WEAK_FUNCTION_DEF(gpio_get_drive_strength)
enum gpio_drive_strength PICO_WEAK_FUNCTION_IMPL_NAME(gpio_get_drive_strength)(__unused uint gpio){
    return GPIO_DRIVE_STRENGTH_4MA;
}

PICO_WEAK_FUNCTION_DEF(gpio_set_irq_enabled)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_irq_enabled)(__unused uint gpio, __unused uint32_t events, __unused bool enable) {

}

PICO_WEAK_FUNCTION_DEF(gpio_set_irq_callback)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_irq_callback)(__unused gpio_irq_callback_t callback) {

}

PICO_WEAK_FUNCTION_DEF(gpio_set_irq_enabled_with_callback)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_irq_enabled_with_callback)(__unused uint gpio, __unused uint32_t event_mask, __unused bool enabled, __unused gpio_irq_callback_t callback) {

}

PICO_WEAK_FUNCTION_DEF(gpio_set_dormant_irq_enabled)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_dormant_irq_enabled)(__unused uint gpio, __unused uint32_t event_mask, __unused bool enabled) {

}

PICO_WEAK_FUNCTION_DEF(gpio_get_irq_event_mask)
uint32_t PICO_WEAK_FUNCTION_IMPL_NAME(gpio_get_irq_event_mask)(__unused uint gpio) {
    return 0;
}

PICO_WEAK_FUNCTION_DEF(gpio_acknowledge_irq)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_acknowledge_irq)(__unused uint gpio, __unused uint32_t events) {

}

PICO_WEAK_FUNCTION_DEF(gpio_add_raw_irq_handler_with_order_priority_masked)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_add_raw_irq_handler_with_order_priority_masked)(__unused uint32_t gpio_mask, __unused irq_handler_t handler, __unused uint8_t order_priority) {

}

PICO_WEAK_FUNCTION_DEF(gpio_add_raw_irq_handler_with_order_priority_masked64)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_add_raw_irq_handler_with_order_priority_masked64)(__unused uint64_t gpio_mask, __unused irq_handler_t handler, __unused uint8_t order_priority) {

}

PICO_WEAK_FUNCTION_DEF(gpio_add_raw_irq_handler_with_order_priority)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_add_raw_irq_handler_with_order_priority)(__unused uint gpio, __unused irq_handler_t handler, __unused uint8_t order_priority) {

}

PICO_WEAK_FUNCTION_DEF(gpio_add_raw_irq_handler_masked)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_add_raw_irq_handler_masked)(__unused uint32_t gpio_mask, __unused irq_handler_t handler) {

}

PICO_WEAK_FUNCTION_DEF(gpio_add_raw_irq_handler_masked64)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_add_raw_irq_handler_masked64)(__unused uint64_t gpio_mask, __unused irq_handler_t handler) {

}

PICO_WEAK_FUNCTION_DEF(gpio_add_raw_irq_handler)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_add_raw_irq_handler)(__unused uint gpio, __unused irq_handler_t handler) {

}

PICO_WEAK_FUNCTION_DEF(gpio_remove_raw_irq_handler_masked)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_remove_raw_irq_handler_masked)(__unused uint32_t gpio_mask, __unused irq_handler_t handler) {

}

PICO_WEAK_FUNCTION_DEF(gpio_remove_raw_irq_handler_masked64)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_remove_raw_irq_handler_masked64)(__unused uint64_t gpio_mask, __unused irq_handler_t handler) {

}

PICO_WEAK_FUNCTION_DEF(gpio_remove_raw_irq_handler)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_remove_raw_irq_handler)(__unused uint gpio, __unused irq_handler_t handler) {

}

PICO_WEAK_FUNCTION_DEF(gpio_init)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_init)(__unused uint gpio) {

}

PICO_WEAK_FUNCTION_DEF(gpio_deinit)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_deinit)(__unused uint gpio) {

}

PICO_WEAK_FUNCTION_DEF(gpio_init_mask)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_init_mask)(__unused uint gpio_mask) {

}

PICO_WEAK_FUNCTION_DEF(gpio_get)
bool PICO_WEAK_FUNCTION_IMPL_NAME(gpio_get)(__unused uint gpio) {
    return 0;
}

PICO_WEAK_FUNCTION_DEF(gpio_get_all)
uint32_t PICO_WEAK_FUNCTION_IMPL_NAME(gpio_get_all)(void) {
    return 0;
}

PICO_WEAK_FUNCTION_DEF(gpio_get_all46)
uint64_t PICO_WEAK_FUNCTION_IMPL_NAME(gpio_get_all64)(void) {
    return 0;
}

PICO_WEAK_FUNCTION_DEF(gpio_set_mask)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_mask)(__unused uint32_t mask) {

}

PICO_WEAK_FUNCTION_DEF(gpio_set_mask64)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_mask64)(__unused uint64_t mask) {

}

PICO_WEAK_FUNCTION_DEF(gpio_set_mask_n)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_mask_n)(__unused uint n, __unused uint32_t mask) {

}


PICO_WEAK_FUNCTION_DEF(gpio_clr_mask)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_clr_mask)(__unused uint32_t mask) {

}

PICO_WEAK_FUNCTION_DEF(gpio_clr_mask64)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_clr_mask64)(__unused uint64_t mask) {

}

PICO_WEAK_FUNCTION_DEF(gpio_clr_mask_n)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_clr_mask_n)(__unused uint n, __unused uint32_t mask) {

}

PICO_WEAK_FUNCTION_DEF(gpio_xor_mask)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_xor_mask)(__unused uint32_t mask) {

}

PICO_WEAK_FUNCTION_DEF(gpio_xor_mask64)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_xor_mask64)(__unused uint64_t mask) {

}

PICO_WEAK_FUNCTION_DEF(gpio_xor_mask_n)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_xor_mask_n)(__unused uint n, __unused uint32_t mask) {

}

PICO_WEAK_FUNCTION_DEF(gpio_put_masked)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_put_masked)(__unused uint32_t mask, __unused uint32_t value) {

}

PICO_WEAK_FUNCTION_DEF(gpio_put_masked64)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_put_masked64)(__unused uint64_t mask, __unused uint64_t value) {

}

PICO_WEAK_FUNCTION_DEF(gpio_put_mask_n)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_put_mask_n)(__unused uint n, __unused uint32_t mask) {

}

PICO_WEAK_FUNCTION_DEF(gpio_put_all)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_put_all)(__unused uint32_t value) {

}

PICO_WEAK_FUNCTION_DEF(gpio_put_all64)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_put_all64)(__unused uint64_t value) {

}
PICO_WEAK_FUNCTION_DEF(gpio_put)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_put)(__unused uint gpio, __unused int value) {

}

PICO_WEAK_FUNCTION_DEF(gpio_set_dir_out_masked)
bool PICO_WEAK_FUNCTION_IMPL_NAME(gpio_get_out_level)(__unused uint gpio) {
    return 0;
}

PICO_WEAK_FUNCTION_DEF(gpio_set_dir_out_masked)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_dir_out_masked)(__unused uint32_t mask) {

}

PICO_WEAK_FUNCTION_DEF(gpio_set_dir_out_masked64)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_dir_out_masked64)(__unused uint64_t mask) {

}

PICO_WEAK_FUNCTION_DEF(gpio_set_dir_in_masked)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_dir_in_masked)(__unused uint32_t mask) {

}

PICO_WEAK_FUNCTION_DEF(gpio_set_dir_in_masked64)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_dir_in_masked64)(__unused uint64_t mask) {

}
PICO_WEAK_FUNCTION_DEF(gpio_set_dir_masked)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_dir_masked)(__unused uint32_t mask, __unused uint32_t value) {

}

PICO_WEAK_FUNCTION_DEF(gpio_set_dir_masked64)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_dir_masked64)(__unused uint64_t mask, __unused uint64_t value) {

}

PICO_WEAK_FUNCTION_DEF(gpio_set_dir_all_bits)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_dir_all_bits)(__unused uint32_t value) {

}

PICO_WEAK_FUNCTION_DEF(gpio_set_dir_all_bits64)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_dir_all_bits64)(__unused uint64_t value) {

}

PICO_WEAK_FUNCTION_DEF(gpio_set_dir)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_dir)(__unused uint gpio, __unused bool out) {

}

PICO_WEAK_FUNCTION_DEF(gpio_is_dir_out)
bool PICO_WEAK_FUNCTION_IMPL_NAME(gpio_is_dir_out)(__unused uint gpio) {
    return 0;
}

PICO_WEAK_FUNCTION_DEF(gpio_get_dir)
uint PICO_WEAK_FUNCTION_IMPL_NAME(gpio_get_dir)(uint gpio) {
    return gpio_is_dir_out(gpio); // note GPIO_OUT is 1/true and GPIO_IN is 0/false anyway
}

PICO_WEAK_FUNCTION_DEF(gpio_assign_to_ns)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_assign_to_ns)(__unused uint gpio, __unused bool ns) {

}

PICO_WEAK_FUNCTION_DEF(gpio_debug_pins_init)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_debug_pins_init)() {

}
