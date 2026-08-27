/*
 * Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------

// This header may be included by other board headers as "boards/pimoroni_tufty2350.h"

#ifndef _BOARDS_PIMORONI_TUFTY2350_H
#define _BOARDS_PIMORONI_TUFTY2350_H

pico_board_cmake_set(PICO_PLATFORM, rp2350)
pico_board_cmake_set(PICO_CYW43_SUPPORTED, 1)

// For board detection
#define PIMORONI_TUFTY2350

// --- BOARD SPECIFIC ---
// PCF85063a real time clock
#define TUFTY2350_RTC_I2C 0
#define TUFTY2350_RTC_I2C_ADDR 0x51
#define TUFTY2350_RTC_I2C_SDA_PIN 4
#define TUFTY2350_RTC_I2C_SCL_PIN 5
#define TUFTY2350_RTC_ALARM_PIN 13

#define TUFTY2350_SW_DOWN_PIN 6
#define TUFTY2350_SW_A_PIN 7
#define TUFTY2350_SW_B_PIN 9
#define TUFTY2350_SW_C_PIN 10
#define TUFTY2350_SW_UP_PIN 11
#define TUFTY2350_SW_HOME_PIN 22
#define TUFTY2350_SW_INT_PIN 15

// wired to the reset button, for long press detection
#define TUFTY2350_RESET_SW_PIN 14

// rear white LEDs
#define TUFTY2350_LED_0_PIN 0
#define TUFTY2350_LED_1_PIN 1
#define TUFTY2350_LED_2_PIN 2
#define TUFTY2350_LED_3_PIN 3

// 8 bit parallel display interface, data on GPIO 32 through 39
#define TUFTY2350_LCD_TE_PIN 21
#define TUFTY2350_BACKLIGHT_PIN 26
#define TUFTY2350_LCD_CS_PIN 27
#define TUFTY2350_LCD_DC_PIN 28
#define TUFTY2350_LCD_WR_PIN 30
#define TUFTY2350_LCD_RD_PIN 31
#define TUFTY2350_LCD_D0_PIN 32
#define TUFTY2350_LCD_D1_PIN 33
#define TUFTY2350_LCD_D2_PIN 34
#define TUFTY2350_LCD_D3_PIN 35
#define TUFTY2350_LCD_D4_PIN 36
#define TUFTY2350_LCD_D5_PIN 37
#define TUFTY2350_LCD_D6_PIN 38
#define TUFTY2350_LCD_D7_PIN 39

#define TUFTY2350_PSRAM_CS_PIN 8

#define TUFTY2350_VBUS_DETECT_PIN 12

// switched power for the real time clock
#define TUFTY2350_SW_POWER_EN_PIN 41

#define TUFTY2350_VBAT_SENSE_PIN 40
#define TUFTY2350_SENSE_1V1_PIN 42
#define TUFTY2350_LIGHT_SENSE_PIN 43

// --- RP2350 VARIANT ---
#define PICO_RP2350A 0

// --- UART ---
// no PICO_DEFAULT_UART
// no PICO_DEFAULT_UART_TX_PIN
// no PICO_DEFAULT_UART_RX_PIN

// --- LED ---
// no PICO_DEFAULT_LED_PIN - LED is on the wireless chip
// no PICO_DEFAULT_WS2812_PIN

// --- I2C ---
#ifndef PICO_DEFAULT_I2C
#define PICO_DEFAULT_I2C TUFTY2350_RTC_I2C
#endif
#ifndef PICO_DEFAULT_I2C_SDA_PIN
#define PICO_DEFAULT_I2C_SDA_PIN TUFTY2350_RTC_I2C_SDA_PIN
#endif
#ifndef PICO_DEFAULT_I2C_SCL_PIN
#define PICO_DEFAULT_I2C_SCL_PIN TUFTY2350_RTC_I2C_SCL_PIN
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
#define PICO_PSRAM_CS_PIN TUFTY2350_PSRAM_CS_PIN
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
