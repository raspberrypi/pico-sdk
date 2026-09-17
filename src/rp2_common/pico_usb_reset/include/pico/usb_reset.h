/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_USB_RESET_H
#define _PICO_USB_RESET_H

/** \file usb_reset.h
 *  \defgroup pico_usb_reset pico_usb_reset
 *
 * \brief Functionality to enable the RP-series microcontroller to be reset over the USB interface.
 *
 * This library can be used to enable the RP-series microcontroller to be reset over the USB interface.
 *
 * This functionality is included by default when using the `pico_stdio_usb` library and not using TinyUSB directly.
 *
 * To add this functionality to a project using TinyUSB directly, you need to:
 * 1. Link the pico_usb_reset library, and include the `pico/usb_reset.h` header file where needed.
 * 2. Define PICO_ENABLE_USB_RESET_VIA_VENDOR_INTERFACE=1
 * 3. Add `TUD_RPI_RESET_DESCRIPTOR(<ITF_NUM>, <STR_IDX>)` to your USB descriptors (length is `TUD_RPI_RESET_DESC_LEN`)
 * 4. Check if your project has an existing `usbd_app_driver_get_cb` function:
 *    - If it does, you need to add the `usb_reset_interface_driver` to the drivers returned
 *    - If it does not, and you aren't using the `pico_stdio_usb` library, you need to define `PICO_USB_RESET_INCLUDE_DEFAULT_APP_DRIVER_CB=1`
 * 5. Check if your project has an existing Microsoft OS 2.0 Descriptor:
 *    - If it does, you need to add the Function Subset header `RPI_RESET_MS_OS_20_DESCRIPTOR(<ITF_NUM>)` to your Microsoft OS 2.0 Descriptor (length is `RPI_RESET_MS_OS_20_DESC_LEN`)
 *    - If it does not, you need to define `PICO_USB_RESET_SUPPORT_MS_OS_20_DESCRIPTOR=1` and `PICO_USB_RESET_MS_OS_20_DESCRIPTOR_ITF=<ITF_NUM>`
 */

#include "pico/usb_reset_interface.h"
#include "pico/usb_reset_config.h"
#include "pico/usb_reset_tusb.h"

#endif