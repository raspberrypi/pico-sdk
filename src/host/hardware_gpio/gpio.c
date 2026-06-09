/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "hardware/gpio.h"

PICO_WEAK_FUNCTION_DEF(check_gpio_param)
void PICO_WEAK_FUNCTION_IMPL_NAME(check_gpio_param)(__unused uint gpio) {

}

PICO_WEAK_FUNCTION_DEF(gpio_set_function)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_function)(uint gpio, __unused enum gpio_function fn) {
    check_gpio_param(gpio);
}

PICO_WEAK_FUNCTION_DEF(gpio_set_function_masked)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_function_masked)(__unused uint32_t gpio_mask, __unused gpio_function_t fn) {

}

PICO_WEAK_FUNCTION_DEF(gpio_set_function_masked64)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_function_masked64)(__unused uint64_t gpio_mask, __unused gpio_function_t fn) {

}

PICO_WEAK_FUNCTION_DEF(gpio_get_function)
gpio_function_t PICO_WEAK_FUNCTION_IMPL_NAME(gpio_get_function)(uint gpio) {
    check_gpio_param(gpio);
    return GPIO_FUNC_NULL;
}

PICO_WEAK_FUNCTION_DEF(gpio_pull_up)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_pull_up)(uint gpio) {
    check_gpio_param(gpio);
}

PICO_WEAK_FUNCTION_DEF(gpio_is_pulled_up)
bool PICO_WEAK_FUNCTION_IMPL_NAME(gpio_is_pulled_up)(uint gpio) {
    check_gpio_param(gpio);
    return 0;
}

PICO_WEAK_FUNCTION_DEF(gpio_pull_down)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_pull_down)(uint gpio) {
    check_gpio_param(gpio);
}

PICO_WEAK_FUNCTION_DEF(gpio_is_pulled_down)
bool PICO_WEAK_FUNCTION_IMPL_NAME(gpio_is_pulled_down)(uint gpio) {
    check_gpio_param(gpio);
    return 0;
}

PICO_WEAK_FUNCTION_DEF(gpio_disable_pulls)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_disable_pulls)(uint gpio) {
    check_gpio_param(gpio);
}

PICO_WEAK_FUNCTION_DEF(gpio_set_pulls)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_pulls)(uint gpio, __unused bool up, __unused bool down) {
    check_gpio_param(gpio);
}

PICO_WEAK_FUNCTION_DEF(gpio_set_irqover)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_irqover)(uint gpio, __unused uint value) {
    check_gpio_param(gpio);
}

PICO_WEAK_FUNCTION_DEF(gpio_set_outover)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_outover)(uint gpio, __unused uint value) {
    check_gpio_param(gpio);
}

PICO_WEAK_FUNCTION_DEF(gpio_set_inover)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_inover)(uint gpio, __unused uint value) {
    check_gpio_param(gpio);
}

PICO_WEAK_FUNCTION_DEF(gpio_set_oeover)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_oeover)(uint gpio, __unused uint value) {
    check_gpio_param(gpio);
}

PICO_WEAK_FUNCTION_DEF(gpio_set_input_enabled)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_input_enabled)(uint gpio, __unused bool enabled){
    check_gpio_param(gpio);
}

PICO_WEAK_FUNCTION_DEF(gpio_set_input_hysteresis_enabled)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_input_hysteresis_enabled)(uint gpio, __unused bool enabled){
    check_gpio_param(gpio);
}

PICO_WEAK_FUNCTION_DEF(gpio_is_input_hysteresis_enabled)
bool PICO_WEAK_FUNCTION_IMPL_NAME(gpio_is_input_hysteresis_enabled)(uint gpio){
    check_gpio_param(gpio);
    return true;
}

PICO_WEAK_FUNCTION_DEF(gpio_set_slew_rate)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_slew_rate)(uint gpio, __unused enum gpio_slew_rate slew){
    check_gpio_param(gpio);
}

PICO_WEAK_FUNCTION_DEF(gpio_get_slew_rate)
enum gpio_slew_rate PICO_WEAK_FUNCTION_IMPL_NAME(gpio_get_slew_rate)(uint gpio){
    check_gpio_param(gpio);
    return GPIO_SLEW_RATE_FAST;
}

PICO_WEAK_FUNCTION_DEF(gpio_set_drive_strength)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_drive_strength)(uint gpio, __unused enum gpio_drive_strength drive){
    check_gpio_param(gpio);
}

PICO_WEAK_FUNCTION_DEF(gpio_get_drive_strength)
enum gpio_drive_strength PICO_WEAK_FUNCTION_IMPL_NAME(gpio_get_drive_strength)(uint gpio){
    check_gpio_param(gpio);
    return GPIO_DRIVE_STRENGTH_4MA;
}

PICO_WEAK_FUNCTION_DEF(gpio_set_irq_enabled)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_irq_enabled)(uint gpio, __unused uint32_t events, __unused bool enable) {
    check_gpio_param(gpio);
}

PICO_WEAK_FUNCTION_DEF(gpio_set_irq_callback)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_irq_callback)(__unused gpio_irq_callback_t callback) {

}

PICO_WEAK_FUNCTION_DEF(gpio_set_irq_enabled_with_callback)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_irq_enabled_with_callback)(uint gpio, __unused uint32_t event_mask, __unused bool enabled, __unused gpio_irq_callback_t callback) {
    check_gpio_param(gpio);
}

