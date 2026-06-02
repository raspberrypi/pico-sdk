/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_USB_RESET_TUSB_H
#define _PICO_USB_RESET_TUSB_H

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

void usb_reset_interface_init(void);
void usb_reset_interface_reset(uint8_t __unused rhport);
uint16_t usb_reset_interface_open(uint8_t __unused rhport, tusb_desc_interface_t const *itf_desc, uint16_t max_len);
bool usb_reset_interface_control_xfer_cb(uint8_t __unused rhport, uint8_t stage, tusb_control_request_t const * request);
bool usb_reset_interface_xfer_cb(uint8_t __unused rhport, uint8_t __unused ep_addr, xfer_result_t __unused result, uint32_t __unused xferred_bytes);

static usbd_class_driver_t const usb_reset_interface_driver =
{
#if CFG_TUSB_DEBUG >= 2
    .name = "RESET",
#endif
    .init             = usb_reset_interface_init,
    .reset            = usb_reset_interface_reset,
    .open             = usb_reset_interface_open,
    .control_xfer_cb  = usb_reset_interface_control_xfer_cb,
    .xfer_cb          = usb_reset_interface_xfer_cb,
    .sof              = NULL
};

#endif