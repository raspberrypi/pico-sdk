/*
 * Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------

// This header may be included by other board headers as "boards/pimoroni_blinky2350.h"

#ifndef _BOARDS_PIMORONI_BLINKY2350_H
#define _BOARDS_PIMORONI_BLINKY2350_H

pico_board_cmake_set(PICO_PLATFORM, rp2350)
pico_board_cmake_set(PICO_CYW43_SUPPORTED, 1)

// For board detection
#define PIMORONI_BLINKY2350

// --- BOARD SPECIFIC ---
// PCF85063a real time clock
#define BLINKY2350_RTC_I2C 0
#define BLINKY2350_RTC_I2C_ADDR 0x51
#define BLINKY2350_RTC_I2C_SDA_PIN 4
#define BLINKY2350_RTC_I2C_SCL_PIN 5
#define BLINKY2350_RTC_ALARM_PIN 13

#define BLINKY2350_SW_DOWN_PIN 6
#define BLINKY2350_SW_A_PIN 7
#define BLINKY2350_SW_B_PIN 9
#define BLINKY2350_SW_C_PIN 10
#define BLINKY2350_SW_UP_PIN 11
#define BLINKY2350_SW_HOME_PIN 22
#define BLINKY2350_SW_INT_PIN 15

// wired to the reset button, for long press detection
#define BLINKY2350_RESET_SW_PIN 14

// rear white LEDs
#define BLINKY2350_LED_0_PIN 0
#define BLINKY2350_LED_1_PIN 1
#define BLINKY2350_LED_2_PIN 2
#define BLINKY2350_LED_3_PIN 3

// LED matrix driver
#define BLINKY2350_COLUMN_CLOCK_PIN 16
#define BLINKY2350_COLUMN_DATA_PIN 17
#define BLINKY2350_COLUMN_LATCH_PIN 18
#define BLINKY2350_COLUMN_BLANK_PIN 19
#define BLINKY2350_ROW_DATA_PIN 20
#define BLINKY2350_ROW_DATA_CLOCK_PIN 21

#define BLINKY2350_PSRAM_CS_PIN 8

#define BLINKY2350_VBUS_DETECT_PIN 12

// switched power for the real time clock
#define BLINKY2350_SW_POWER_EN_PIN 27

#define BLINKY2350_VBAT_SENSE_PIN 26
#define BLINKY2350_SENSE_1V1_PIN 28

// --- RP2350 VARIANT ---
#define PICO_RP2350A 1

// --- UART ---
// no PICO_DEFAULT_UART
// no PICO_DEFAULT_UART_TX_PIN
// no PICO_DEFAULT_UART_RX_PIN

// --- LED ---
#ifndef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN BLINKY2350_LED_0_PIN
#endif
// no PICO_DEFAULT_WS2812_PIN

// --- I2C ---
#ifndef PICO_DEFAULT_I2C
#define PICO_DEFAULT_I2C BLINKY2350_RTC_I2C
#endif
#ifndef PICO_DEFAULT_I2C_SDA_PIN
#define PICO_DEFAULT_I2C_SDA_PIN BLINKY2350_RTC_I2C_SDA_PIN
#endif
#ifndef PICO_DEFAULT_I2C_SCL_PIN
#define PICO_DEFAULT_I2C_SCL_PIN BLINKY2350_RTC_I2C_SCL_PIN
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

// --- PSRAM ---
#ifndef PICO_PSRAM_CS_PIN
#define PICO_PSRAM_CS_PIN BLINKY2350_PSRAM_CS_PIN
#endif

pico_board_cmake_set_default(PICO_PSRAM_SIZE_BYTES, (8 * 1024 * 1024))
#ifndef PICO_PSRAM_SIZE_BYTES
#define PICO_PSRAM_SIZE_BYTES (8 * 1024 * 1024)
#endif

pico_board_cmake_set_default(PICO_RP2350_A2_SUPPORTED, 1)
#ifndef PICO_RP2350_A2_SUPPORTED
#define PICO_RP2350_A2_SUPPORTED 1
#endif

// no PICO_SMPS_MODE_PIN
// no PICO_VBUS_PIN
// no PICO_VSYS_PIN

// --- CYW43 ---

#ifndef CYW43_WL_GPIO_COUNT
#define CYW43_WL_GPIO_COUNT 3
#endif

// cyw43 SPI pins can't be changed at runtime
#ifndef CYW43_PIN_WL_DYNAMIC
#define CYW43_PIN_WL_DYNAMIC 0
#endif

// gpio pin to power up the cyw43 chip
#ifndef CYW43_DEFAULT_PIN_WL_REG_ON
#define CYW43_DEFAULT_PIN_WL_REG_ON 23u
#endif

// gpio pin for spi data out to the cyw43 chip
#ifndef CYW43_DEFAULT_PIN_WL_DATA_OUT
#define CYW43_DEFAULT_PIN_WL_DATA_OUT 24u
#endif

// gpio pin for spi data in from the cyw43 chip
#ifndef CYW43_DEFAULT_PIN_WL_DATA_IN
#define CYW43_DEFAULT_PIN_WL_DATA_IN 24u
#endif

// gpio (irq) pin for the irq line from the cyw43 chip
#ifndef CYW43_DEFAULT_PIN_WL_HOST_WAKE
#define CYW43_DEFAULT_PIN_WL_HOST_WAKE 24u
#endif

// gpio pin for the spi clock line to the cyw43 chip
#ifndef CYW43_DEFAULT_PIN_WL_CLOCK
#define CYW43_DEFAULT_PIN_WL_CLOCK 29u
#endif

// gpio pin for the spi chip select to the cyw43 chip
#ifndef CYW43_DEFAULT_PIN_WL_CS
#define CYW43_DEFAULT_PIN_WL_CS 25u
#endif

#endif