PICO_WEAK_FUNCTION_DEF(gpio_set_dormant_irq_enabled)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_dormant_irq_enabled)(uint gpio, __unused uint32_t event_mask, __unused bool enabled) {
    check_gpio_param(gpio);
}

PICO_WEAK_FUNCTION_DEF(gpio_get_irq_event_mask)
uint32_t PICO_WEAK_FUNCTION_IMPL_NAME(gpio_get_irq_event_mask)(uint gpio) {
    check_gpio_param(gpio);
    return 0;
}

PICO_WEAK_FUNCTION_DEF(gpio_acknowledge_irq)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_acknowledge_irq)(uint gpio, __unused uint32_t events) {
    check_gpio_param(gpio);
}

PICO_WEAK_FUNCTION_DEF(gpio_add_raw_irq_handler_with_order_priority_masked)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_add_raw_irq_handler_with_order_priority_masked)(__unused uint32_t gpio_mask, __unused irq_handler_t handler, __unused uint8_t order_priority) {

}

PICO_WEAK_FUNCTION_DEF(gpio_add_raw_irq_handler_with_order_priority_masked64)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_add_raw_irq_handler_with_order_priority_masked64)(__unused uint64_t gpio_mask, __unused irq_handler_t handler, __unused uint8_t order_priority) {

}

PICO_WEAK_FUNCTION_DEF(gpio_add_raw_irq_handler_with_order_priority)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_add_raw_irq_handler_with_order_priority)(uint gpio, __unused irq_handler_t handler, __unused uint8_t order_priority) {
    check_gpio_param(gpio);
}

PICO_WEAK_FUNCTION_DEF(gpio_add_raw_irq_handler_masked)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_add_raw_irq_handler_masked)(__unused uint32_t gpio_mask, __unused irq_handler_t handler) {

}

PICO_WEAK_FUNCTION_DEF(gpio_add_raw_irq_handler_masked64)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_add_raw_irq_handler_masked64)(__unused uint64_t gpio_mask, __unused irq_handler_t handler) {

}

PICO_WEAK_FUNCTION_DEF(gpio_add_raw_irq_handler)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_add_raw_irq_handler)(uint gpio, __unused irq_handler_t handler) {
    check_gpio_param(gpio);
}

PICO_WEAK_FUNCTION_DEF(gpio_remove_raw_irq_handler_masked)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_remove_raw_irq_handler_masked)(__unused uint32_t gpio_mask, __unused irq_handler_t handler) {

}

PICO_WEAK_FUNCTION_DEF(gpio_remove_raw_irq_handler_masked64)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_remove_raw_irq_handler_masked64)(__unused uint64_t gpio_mask, __unused irq_handler_t handler) {

}

PICO_WEAK_FUNCTION_DEF(gpio_remove_raw_irq_handler)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_remove_raw_irq_handler)(uint gpio, __unused irq_handler_t handler) {
    check_gpio_param(gpio);
}

PICO_WEAK_FUNCTION_DEF(gpio_init)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_init)(uint gpio) {
    check_gpio_param(gpio);
}

PICO_WEAK_FUNCTION_DEF(gpio_deinit)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_deinit)(uint gpio) {
    check_gpio_param(gpio);
}

PICO_WEAK_FUNCTION_DEF(gpio_init_mask)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_init_mask)(__unused uint32_t gpio_mask) {

}

PICO_WEAK_FUNCTION_DEF(gpio_init_mask64)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_init_mask64)(__unused uint64_t gpio_mask) {

}

PICO_WEAK_FUNCTION_DEF(gpio_get)
bool PICO_WEAK_FUNCTION_IMPL_NAME(gpio_get)(uint gpio) {
    check_gpio_param(gpio);
    return 0;
}

PICO_WEAK_FUNCTION_DEF(gpio_get_all)
uint32_t PICO_WEAK_FUNCTION_IMPL_NAME(gpio_get_all)(void) {
    return 0;
}

PICO_WEAK_FUNCTION_DEF(gpio_get_all64)
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
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_put)(uint gpio, __unused int value) {
    check_gpio_param(gpio);
}

PICO_WEAK_FUNCTION_DEF(gpio_set_dir_out_masked)
bool PICO_WEAK_FUNCTION_IMPL_NAME(gpio_get_out_level)(uint gpio) {
    check_gpio_param(gpio);
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
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_dir)(uint gpio, __unused bool out) {
    check_gpio_param(gpio);
}

PICO_WEAK_FUNCTION_DEF(gpio_is_dir_out)
bool PICO_WEAK_FUNCTION_IMPL_NAME(gpio_is_dir_out)(uint gpio) {
    check_gpio_param(gpio);
    return 0;
}

PICO_WEAK_FUNCTION_DEF(gpio_get_dir)
uint PICO_WEAK_FUNCTION_IMPL_NAME(gpio_get_dir)(uint gpio) {
    return gpio_is_dir_out(gpio); // note GPIO_OUT is 1/true and GPIO_IN is 0/false anyway
}

PICO_WEAK_FUNCTION_DEF(gpio_assign_to_ns)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_assign_to_ns)(uint gpio, __unused bool ns) {
    check_gpio_param(gpio);
}

PICO_WEAK_FUNCTION_DEF(gpio_debug_pins_init)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_debug_pins_init)() {

}
