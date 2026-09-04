/*
 * Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------

// This header may be included by other board headers as "boards/pimoroni_badger2350.h"

#ifndef _BOARDS_PIMORONI_BADGER2350_H
#define _BOARDS_PIMORONI_BADGER2350_H

pico_board_cmake_set(PICO_PLATFORM, rp2350)
pico_board_cmake_set(PICO_CYW43_SUPPORTED, 1)

// For board detection
#define PIMORONI_BADGER2350

// --- BOARD SPECIFIC ---
// PCF85063a real time clock
#define BADGER2350_RTC_I2C 0
#define BADGER2350_RTC_I2C_ADDR 0x51
#define BADGER2350_RTC_I2C_SDA_PIN 4
#define BADGER2350_RTC_I2C_SCL_PIN 5
#define BADGER2350_RTC_ALARM_PIN 13

#define BADGER2350_SW_DOWN_PIN 6
#define BADGER2350_SW_A_PIN 7
#define BADGER2350_SW_B_PIN 9
#define BADGER2350_SW_C_PIN 10
#define BADGER2350_SW_UP_PIN 11
#define BADGER2350_SW_HOME_PIN 22
#define BADGER2350_SW_INT_PIN 15

// wired to the reset button, for long press detection
#define BADGER2350_RESET_SW_PIN 14

// rear white LEDs
#define BADGER2350_LED_0_PIN 0
#define BADGER2350_LED_1_PIN 1
#define BADGER2350_LED_2_PIN 2
#define BADGER2350_LED_3_PIN 3

// SSD1680 e-paper display, write only so there is no MISO
#define BADGER2350_INKY_SPI 0
#define BADGER2350_INKY_BUSY_PIN 16
#define BADGER2350_INKY_CSN_PIN 17
#define BADGER2350_INKY_SCK_PIN 18
#define BADGER2350_INKY_MOSI_PIN 19
#define BADGER2350_INKY_DC_PIN 20
#define BADGER2350_INKY_RESET_PIN 21

#define BADGER2350_PSRAM_CS_PIN 8

#define BADGER2350_VBUS_DETECT_PIN 12

// switched power for the real time clock
#define BADGER2350_SW_POWER_EN_PIN 27

#define BADGER2350_VBAT_SENSE_PIN 26
#define BADGER2350_SENSE_1V1_PIN 28

// --- RP2350 VARIANT ---
#define PICO_RP2350A 1

// --- UART ---
// no PICO_DEFAULT_UART
// no PICO_DEFAULT_UART_TX_PIN
// no PICO_DEFAULT_UART_RX_PIN

// --- LED ---
#ifndef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN BADGER2350_LED_0_PIN
#endif
// no PICO_DEFAULT_WS2812_PIN

// --- I2C ---
#ifndef PICO_DEFAULT_I2C
#define PICO_DEFAULT_I2C BADGER2350_RTC_I2C
#endif
#ifndef PICO_DEFAULT_I2C_SDA_PIN
#define PICO_DEFAULT_I2C_SDA_PIN BADGER2350_RTC_I2C_SDA_PIN
#endif
#ifndef PICO_DEFAULT_I2C_SCL_PIN
#define PICO_DEFAULT_I2C_SCL_PIN BADGER2350_RTC_I2C_SCL_PIN
#endif

// --- SPI ---
#ifndef PICO_DEFAULT_SPI
#define PICO_DEFAULT_SPI BADGER2350_INKY_SPI
#endif
#ifndef PICO_DEFAULT_SPI_SCK_PIN
#define PICO_DEFAULT_SPI_SCK_PIN BADGER2350_INKY_SCK_PIN
#endif
#ifndef PICO_DEFAULT_SPI_TX_PIN
#define PICO_DEFAULT_SPI_TX_PIN BADGER2350_INKY_MOSI_PIN
#endif
// no PICO_DEFAULT_SPI_RX_PIN
#ifndef PICO_DEFAULT_SPI_CSN_PIN
#define PICO_DEFAULT_SPI_CSN_PIN BADGER2350_INKY_CSN_PIN
#endif

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
#define PICO_PSRAM_CS_PIN BADGER2350_PSRAM_CS_PIN
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
