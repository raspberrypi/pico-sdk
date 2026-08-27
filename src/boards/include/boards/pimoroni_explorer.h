/*
 * Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------

// This header may be included by other board headers as "boards/pimoroni_explorer.h"

#ifndef _BOARDS_PIMORONI_EXPLORER_H
#define _BOARDS_PIMORONI_EXPLORER_H

pico_board_cmake_set(PICO_PLATFORM, rp2350)

// For board detection
#define PIMORONI_EXPLORER

// --- BOARD SPECIFIC ---
#define EXPLORER_SERVO_4_PIN 6
#define EXPLORER_SERVO_3_PIN 7
#define EXPLORER_SERVO_2_PIN 8
#define EXPLORER_SERVO_1_PIN 9

#define EXPLORER_PWM_AUDIO_PIN 12
#define EXPLORER_AMP_EN_PIN 13

#define EXPLORER_SW_C_PIN 14
#define EXPLORER_SW_B_PIN 15
#define EXPLORER_SW_A_PIN 16
#define EXPLORER_SW_X_PIN 17
#define EXPLORER_SW_Y_PIN 18
#define EXPLORER_SW_Z_PIN 19

#define EXPLORER_I2C 0
#define EXPLORER_SDA_PIN 20
#define EXPLORER_SCL_PIN 21

#define EXPLORER_USER_SW_PIN 22

#define EXPLORER_LED_PIN 25

#define EXPLORER_ADC_0_PIN 40
#define EXPLORER_ADC_1_PIN 41
#define EXPLORER_ADC_2_PIN 42
#define EXPLORER_ADC_3_PIN 43
#define EXPLORER_ADC_4_PIN 44
#define EXPLORER_ADC_5_PIN 45
#define EXPLORER_NUM_ADC_PINS 6

// --- RP2350 VARIANT ---
#define PICO_RP2350A 0

// --- UART ---
#ifndef PICO_DEFAULT_UART
#define PICO_DEFAULT_UART 0
#endif
#ifndef PICO_DEFAULT_UART_TX_PIN
#define PICO_DEFAULT_UART_TX_PIN 0
#endif
#ifndef PICO_DEFAULT_UART_RX_PIN
#define PICO_DEFAULT_UART_RX_PIN 1
#endif

// --- LED ---
#ifndef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN EXPLORER_LED_PIN
#endif
// no PICO_DEFAULT_WS2812_PIN

// --- I2C ---
// routed to Qw/St and Breakout Garden connectors
#ifndef PICO_DEFAULT_I2C
#define PICO_DEFAULT_I2C EXPLORER_I2C
#endif
#ifndef PICO_DEFAULT_I2C_SDA_PIN
#define PICO_DEFAULT_I2C_SDA_PIN EXPLORER_SDA_PIN
#endif
#ifndef PICO_DEFAULT_I2C_SCL_PIN
#define PICO_DEFAULT_I2C_SCL_PIN EXPLORER_SCL_PIN
#endif

// --- SPI ---
// no PICO_DEFAULT_SPI
// no PICO_DEFAULT_SPI_SCK_PIN
// no PICO_DEFAULT_SPI_TX_PIN
// no PICO_DEFAULT_SPI_RX_PIN
// no PICO_DEFAULT_SPI_CSN_PIN

// --- FLASH ---
#define PICO_BOOT_STAGE2_CHOOSE_W25Q080 1

#ifndef PICO_FLASH_SPI_CLKDIV
#define PICO_FLASH_SPI_CLKDIV 2
#endif

pico_board_cmake_set_default(PICO_FLASH_SIZE_BYTES, (16 * 1024 * 1024))
#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (16 * 1024 * 1024)
#endif

pico_board_cmake_set_default(PICO_RP2350_A2_SUPPORTED, 1)
#ifndef PICO_RP2350_A2_SUPPORTED
#define PICO_RP2350_A2_SUPPORTED 1
#endif

// no PICO_SMPS_MODE_PIN
// no PICO_VBUS_PIN
// no PICO_VSYS_PIN

#endif
