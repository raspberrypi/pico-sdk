/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_TONE_H
#define _PICO_TONE_H

#ifdef __cplusplus
extern "C" {
#endif

/** \file tone.h
 *  \defgroup pico_tone pico_tone
 *  Adds support for playing tones using PWM.
 *  
 */

// PICO_TONE_SILENCE_DELAY_MS, Default delay between tones in milliseconds
#ifndef PICO_TONE_SILENCE_DELAY_MS
#define PICO_TONE_SILENCE_DELAY_MS 10U
#endif

/*! \brief Initialise the tone generator
 *  \ingroup pico_tone
 *
 *  Initilise PWM on the given GPIO using the default pwm config.
 *
 *  \param gpio The GPIO to use for the tone generator
 */
void tone_init(uint gpio);

/*! \brief Play a tone for a given duration
 *  \ingroup pico_tone
 *
 *  Play a tone on the given GPIO for the given duration.
 *
 *  \param gpio The GPIO to use for the tone generator
 *  \param freq The frequency of the tone in Hz
 *  \param duration_ms The duration of the tone in milliseconds
 */
void tone(uint gpio, uint freq, float duration_ms);

/*! \brief Do not play any tone.
 *  \ingroup pico_tone
 *
 *  Stop playing a tone on the given GPIO.
 *
 *  \param gpio The GPIO to use for the tone generator
 */
void no_tone(uint gpio);


#ifdef __cplusplus
}
#endif

#endif
