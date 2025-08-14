/*
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_USB_RESET_INTERFACE_H
#define _PICO_USB_RESET_INTERFACE_H

/** \file usb_reset_interface.h
 *  \defgroup pico_usb_reset_interface pico_usb_reset_interface
 *
 * \brief Functionality to enable the RP-series microcontroller to be reset over the USB interface.
 *
 * This library can be used to enable the RP-series microcontroller to be reset over the USB interface.
 *
 * This functionality is included by default when using the `pico_stdio_usb` library and not using TinyUSB directly.
 *
 * To add this functionality to a project using TinyUSB directly, you need to:
 * 1. Link the pico_usb_reset_interface library, and include the `pico/usb_reset_interface.h` header file where needed.
 * 2. Define PICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE=1
 * 3. Add `TUD_RPI_RESET_DESCRIPTOR(<ITF_NUM>, <STR_IDX>)` to your USB descriptors (length is `TUD_RPI_RESET_DESC_LEN`)
 * 4. Check if your project has an existing `usbd_app_driver_get_cb` function:
 *    - If it does, you need to add the `_resetd_driver` to the drivers returned
 *    - If it does not, and you aren't using the `pico_stdio_usb` library, you need to define `PICO_STDIO_USB_RESET_INCLUDE_APP_DRIVER_CB=1`
 * 5. Check if your project has an existing Microsoft OS 2.0 Descriptor:
 *    - If it does, you need to add the Function Subset header `RPI_RESET_MS_OS_20_DESCRIPTOR(<ITF_NUM>)` to your Microsoft OS 2.0 Descriptor (length is `RPI_RESET_MS_OS_20_DESC_LEN`)
 *    - If it does not, you need to define `PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_MS_OS_20_DESCRIPTOR=1` and `PICO_STDIO_USB_RESET_INTERFACE_MS_OS_20_DESCRIPTOR_ITF=<ITF_NUM>`
 */

// These defines are used by picotool
// VENDOR sub-class for the reset interface
#define RESET_INTERFACE_SUBCLASS 0x00
// VENDOR protocol for the reset interface
#define RESET_INTERFACE_PROTOCOL 0x01

// CONTROL requests:

// reset to BOOTSEL
#define RESET_REQUEST_BOOTSEL 0x01
// regular flash boot
#define RESET_REQUEST_FLASH 0x02

#if LIB_PICO_USB_RESET_INTERFACE
// These defines are only used by the pico_usb_reset_interface library, not the pico_usb_reset_interface_headers library

// PICO_CONFIG: PICO_STDIO_USB_RESET_BOOTSEL_ACTIVITY_LED, Optionally define a pin to use as bootloader activity LED when BOOTSEL mode is entered via USB (either VIA_BAUD_RATE or VIA_VENDOR_INTERFACE), type=int, min=0, max=47 on RP2350B, 29 otherwise, group=pico_usb_reset_interface

// PICO_CONFIG: PICO_STDIO_USB_RESET_BOOTSEL_ACTIVITY_LED_ACTIVE_LOW, Whether pin to use as bootloader activity LED when BOOTSEL mode is entered via USB (either VIA_BAUD_RATE or VIA_VENDOR_INTERFACE) is active low, type=bool, default=0, group=pico_usb_reset_interface
#ifndef PICO_STDIO_USB_RESET_BOOTSEL_ACTIVITY_LED_ACTIVE_LOW
#define PICO_STDIO_USB_RESET_BOOTSEL_ACTIVITY_LED_ACTIVE_LOW 0
#endif

// PICO_CONFIG: PICO_STDIO_USB_RESET_BOOTSEL_FIXED_ACTIVITY_LED, Whether the pin specified by PICO_STDIO_USB_RESET_BOOTSEL_ACTIVITY_LED is fixed or can be modified by picotool over the VENDOR USB interface, type=bool, default=0, group=pico_usb_reset_interface
#ifndef PICO_STDIO_USB_RESET_BOOTSEL_FIXED_ACTIVITY_LED
#define PICO_STDIO_USB_RESET_BOOTSEL_FIXED_ACTIVITY_LED 0
#endif

