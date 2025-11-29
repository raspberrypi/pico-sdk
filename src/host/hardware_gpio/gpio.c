/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "hardware/gpio.h"

PICO_WEAK_FUNCTION_DEF(gpio_set_function)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_function)(uint gpio, enum gpio_function fn) {

}

PICO_WEAK_FUNCTION_DEF(gpio_pull_up)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_pull_up)(uint gpio) {

}

PICO_WEAK_FUNCTION_DEF(gpio_pull_down)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_pull_down)(uint gpio) {

}

PICO_WEAK_FUNCTION_DEF(gpio_disable_pulls)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_disable_pulls)(uint gpio) {

}

PICO_WEAK_FUNCTION_DEF(gpio_set_pulls)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_pulls)(uint gpio, bool up, bool down) {

}

PICO_WEAK_FUNCTION_DEF(gpio_set_irqover)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_irqover)(uint gpio, uint value) {

}

PICO_WEAK_FUNCTION_DEF(gpio_set_outover)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_outover)(uint gpio, uint value) {

}

PICO_WEAK_FUNCTION_DEF(gpio_set_inover)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_inover)(uint gpio, uint value) {

}

PICO_WEAK_FUNCTION_DEF(gpio_set_oeover)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_oeover)(uint gpio, uint value) {

}

PICO_WEAK_FUNCTION_DEF(gpio_set_input_hysteresis_enabled)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_input_hysteresis_enabled)(uint gpio, bool enabled){

}

PICO_WEAK_FUNCTION_DEF(gpio_is_input_hysteresis_enabled)
bool PICO_WEAK_FUNCTION_IMPL_NAME(gpio_is_input_hysteresis_enabled)(uint gpio){
    return true;
}

PICO_WEAK_FUNCTION_DEF(gpio_set_slew_rate)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_slew_rate)(uint gpio, enum gpio_slew_rate slew){

}

PICO_WEAK_FUNCTION_DEF(gpio_get_slew_rate)
enum gpio_slew_rate PICO_WEAK_FUNCTION_IMPL_NAME(gpio_get_slew_rate)(uint gpio){
    return GPIO_SLEW_RATE_FAST;
}

PICO_WEAK_FUNCTION_DEF(gpio_set_drive_strength)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_drive_strength)(uint gpio, enum gpio_drive_strength drive){

}

PICO_WEAK_FUNCTION_DEF(gpio_get_drive_strength)
enum gpio_drive_strength PICO_WEAK_FUNCTION_IMPL_NAME(gpio_get_drive_strength)(uint gpio){
    return GPIO_DRIVE_STRENGTH_4MA;
}

PICO_WEAK_FUNCTION_DEF(gpio_set_irq_enabled)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_irq_enabled)(uint gpio, uint32_t events, bool enable) {

}

PICO_WEAK_FUNCTION_DEF(gpio_acknowledge_irq)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_acknowledge_irq)(uint gpio, uint32_t events) {

}

PICO_WEAK_FUNCTION_DEF(gpio_init)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_init)(uint gpio) {

}

PICO_WEAK_FUNCTION_DEF(gpio_get)
bool PICO_WEAK_FUNCTION_IMPL_NAME(gpio_get)(uint gpio) {
    return 0;
}

PICO_WEAK_FUNCTION_DEF(gpio_get_all)
uint32_t PICO_WEAK_FUNCTION_IMPL_NAME(gpio_get_all)() {
    return 0;
}

PICO_WEAK_FUNCTION_DEF(gpio_set_mask)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_mask)(uint32_t mask) {

}

PICO_WEAK_FUNCTION_DEF(gpio_clr_mask)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_clr_mask)(uint32_t mask) {

}

PICO_WEAK_FUNCTION_DEF(gpio_xor_mask)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_xor_mask)(uint32_t mask) {

}

PICO_WEAK_FUNCTION_DEF(gpio_put_masked)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_put_masked)(uint32_t mask, uint32_t value) {

}

PICO_WEAK_FUNCTION_DEF(gpio_put_all)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_put_all)(uint32_t value) {

}

PICO_WEAK_FUNCTION_DEF(gpio_put)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_put)(uint gpio, int value) {

}

PICO_WEAK_FUNCTION_DEF(gpio_set_dir_out_masked)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_dir_out_masked)(uint32_t mask) {

}

PICO_WEAK_FUNCTION_DEF(gpio_set_dir_in_masked)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_dir_in_masked)(uint32_t mask) {

}

PICO_WEAK_FUNCTION_DEF(gpio_set_dir_masked)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_dir_masked)(uint32_t mask, uint32_t value) {

}

PICO_WEAK_FUNCTION_DEF(gpio_set_dir_all_bits)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_dir_all_bits)(uint32_t value) {

}

PICO_WEAK_FUNCTION_DEF(gpio_set_dir)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_dir)(uint gpio, bool out) {

}

PICO_WEAK_FUNCTION_DEF(gpio_debug_pins_init)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_debug_pins_init)() {

}

PICO_WEAK_FUNCTION_DEF(gpio_set_input_enabled)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_set_input_enabled)(uint gpio, bool enable) {

}

PICO_WEAK_FUNCTION_DEF(gpio_init_mask)
void PICO_WEAK_FUNCTION_IMPL_NAME(gpio_init_mask)(uint gpio_mask) {

}
