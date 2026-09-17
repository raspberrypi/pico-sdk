/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_USB_RESET_CONFIG_H
#define _PICO_USB_RESET_CONFIG_H

// PICO_CONFIG: PICO_ENABLE_USB_RESET_VIA_BAUD_RATE, Enable/disable resetting into BOOTSEL mode if the host sets the baud rate to a magic value (PICO_USB_RESET_MAGIC_BAUD_RATE), type=bool, default=1 if application is not using TinyUSB directly, group=pico_usb_reset
#ifndef PICO_ENABLE_USB_RESET_VIA_BAUD_RATE
#ifdef PICO_STDIO_USB_ENABLE_RESET_VIA_BAUD_RATE // backwards compatibility with SDK <= 2.2.0
#define PICO_ENABLE_USB_RESET_VIA_BAUD_RATE PICO_STDIO_USB_ENABLE_RESET_VIA_BAUD_RATE
#elif !LIB_TINYUSB_HOST && !LIB_TINYUSB_DEVICE
#define PICO_ENABLE_USB_RESET_VIA_BAUD_RATE 1
#else
#define PICO_ENABLE_USB_RESET_VIA_BAUD_RATE 0
#endif
#endif

// PICO_CONFIG: PICO_USB_RESET_MAGIC_BAUD_RATE, Baud rate that if selected causes a reset into BOOTSEL mode (if PICO_ENABLE_USB_RESET_VIA_BAUD_RATE is set), default=1200, group=pico_usb_reset
#ifndef PICO_USB_RESET_MAGIC_BAUD_RATE
#ifdef PICO_STDIO_USB_RESET_MAGIC_BAUD_RATE // backwards compatibility with SDK <= 2.2.0
#define PICO_USB_RESET_MAGIC_BAUD_RATE PICO_STDIO_USB_RESET_MAGIC_BAUD_RATE
#else
#define PICO_USB_RESET_MAGIC_BAUD_RATE 1200
#endif
#endif

// PICO_CONFIG: PICO_ENABLE_USB_RESET_VIA_VENDOR_INTERFACE, Enable/disable resetting into BOOTSEL mode via an additional VENDOR USB interface - enables picotool based reset, type=bool, default=1 if application is not using TinyUSB directly, group=pico_usb_reset
#ifndef PICO_ENABLE_USB_RESET_VIA_VENDOR_INTERFACE
#ifdef PICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE // backwards compatibility with SDK <= 2.2.0
#define PICO_ENABLE_USB_RESET_VIA_VENDOR_INTERFACE PICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE
#elif !LIB_TINYUSB_HOST && !LIB_TINYUSB_DEVICE
#define PICO_ENABLE_USB_RESET_VIA_VENDOR_INTERFACE 1
#else
#define PICO_ENABLE_USB_RESET_VIA_VENDOR_INTERFACE 0
#endif
#endif

// PICO_CONFIG: PICO_USB_RESET_SUPPORT_MS_OS_20_DESCRIPTOR, If vendor reset interface is included add support for Microsoft OS 2.0 Descriptor, type=bool, default=1 if application is not using TinyUSB directly, 0 otherwise, group=pico_usb_reset
#ifndef PICO_USB_RESET_SUPPORT_MS_OS_20_DESCRIPTOR
#ifdef PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_MS_OS_20_DESCRIPTOR // backwards compatibility with SDK <= 2.2.0
#define PICO_USB_RESET_SUPPORT_MS_OS_20_DESCRIPTOR PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_MS_OS_20_DESCRIPTOR
#elif !LIB_TINYUSB_HOST && !LIB_TINYUSB_DEVICE
#define PICO_USB_RESET_SUPPORT_MS_OS_20_DESCRIPTOR 1
#else
#define PICO_USB_RESET_SUPPORT_MS_OS_20_DESCRIPTOR 0
#endif
#endif

// PICO_CONFIG: PICO_USB_RESET_MS_OS_20_DESCRIPTOR_ITF, If vendor reset interface is included this specifies the USB interface number for the reset interface, type=int, default=2 if application is not using TinyUSB directly, undefined otherwise, group=pico_usb_reset
#ifndef PICO_USB_RESET_MS_OS_20_DESCRIPTOR_ITF
#if !LIB_TINYUSB_HOST && !LIB_TINYUSB_DEVICE
#define PICO_USB_RESET_MS_OS_20_DESCRIPTOR_ITF 2
#elif PICO_USB_RESET_SUPPORT_MS_OS_20_DESCRIPTOR
#error Must set PICO_USB_RESET_MS_OS_20_DESCRIPTOR_ITF to the reset interface number when using PICO_USB_RESET_SUPPORT_MS_OS_20_DESCRIPTOR with custom USB descriptors
#endif
#endif

// PICO_CONFIG: PICO_USB_RESET_INCLUDE_DEFAULT_APP_DRIVER_CB, Set to 0 if your application defines its own usbd_app_driver_get_cb function, type=bool, default=1 when using pico_stdio_usb, 0 otherwise, group=pico_usb_reset
#ifndef PICO_USB_RESET_INCLUDE_DEFAULT_APP_DRIVER_CB
#define PICO_USB_RESET_INCLUDE_DEFAULT_APP_DRIVER_CB LIB_PICO_STDIO_USB
#endif