// Any modes disabled here can't be re-enabled by picotool via VENDOR_INTERFACE.
// PICO_CONFIG: PICO_STDIO_USB_RESET_BOOTSEL_INTERFACE_DISABLE_MASK, Optionally disable either the mass storage interface (bit 0) or the PICOBOOT interface (bit 1) when entering BOOTSEL mode via USB (either VIA_BAUD_RATE or VIA_VENDOR_INTERFACE), type=int, min=0, max=3, default=0, group=pico_usb_reset_interface
#ifndef PICO_STDIO_USB_RESET_BOOTSEL_INTERFACE_DISABLE_MASK
#define PICO_STDIO_USB_RESET_BOOTSEL_INTERFACE_DISABLE_MASK 0u
#endif

// PICO_CONFIG: PICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE, Enable/disable resetting into BOOTSEL mode via an additional VENDOR USB interface - enables picotool based reset, type=bool, default=1 if application is not using TinyUSB directly, group=pico_usb_reset_interface
#ifndef PICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE
#if !LIB_TINYUSB_HOST && !LIB_TINYUSB_DEVICE
#define PICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE 1
#else
#define PICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE 0
#endif
#endif

// PICO_CONFIG: PICO_STDIO_USB_RESET_INCLUDE_APP_DRIVER_CB, Set to 0 if your application defines usbd_app_driver_get_cb, type=bool, default=1 when using pico_usb_reset_interface, 0 otherwise, group=pico_usb_reset_interface
#ifndef PICO_STDIO_USB_RESET_INCLUDE_APP_DRIVER_CB
#define PICO_STDIO_USB_RESET_INCLUDE_APP_DRIVER_CB LIB_PICO_STDIO_USB
#endif

// PICO_CONFIG: PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_RESET_TO_BOOTSEL, If vendor reset interface is included allow rebooting to BOOTSEL mode, type=bool, default=1, group=pico_usb_reset_interface
#ifndef PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_RESET_TO_BOOTSEL
#define PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_RESET_TO_BOOTSEL 1
#endif

// PICO_CONFIG: PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_RESET_TO_FLASH_BOOT, If vendor reset interface is included allow rebooting with regular flash boot, type=bool, default=1, group=pico_usb_reset_interface
#ifndef PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_RESET_TO_FLASH_BOOT
#define PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_RESET_TO_FLASH_BOOT 1
#endif

// PICO_CONFIG: PICO_STDIO_USB_RESET_RESET_TO_FLASH_DELAY_MS, Delay in ms before rebooting via regular flash boot, default=100, group=pico_usb_reset_interface
#ifndef PICO_STDIO_USB_RESET_RESET_TO_FLASH_DELAY_MS
#define PICO_STDIO_USB_RESET_RESET_TO_FLASH_DELAY_MS 100
#endif

// PICO_CONFIG: PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_MS_OS_20_DESCRIPTOR, If vendor reset interface is included add support for Microsoft OS 2.0 Descriptor, type=bool, default=1 if application is not using TinyUSB directly, 0 otherwise, group=pico_usb_reset_interface
#ifndef PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_MS_OS_20_DESCRIPTOR
#if !LIB_TINYUSB_HOST && !LIB_TINYUSB_DEVICE
#define PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_MS_OS_20_DESCRIPTOR 1
#else
#define PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_MS_OS_20_DESCRIPTOR 0
#endif
#endif

