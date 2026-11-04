/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------


#ifndef _BOARDS_CYTRON_MOTION_2350_PRO_H
#define _BOARDS_CYTRON_MOTION_2350_PRO_H


pico_board_cmake_set(PICO_PLATFORM, rp2350)

// For board detection
#define CYTRON_MOTION_2350_PRO

// --- RP2350 VARIANT ---
#define PICO_RP2350A 1


// --- BOARD SPECIFIC ---

// Motor drivers
#ifndef MOTION_2350_PRO_M1A_PIN
#define MOTION_2350_PRO_M1A_PIN 8
#endif

#ifndef MOTION_2350_PRO_M1B_PIN
#define MOTION_2350_PRO_M1B_PIN 9
#endif

#ifndef MOTION_2350_PRO_M2A_PIN
#define MOTION_2350_PRO_M2A_PIN 10
#endif

#ifndef MOTION_2350_PRO_M2B_PIN
#define MOTION_2350_PRO_M2B_PIN 11
#endif

#ifndef MOTION_2350_PRO_M3A_PIN
#define MOTION_2350_PRO_M3A_PIN 12
#endif

#ifndef MOTION_2350_PRO_M3B_PIN
#define MOTION_2350_PRO_M3B_PIN 13
#endif

#ifndef MOTION_2350_PRO_M4A_PIN
#define MOTION_2350_PRO_M4A_PIN 14
#endif

#ifndef MOTION_2350_PRO_M4B_PIN
#define MOTION_2350_PRO_M4B_PIN 15
#endif


// --- Buttons ---
#ifndef MOTION_2350_PRO_BUTTON1_PIN
#define MOTION_2350_PRO_BUTTON1_PIN 20
#endif

#ifndef MOTION_2350_PRO_BUTTON2_PIN
#define MOTION_2350_PRO_BUTTON2_PIN 21
#endif


// --- PIO USB ---

#define MOTION_2350_PRO_HOST_DATA_PLUS_PIN 24
#define MOTION_2350_PRO_HOST_DATA_MINUS_PIN 25

#ifndef PICO_DEFAULT_PIO_USB_DP_PIN
#define PICO_DEFAULT_PIO_USB_DP_PIN MOTION_2350_PRO_HOST_DATA_PLUS_PIN
#endif

#ifndef PICO_DEFAULT_PIO_USB_DM_PIN
#define PICO_DEFAULT_PIO_USB_DM_PIN MOTION_2350_PRO_HOST_DATA_MINUS_PIN
#endif


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


#define MOTION_2350_PRO_BUZZER_PIN 22

// --- LED ---
#ifndef PICO_DEFAULT_WS2812_PIN
#define PICO_DEFAULT_WS2812_PIN 23
#endif

// This board has two WS2812 leds in series
#ifndef PICO_DEFAULT_WS2812_NUM_PIXELS
#define PICO_DEFAULT_WS2812_NUM_PIXELS 2
#endif


// --- I2C ---
#ifndef PICO_DEFAULT_I2C
#define PICO_DEFAULT_I2C 0
#endif
#ifndef PICO_DEFAULT_I2C_SDA_PIN
#define PICO_DEFAULT_I2C_SDA_PIN 4
#endif
#ifndef PICO_DEFAULT_I2C_SCL_PIN
#define PICO_DEFAULT_I2C_SCL_PIN 5
#endif

// --- SPI ---
#ifndef PICO_DEFAULT_SPI
#define PICO_DEFAULT_SPI 0
#endif
#ifndef PICO_DEFAULT_SPI_SCK_PIN
#define PICO_DEFAULT_SPI_SCK_PIN 18
#endif
#ifndef PICO_DEFAULT_SPI_TX_PIN
#define PICO_DEFAULT_SPI_TX_PIN 19
#endif
#ifndef PICO_DEFAULT_SPI_RX_PIN
#define PICO_DEFAULT_SPI_RX_PIN 16
#endif
#ifndef PICO_DEFAULT_SPI_CSN_PIN
#define PICO_DEFAULT_SPI_CSN_PIN 17
#endif

// --- FLASH ---

#define PICO_BOOT_STAGE2_CHOOSE_W25Q080 1

#ifndef PICO_FLASH_SPI_CLKDIV
#define PICO_FLASH_SPI_CLKDIV 2
#endif

pico_board_cmake_set_default(PICO_FLASH_SIZE_BYTES, (2 * 1024 * 1024))
#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (2 * 1024 * 1024)
#endif



// The GPIO Pin used to monitor VSYS. Typically you would use this with ADC.
// Optional Solder jumper, VSYS = ADC*6.1
#ifndef PICO_VSYS_PIN
#define PICO_VSYS_PIN 29
#endif

// Old variants still use A2
pico_board_cmake_set_default(PICO_RP2350_A2_SUPPORTED, 1)
#ifndef PICO_RP2350_A2_SUPPORTED
#define PICO_RP2350_A2_SUPPORTED 1
#endif

#endif