// PICO_CONFIG: PICO_USB_RESET_SUPPORT_RESET_TO_BOOTSEL, If vendor reset interface is included allow rebooting to BOOTSEL mode, type=bool, default=1, group=pico_usb_reset
#ifndef PICO_USB_RESET_SUPPORT_RESET_TO_BOOTSEL
#ifdef PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_RESET_TO_BOOTSEL // backwards compatibility with SDK <= 2.2.0
#define PICO_USB_RESET_SUPPORT_RESET_TO_BOOTSEL PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_RESET_TO_BOOTSEL
#else
#define PICO_USB_RESET_SUPPORT_RESET_TO_BOOTSEL 1
#endif
#endif

// PICO_CONFIG: PICO_USB_RESET_BOOTSEL_ACTIVITY_LED, Optionally define a pin to use as bootloader activity LED when BOOTSEL mode is entered via USB (either VIA_BAUD_RATE or VIA_VENDOR_INTERFACE), type=int, min=0, max=47 on RP2350B, 29 otherwise, group=pico_usb_reset
#ifdef PICO_STDIO_USB_RESET_BOOTSEL_ACTIVITY_LED // backwards compatibility with SDK <= 2.2.0
#define PICO_USB_RESET_BOOTSEL_ACTIVITY_LED PICO_STDIO_USB_RESET_BOOTSEL_ACTIVITY_LED
#endif // default is undefined

// PICO_CONFIG: PICO_USB_RESET_BOOTSEL_ACTIVITY_LED_ACTIVE_LOW, Whether pin to use as bootloader activity LED when BOOTSEL mode is entered via USB (either VIA_BAUD_RATE or VIA_VENDOR_INTERFACE) is active low (ignored on RP2040), type=bool, default=0, group=pico_usb_reset
#ifndef PICO_USB_RESET_BOOTSEL_ACTIVITY_LED_ACTIVE_LOW
#ifdef PICO_STDIO_USB_RESET_BOOTSEL_ACTIVITY_LED_ACTIVE_LOW // backwards compatibility with SDK <= 2.2.0
#define PICO_USB_RESET_BOOTSEL_ACTIVITY_LED_ACTIVE_LOW PICO_STDIO_USB_RESET_BOOTSEL_ACTIVITY_LED_ACTIVE_LOW
#else
#define PICO_USB_RESET_BOOTSEL_ACTIVITY_LED_ACTIVE_LOW 0
#endif
#endif

// PICO_CONFIG: PICO_USB_RESET_BOOTSEL_FIXED_ACTIVITY_LED, Whether the pin specified by PICO_USB_RESET_BOOTSEL_ACTIVITY_LED is fixed or can be modified by picotool over the VENDOR USB interface, type=bool, default=0, group=pico_usb_reset
#ifndef PICO_USB_RESET_BOOTSEL_FIXED_ACTIVITY_LED
#ifdef PICO_STDIO_USB_RESET_BOOTSEL_FIXED_ACTIVITY_LED // backwards compatibility with SDK <= 2.2.0
#define PICO_USB_RESET_BOOTSEL_FIXED_ACTIVITY_LED PICO_STDIO_USB_RESET_BOOTSEL_FIXED_ACTIVITY_LED
#else
#define PICO_USB_RESET_BOOTSEL_FIXED_ACTIVITY_LED 0
#endif
#endif

// Any modes disabled here can't be re-enabled by picotool via VENDOR_INTERFACE.
// PICO_CONFIG: PICO_USB_RESET_BOOTSEL_INTERFACE_DISABLE_MASK, Optionally disable either the mass storage interface (bit 0) or the PICOBOOT interface (bit 1) when entering BOOTSEL mode via USB (either VIA_BAUD_RATE or VIA_VENDOR_INTERFACE), type=int, min=0, max=3, default=0, group=pico_usb_reset
#ifndef PICO_USB_RESET_BOOTSEL_INTERFACE_DISABLE_MASK
#ifdef PICO_STDIO_USB_RESET_BOOTSEL_INTERFACE_DISABLE_MASK // backwards compatibility with SDK <= 2.2.0
#define PICO_USB_RESET_BOOTSEL_INTERFACE_DISABLE_MASK PICO_STDIO_USB_RESET_BOOTSEL_INTERFACE_DISABLE_MASK
#else
#define PICO_USB_RESET_BOOTSEL_INTERFACE_DISABLE_MASK 0u
#endif
#endif

// PICO_CONFIG: PICO_USB_RESET_SUPPORT_RESET_TO_FLASH_BOOT, If vendor reset interface is included allow rebooting with regular flash boot, type=bool, default=1, group=pico_usb_reset
#ifndef PICO_USB_RESET_SUPPORT_RESET_TO_FLASH_BOOT
#ifdef PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_RESET_TO_FLASH_BOOT // backwards compatibility with SDK <= 2.2.0
#define PICO_USB_RESET_SUPPORT_RESET_TO_FLASH_BOOT PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_RESET_TO_FLASH_BOOT
#else
#define PICO_USB_RESET_SUPPORT_RESET_TO_FLASH_BOOT 1
#endif
#endif

// PICO_CONFIG: PICO_USB_RESET_RESET_TO_FLASH_DELAY_MS, Delay in ms before rebooting via regular flash boot, default=100, group=pico_usb_reset
#ifndef PICO_USB_RESET_RESET_TO_FLASH_DELAY_MS
#ifdef PICO_STDIO_USB_RESET_RESET_TO_FLASH_DELAY_MS // backwards compatibility with SDK <= 2.2.0
#define PICO_USB_RESET_RESET_TO_FLASH_DELAY_MS PICO_STDIO_USB_RESET_RESET_TO_FLASH_DELAY_MS
#else
#define PICO_USB_RESET_RESET_TO_FLASH_DELAY_MS 100
#endif
#endif

#endif