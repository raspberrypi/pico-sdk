/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "pico/stdlib.h"
#include "pico/tone.h"
#include "hardware/pwm.h"

void tone_init(uint gpio) {
    // Configure GPIO for PWM output
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    // Find out which PWM slice is connected to GPIO
    uint slice_num = pwm_gpio_to_slice_num(gpio);
    // Get default configuration for PWM slice and initialise PWM with it
    pwm_config config = pwm_get_default_config();
    pwm_init(slice_num, &config, true);
}

void no_tone(uint gpio) {
    pwm_set_gpio_level(gpio, 0);
}

void tone(uint gpio, uint freq, uint32_t duration_ms) {
    // Calculate and cofigure new clock divider according to the frequency
    // This formula is assuming we are running at 125MHz.
    // TODO: Make this work for any frequency
    float clkdiv = (1.f / freq) * 2000.f;
    uint slice_num = pwm_gpio_to_slice_num(gpio);
    pwm_set_clkdiv(slice_num, clkdiv);
    // Configure duty to 50% ((2**16)-1)/2) to generate a square wave
    pwm_set_gpio_level(gpio, 32768U);
    sleep_ms(duration_ms);

    // Make silence for some ms to distinguish between tones
    no_tone(gpio);
    sleep_ms(PICO_TONE_SILENCE_DELAY_MS);
}
