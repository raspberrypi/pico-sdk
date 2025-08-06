/*
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_STDIO_USB_RESET_INTERFACE_H
#define _PICO_STDIO_USB_RESET_INTERFACE_H

// definitions have been moved here
#include "pico/usb_reset_interface.h"

#if PICO_ON_DEVICE
// PICO_CONFIG: PICO_STDIO_USB_RESET_BOOTSEL_ACTIVITY_LED, Optionally define a pin to use as bootloader activity LED when BOOTSEL mode is entered via USB (either VIA_BAUD_RATE or VIA_VENDOR_INTERFACE), type=int, min=0, max=47 on RP2350B, 29 otherwise, group=pico_stdio_usb

// PICO_CONFIG: PICO_STDIO_USB_RESET_BOOTSEL_ACTIVITY_LED_ACTIVE_LOW, Whether pin to use as bootloader activity LED when BOOTSEL mode is entered via USB (either VIA_BAUD_RATE or VIA_VENDOR_INTERFACE) is active low, type=bool, default=0, group=pico_stdio_usb
#ifndef PICO_STDIO_USB_RESET_BOOTSEL_ACTIVITY_LED_ACTIVE_LOW
#define PICO_STDIO_USB_RESET_BOOTSEL_ACTIVITY_LED_ACTIVE_LOW 0
#endif

// PICO_CONFIG: PICO_STDIO_USB_RESET_BOOTSEL_FIXED_ACTIVITY_LED, Whether the pin specified by PICO_STDIO_USB_RESET_BOOTSEL_ACTIVITY_LED is fixed or can be modified by picotool over the VENDOR USB interface, type=bool, default=0, group=pico_stdio_usb
#ifndef PICO_STDIO_USB_RESET_BOOTSEL_FIXED_ACTIVITY_LED
#define PICO_STDIO_USB_RESET_BOOTSEL_FIXED_ACTIVITY_LED 0
#endif

// Any modes disabled here can't be re-enabled by picotool via VENDOR_INTERFACE.
// PICO_CONFIG: PICO_STDIO_USB_RESET_BOOTSEL_INTERFACE_DISABLE_MASK, Optionally disable either the mass storage interface (bit 0) or the PICOBOOT interface (bit 1) when entering BOOTSEL mode via USB (either VIA_BAUD_RATE or VIA_VENDOR_INTERFACE), type=int, min=0, max=3, default=0, group=pico_stdio_usb
#ifndef PICO_STDIO_USB_RESET_BOOTSEL_INTERFACE_DISABLE_MASK
#define PICO_STDIO_USB_RESET_BOOTSEL_INTERFACE_DISABLE_MASK 0u
#endif

// PICO_CONFIG: PICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE, Enable/disable resetting into BOOTSEL mode via an additional VENDOR USB interface - enables picotool based reset, type=bool, default=1 if application is not using TinyUSB directly, group=pico_stdio_usb
#ifndef PICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE
#if !LIB_TINYUSB_HOST && !LIB_TINYUSB_DEVICE
#define PICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE 1
#else
#define PICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE 0
#endif
#endif

// PICO_CONFIG: PICO_STDIO_USB_RESET_INCLUDE_APP_DRIVER_CB, Set to 0 if your application defines usbd_app_driver_get_cb, type=bool, default=1, group=pico_stdio_usb
#ifndef PICO_STDIO_USB_RESET_INCLUDE_APP_DRIVER_CB
#define PICO_STDIO_USB_RESET_INCLUDE_APP_DRIVER_CB 1
#endif

// PICO_CONFIG: PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_RESET_TO_BOOTSEL, If vendor reset interface is included allow rebooting to BOOTSEL mode, type=bool, default=1, group=pico_stdio_usb
#ifndef PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_RESET_TO_BOOTSEL
#define PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_RESET_TO_BOOTSEL 1
#endif

// PICO_CONFIG: PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_RESET_TO_FLASH_BOOT, If vendor reset interface is included allow rebooting with regular flash boot, type=bool, default=1, group=pico_stdio_usb
#ifndef PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_RESET_TO_FLASH_BOOT
#define PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_RESET_TO_FLASH_BOOT 1
#endif

// PICO_CONFIG: PICO_STDIO_USB_RESET_RESET_TO_FLASH_DELAY_MS, Delay in ms before rebooting via regular flash boot, default=100, group=pico_stdio_usb
#ifndef PICO_STDIO_USB_RESET_RESET_TO_FLASH_DELAY_MS
#define PICO_STDIO_USB_RESET_RESET_TO_FLASH_DELAY_MS 100
#endif

// PICO_CONFIG: PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_MS_OS_20_DESCRIPTOR, If vendor reset interface is included add support for Microsoft OS 2.0 Descriptor, type=bool, default=1 when using default USB descriptors, 0 otherwise, group=pico_stdio_usb
#ifndef PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_MS_OS_20_DESCRIPTOR
#ifdef PICO_STDIO_USB_USE_DEFAULT_DESCRIPTORS
#define PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_MS_OS_20_DESCRIPTOR 1
#else
#define PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_MS_OS_20_DESCRIPTOR 0
#endif
#endif

// PICO_CONFIG: PICO_STDIO_USB_RESET_INTERFACE_MS_OS_20_DESCRIPTOR_ITF, If vendor reset interface is included the USB interface number for the reset interface, type=int, default=2 when using default USB descriptors, undefined otherwise, group=pico_stdio_usb
#ifndef PICO_STDIO_USB_RESET_INTERFACE_MS_OS_20_DESCRIPTOR_ITF
#if PICO_STDIO_USB_USE_DEFAULT_DESCRIPTORS
#define PICO_STDIO_USB_RESET_INTERFACE_MS_OS_20_DESCRIPTOR_ITF 2
#elif PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_MS_OS_20_DESCRIPTOR
#error Must set PICO_STDIO_USB_RESET_INTERFACE_MS_OS_20_DESCRIPTOR_ITF when using PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_MS_OS_20_DESCRIPTOR with custom USB descriptors
#endif
#endif

// Interface descriptor
#define TUD_RPI_RESET_DESC_LEN  9
#define TUD_RPI_RESET_DESCRIPTOR(_itfnum, _stridx) \
  /* Interface */\
  TUD_RPI_RESET_DESC_LEN, TUSB_DESC_INTERFACE, _itfnum, 0, 0, TUSB_CLASS_VENDOR_SPECIFIC, RESET_INTERFACE_SUBCLASS, RESET_INTERFACE_PROTOCOL, _stridx


#include "stdint.h"
#include "device/usbd_pvt.h"

void resetd_init(void);
void resetd_reset(uint8_t __unused rhport);
uint16_t resetd_open(uint8_t __unused rhport, tusb_desc_interface_t const *itf_desc, uint16_t max_len);
bool resetd_control_xfer_cb(uint8_t __unused rhport, uint8_t stage, tusb_control_request_t const * request);
bool resetd_xfer_cb(uint8_t __unused rhport, uint8_t __unused ep_addr, xfer_result_t __unused result, uint32_t __unused xferred_bytes);

static usbd_class_driver_t const _resetd_driver =
{
#if CFG_TUSB_DEBUG >= 2
    .name = "RESET",
#endif
    .init             = resetd_init,
    .reset            = resetd_reset,
    .open             = resetd_open,
    .control_xfer_cb  = resetd_control_xfer_cb,
    .xfer_cb          = resetd_xfer_cb,
    .sof              = NULL
};

#endif

#endif