// PICO_CONFIG: PICO_STDIO_USB_RESET_INTERFACE_MS_OS_20_DESCRIPTOR_ITF, If vendor reset interface is included the USB interface number for the reset interface, type=int, default=2 if application is not using TinyUSB directly, undefined otherwise, group=pico_usb_reset_interface
#ifndef PICO_STDIO_USB_RESET_INTERFACE_MS_OS_20_DESCRIPTOR_ITF
#if !LIB_TINYUSB_HOST && !LIB_TINYUSB_DEVICE
#define PICO_STDIO_USB_RESET_INTERFACE_MS_OS_20_DESCRIPTOR_ITF 2
#elif PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_MS_OS_20_DESCRIPTOR
#error Must set PICO_STDIO_USB_RESET_INTERFACE_MS_OS_20_DESCRIPTOR_ITF to the reset interface number when using PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_MS_OS_20_DESCRIPTOR with custom USB descriptors
#endif
#endif

// Interface descriptor
#define TUD_RPI_RESET_DESC_LEN  9
#define TUD_RPI_RESET_DESCRIPTOR(_itfnum, _stridx) \
  /* Interface */\
  TUD_RPI_RESET_DESC_LEN, TUSB_DESC_INTERFACE, _itfnum, 0, 0, TUSB_CLASS_VENDOR_SPECIFIC, RESET_INTERFACE_SUBCLASS, RESET_INTERFACE_PROTOCOL, _stridx


// Microsoft OS 2.0 Descriptor
#define RPI_RESET_MS_OS_20_DESC_LEN (0x08 + 0x14 + 0x80)
#define RPI_RESET_MS_OS_20_DESCRIPTOR(itf_num) \
  /* Function Subset header: length, type, first interface, reserved, subset length */ \
  U16_TO_U8S_LE(0x0008), U16_TO_U8S_LE(MS_OS_20_SUBSET_HEADER_FUNCTION), itf_num, 0, U16_TO_U8S_LE(RPI_RESET_MS_OS_20_DESC_LEN), \
  \
  /* MS OS 2.0 Compatible ID descriptor: length, type, compatible ID, sub compatible ID */ \
  U16_TO_U8S_LE(0x0014), U16_TO_U8S_LE(MS_OS_20_FEATURE_COMPATBLE_ID), 'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00, \
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* sub-compatible */ \
  \
  /* MS OS 2.0 Registry property descriptor: length, type */ \
  U16_TO_U8S_LE(0x0080), U16_TO_U8S_LE(MS_OS_20_FEATURE_REG_PROPERTY), \
  U16_TO_U8S_LE(0x0001), U16_TO_U8S_LE(0x0028), /* wPropertyDataType, wPropertyNameLength and PropertyName "DeviceInterfaceGUID" in UTF-16 */ \
  'D', 0x00, 'e', 0x00, 'v', 0x00, 'i', 0x00, 'c', 0x00, 'e', 0x00, 'I', 0x00, 'n', 0x00, 't', 0x00, 'e', 0x00, \
  'r', 0x00, 'f', 0x00, 'a', 0x00, 'c', 0x00, 'e', 0x00, 'G', 0x00, 'U', 0x00, 'I', 0x00, 'D', 0x00, 0x00, 0x00, \
  U16_TO_U8S_LE(0x004E), /* wPropertyDataLength */ \
  /* Vendor-defined Property Data: {bc7398c1-73cd-4cb7-98b8-913a8fca7bf6} */ \
  '{', 0,     'b', 0,     'c', 0,     '7', 0,     '3', 0,     '9', 0, \
  '8', 0,     'c', 0,     '1', 0,     '-', 0,     '7', 0,     '3', 0, \
  'c', 0,     'd', 0,     '-', 0,     '4', 0,     'c', 0,     'b', 0, \
  '7', 0,     '-', 0,     '9', 0,     '8', 0,     'b', 0,     '8', 0, \
  '-', 0,     '9', 0,     '1', 0,     '3', 0,     'a', 0,     '8', 0, \
  'f', 0,     'c', 0,     'a', 0,     '7', 0,     'b', 0,     'f', 0, \
  '6', 0,     '}', 0,       0, 0

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