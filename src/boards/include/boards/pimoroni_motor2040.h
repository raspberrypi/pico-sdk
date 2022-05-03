/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------

#ifndef _BOARDS_PIMORONI_MOTOR2040_H
#define _BOARDS_PIMORONI_MOTOR2040_H

// For board detection
#define PIMORONI_MOTOR2040

#ifndef MOTOR2040_USE_DISTANCE_SENSOR
// default to 0, unless explicitly set to 1
#define MOTOR2040_USE_DISTANCE_SENSOR 0
#endif

// --- BOARD SPECIFIC ---
#define MOTOR2040_MOTOR_A_P_PIN 4
#define MOTOR2040_MOTOR_A_N_PIN 5
#define MOTOR2040_MOTOR_B_P_PIN 6
#define MOTOR2040_MOTOR_B_N_PIN 7
#define MOTOR2040_MOTOR_C_P_PIN 8
#define MOTOR2040_MOTOR_C_N_PIN 9
#define MOTOR2040_MOTOR_D_P_PIN 10
#define MOTOR2040_MOTOR_D_N_PIN 11
#define MOTOR2040_NUM_MOTORS 4

#define MOTOR2040_ENCODER_A_A_PIN 0
#define MOTOR2040_ENCODER_A_B_PIN 1
#define MOTOR2040_ENCODER_B_A_PIN 2
#define MOTOR2040_ENCODER_B_B_PIN 3
#define MOTOR2040_ENCODER_C_A_PIN 12
#define MOTOR2040_ENCODER_C_B_PIN 13
#define MOTOR2040_ENCODER_D_A_PIN 14
#define MOTOR2040_ENCODER_D_B_PIN 15
#define MOTOR2040_NUM_ENCODERS 4

#if MOTOR2040_USE_DISTANCE_SENSOR
#define MOTOR2040_TRIG_PIN 16
#define MOTOR2040_ECHO_PIN 17
#else
#define MOTOR2040_TX_PIN 16
#define MOTOR2040_RX_PIN 17
#endif

#define MOTOR2040_LED_DATA_PIN 18
#define MOTOR2040_NUM_LEDS 1

#define MOTOR2040_I2C 0
#define MOTOR2040_INT_PIN 19
#define MOTOR2040_SDA_PIN 20
#define MOTOR2040_SCL_PIN 21

#define MOTOR2040_USER_SW_PIN 23
#define MOTOR2040_ADC_ADDR_0_PIN 22
#define MOTOR2040_ADC_ADDR_1_PIN 24
#define MOTOR2040_ADC_ADDR_2_PIN 25

#define MOTOR2040_ADC0_PIN 26
#define MOTOR2040_ADC1_PIN 27
#define MOTOR2040_ADC2_PIN 28
#define MOTOR2040_NUM_ADC_PINS 3

#define MOTOR2040_SHARED_ADC_PIN 29

#define MOTOR2040_CURRENT_SENSE_A_ADDR 0b000
#define MOTOR2040_CURRENT_SENSE_B_ADDR 0b001
#define MOTOR2040_CURRENT_SENSE_C_ADDR 0b010
#define MOTOR2040_CURRENT_SENSE_D_ADDR 0b011
#define MOTOR2040_VOLTAGE_SENSE_ADDR 0b100
#define MOTOR2040_FAULT_SENSE_ADDR 0b101
#define MOTOR2040_SENSOR_1_ADDR 0b110
#define MOTOR2040_SENSOR_2_ADDR 0b111
#define MOTOR2040_NUM_SENSORS 2

// --- UART ---
#if MOTOR2040_USE_DISTANCE_SENSOR
// no PICO_DEFAULT_UART
#else
#ifndef PICO_DEFAULT_UART
#define PICO_DEFAULT_UART 0
#endif
#ifndef PICO_DEFAULT_UART_TX_PIN
#define PICO_DEFAULT_UART_TX_PIN MOTOR2040_TX_PIN
#endif
#ifndef PICO_DEFAULT_UART_RX_PIN
#define PICO_DEFAULT_UART_RX_PIN MOTOR2040_RX_PIN
#endif
#endif

// --- LED ---
// no PICO_DEFAULT_LED_PIN
#ifndef PICO_DEFAULT_WS2812_PIN
#define PICO_DEFAULT_WS2812_PIN MOTOR2040_LED_DATA_PIN
#endif

// --- I2C ---
#ifndef PICO_DEFAULT_I2C
#define PICO_DEFAULT_I2C MOTOR2040_I2C
#endif
#ifndef PICO_DEFAULT_I2C_SDA_PIN
#define PICO_DEFAULT_I2C_SDA_PIN MOTOR2040_SDA_PIN
#endif
#ifndef PICO_DEFAULT_I2C_SCL_PIN
#define PICO_DEFAULT_I2C_SCL_PIN MOTOR2040_SCL_PIN
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

#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (2 * 1024 * 1024)
#endif

// All boards have B1 RP2040
#ifndef PICO_RP2040_B0_SUPPORTED
#define PICO_RP2040_B0_SUPPORTED 0
#endif

#endif
