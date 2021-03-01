/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_STDIO_USB_H
#define _PICO_STDIO_USB_H

#include "pico/stdio.h"

/** \brief Support for stdin/stdout over USB serial (CDC)
 *  \defgroup pico_stdio_usb pico_stdio_usb
 *  \ingroup pico_stdio
 *
 *  Linking this library or calling `pico_enable_stdio_usb(TARGET)` in the CMake (which
 *  achieves the same thing) will add USB CDC to the drivers used for standard output
 *
 *  Note this library is a developer convenience. It is not applicable in all cases; for one it takes full control of the USB device precluding your
 *  use of the USB in device or host mode. For this reason, this library will automatically disengage if you try to using it alongside \ref tinyusb_device or
 *  \ref tinyusb_host. It also takes control of a lower level IRQ and sets up a periodic background task.
 *
 *  This library also includes (by default) functionality to enable the RP2040 to be reset over the USB interface.
 */

// PICO_CONFIG: PICO_STDIO_USB_DEFAULT_CRLF, Default state of CR/LF translation for USB output, type=bool, default=PICO_STDIO_DEFAULT_CRLF, group=pico_stdio_usb
#ifndef PICO_STDIO_USB_DEFAULT_CRLF
#define PICO_STDIO_USB_DEFAULT_CRLF PICO_STDIO_DEFAULT_CRLF
#endif

// PICO_CONFIG: PICO_STDIO_USB_STDOUT_TIMEOUT_US, Number of microseconds to be blocked trying to write USB output before assuming the host has disappeared and discarding data, default=500000, group=pico_stdio_usb
#ifndef PICO_STDIO_USB_STDOUT_TIMEOUT_US
#define PICO_STDIO_USB_STDOUT_TIMEOUT_US 500000
#endif

// todo perhaps unnecessarily frequent?
// PICO_CONFIG: PICO_STDIO_USB_TASK_INTERVAL_US, Period of microseconds between calling tud_task in the background, default=1000, advanced=true, group=pico_stdio_usb
#ifndef PICO_STDIO_USB_TASK_INTERVAL_US
#define PICO_STDIO_USB_TASK_INTERVAL_US 1000
#endif

// PICO_CONFIG: PICO_STDIO_USB_LOW_PRIORITY_IRQ, low priority (non hardware) IRQ number to claim for tud_task() background execution, default=31, advanced=true, group=pico_stdio_usb
#ifndef PICO_STDIO_USB_LOW_PRIORITY_IRQ
#define PICO_STDIO_USB_LOW_PRIORITY_IRQ 31
#endif

// PICO_CONFIG: PICO_STDIO_USB_ENABLE_RESET_VIA_BAUD_RATE, Enable/disable resetting into BOOTSEL mode if the host sets the baud rate to a magic value (PICO_STDIO_USB_RESET_MAGIC_BAUD_RATE), type=bool, default=1, group=pico_stdio_usb
#ifndef PICO_STDIO_USB_ENABLE_RESET_VIA_BAUD_RATE
#define PICO_STDIO_USB_ENABLE_RESET_VIA_BAUD_RATE 1
#endif

// PICO_CONFIG: PICO_STDIO_USB_RESET_MAGIC_BAUD_RATE, baud rate that if selected causes a reset into BOOTSEL mode (if PICO_STDIO_USB_ENABLE_RESET_VIA_BAUD_RATE==1), default=1200, group=pico_stdio_usb
#ifndef PICO_STDIO_USB_RESET_MAGIC_BAUD_RATE
#define PICO_STDIO_USB_RESET_MAGIC_BAUD_RATE 1200
#endif

// PICO_CONFIG: PICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE, Enable/disable resetting into BOOTSEL mode via an additional VENDOR USB interface - enables picotool based reset, type=bool, default=1, group=pico_stdio_usb
#ifndef PICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE
#define PICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE 1
#endif

// PICO_CONFIG: PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_RESET_TO_BOOTSEL, If vendor reset interface is included allow rebooting to BOOTSEL mode, type=bool, default=1, group=pico_stdio_usb
#ifndef PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_RESET_TO_BOOTSEL
#define PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_RESET_TO_BOOTSEL 1
#endif

// PICO_CONFIG: PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_RESET_TO_FLASH_BOOT, If vendor reset interface is included allow rebooting with regular flash boot, type=bool, default=1, group=pico_stdio_usb
#ifndef PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_RESET_TO_FLASH_BOOT
#define PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_RESET_TO_FLASH_BOOT 1
#endif

// PICO_CONFIG: PICO_STDIO_USB_RESET_RESET_TO_FLASH_DELAY_MS, delays in ms before rebooting via regular flash boot, default=100, group=pico_stdio_usb
#ifndef PICO_STDIO_USB_RESET_RESET_TO_FLASH_DELAY_MS
#define PICO_STDIO_USB_RESET_RESET_TO_FLASH_DELAY_MS 100
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern stdio_driver_t stdio_usb;

/*! \brief Explicitly initialize USB stdio and add it to the current set of stdin drivers
 *  \ingroup pico_stdio_uart
 */
bool stdio_usb_init(void);

#ifdef __cplusplus
}
#endif

#endif
