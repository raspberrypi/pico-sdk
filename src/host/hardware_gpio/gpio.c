/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "hardware/gpio.h"

PICO_WEAK_FUNCTION_DEF(gpio_set_function)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_function)(__unused uint gpio, __unused enum gpio_function fn) {

}

PICO_WEAK_FUNCTION_DEF(gpio_pull_up)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_pull_up)(__unused uint gpio) {

}

PICO_WEAK_FUNCTION_DEF(gpio_pull_down)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_pull_down)(__unused uint gpio) {

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

PICO_WEAK_FUNCTION_DEF(gpio_acknowledge_irq)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_acknowledge_irq)(__unused uint gpio, __unused uint32_t events) {

}

PICO_WEAK_FUNCTION_DEF(gpio_init)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_init)(__unused uint gpio) {

}

PICO_WEAK_FUNCTION_DEF(gpio_get)
bool PICO_WEAK_FUNCTION_IMPL_NAME(gpio_get)(__unused uint gpio) {
    return 0;
}

PICO_WEAK_FUNCTION_DEF(gpio_get_all)
uint32_t PICO_WEAK_FUNCTION_IMPL_NAME(gpio_get_all)() {
    return 0;
}

PICO_WEAK_FUNCTION_DEF(gpio_set_mask)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_mask)(__unused uint32_t mask) {

}

PICO_WEAK_FUNCTION_DEF(gpio_clr_mask)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_clr_mask)(__unused uint32_t mask) {

}

PICO_WEAK_FUNCTION_DEF(gpio_xor_mask)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_xor_mask)(__unused uint32_t mask) {

}

PICO_WEAK_FUNCTION_DEF(gpio_put_masked)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_put_masked)(__unused uint32_t mask, __unused uint32_t value) {

}

PICO_WEAK_FUNCTION_DEF(gpio_put_all)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_put_all)(__unused uint32_t value) {

}

PICO_WEAK_FUNCTION_DEF(gpio_put)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_put)(__unused uint gpio, __unused int value) {

}

PICO_WEAK_FUNCTION_DEF(gpio_set_dir_out_masked)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_dir_out_masked)(__unused uint32_t mask) {

}

PICO_WEAK_FUNCTION_DEF(gpio_set_dir_in_masked)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_dir_in_masked)(__unused uint32_t mask) {

}

PICO_WEAK_FUNCTION_DEF(gpio_set_dir_masked)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_dir_masked)(__unused uint32_t mask, __unused uint32_t value) {

}

PICO_WEAK_FUNCTION_DEF(gpio_set_dir_all_bits)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_dir_all_bits)(__unused uint32_t value) {

}

PICO_WEAK_FUNCTION_DEF(gpio_set_dir)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_dir)(__unused uint gpio, __unused bool out) {

}

PICO_WEAK_FUNCTION_DEF(gpio_debug_pins_init)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_debug_pins_init)() {

}

PICO_WEAK_FUNCTION_DEF(gpio_set_input_enabled)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_input_enabled)(__unused uint gpio, __unused bool enable) {

}

PICO_WEAK_FUNCTION_DEF(gpio_init_mask)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_init_mask)(__unused uint gpio_mask) {

}
