/*
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------
//
//------------------------------------------------------------------------------------------
// Board definition for the SparkFun rp2040 MicroMod processor board
//
// This header may be included by other board headers as "boards/sparkfun_promicro.h"

#ifndef _BOARDS_SPARKFUN_MICROMOD_H
#define _BOARDS_SPARKFUN_MICROMOD_H

#define PICO_DEFAULT_UART 0

#define PICO_DEFAULT_UART_TX_PIN 0

#define PICO_DEFAULT_UART_RX_PIN 1

#define PICO_DEFAULT_LED_PIN 25


// Default I2C - for qwiic connector
#define PICO_DEFAULT_I2C_SDA   4
#define PICO_DEFAULT_I2C_SCL   5
#define PICO_DEFAULT_I2C_PORT  i2c0

// spi flash
#define PICO_FLASH_SPI_CLKDIV 2

#define PICO_FLASH_SIZE_BYTES (16 * 1024 * 1024)

//////////////////////////
// brining over from pico.h  - need to verify...

// Drive high to force power supply into PWM mode (lower ripple on 3V3 at light loads)
#define PICO_SMPS_MODE_PIN 23

#define PICO_FLOAT_SUPPORT_ROM_V1 1

#define PICO_DOUBLE_SUPPORT_ROM_V1 1

#endif
