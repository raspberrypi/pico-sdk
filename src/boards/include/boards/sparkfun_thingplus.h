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
// Board definition for the SparkFun rp2040 Thing Plus
//
// This header may be included by other board headers as "boards/sparkfun_thingplus.h"

#ifndef _BOARDS_SPARKFUN_THINGPLUS_H
#define _BOARDS_SPARKFUN_THINGPLUS_H

#define PICO_DEFAULT_UART 0

#define PICO_DEFAULT_UART_TX_PIN 0

#define PICO_DEFAULT_UART_RX_PIN 1

#define PICO_DEFAULT_LED_PIN 25


// Default I2C - for qwiic connector
#define PICO_DEFAULT_I2C_SDA   6
#define PICO_DEFAULT_I2C_SCL   7
#define PICO_DEFAULT_I2C_PORT  i2c1

// spi flash
#define PICO_FLASH_SPI_CLKDIV 2
#define PICO_FLASH_SIZE_BYTES (16 * 1024 * 1024)

// The thing plus has a SD Card. 
//
// Note: The current configuration for the PICO boards with SD support 
// 	     like the vga board is to define the DATA0 pin, and the SD 
//       support library counts up (DATA1 = DATA0++, ..etc). The Thing Plus
//       has the pins in reverse order. So the SD card support in the pico SDK
//       needs updating. Defining all pins in this file for now. 

#define PICO_SD_CLK_PIN   14
#define PICO_SD_CMD_PIN   15
#define PICO_SD_DATA0_PIN 12
#define PICO_SD_DATA1_PIN 11
#define PICO_SD_DATA2_PIN 10
#define PICO_SD_DATA3_PIN 09

//////////////////////////
// brining over from pico.h 

// Drive high to force power supply into PWM mode (lower ripple on 3V3 at light loads)
#define PICO_SMPS_MODE_PIN 23

#define PICO_FLOAT_SUPPORT_ROM_V1 1

#define PICO_DOUBLE_SUPPORT_ROM_V1 1

#endif
