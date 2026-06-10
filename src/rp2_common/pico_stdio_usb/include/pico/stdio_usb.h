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
 *  Linking this library or calling `pico_enable_stdio_usb(TARGET ENABLED)` in the CMake (which
 *  achieves the same thing) will add USB CDC to the drivers used for standard input/output
 *
 *  Note this library is a developer convenience. It is not applicable in all cases; for one it takes full control of the USB device precluding your
 *  use of the USB in device or host mode. For this reason, this library will automatically disengage if you try to using it alongside \ref tinyusb_device or
 *  \ref tinyusb_host. It also takes control of a lower level IRQ and sets up a periodic background task.
 *
 *  This library also includes (by default) functionality to enable the RP-series microcontroller to be reset over the USB interface.
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

// PICO_CONFIG: PICO_STDIO_USB_LOW_PRIORITY_IRQ, Explicit User IRQ number to claim for tud_task() background execution instead of letting the implementation pick a free one dynamically (deprecated), advanced=true, group=pico_stdio_usb
#ifndef PICO_STDIO_USB_LOW_PRIORITY_IRQ
// this variable is no longer set by default (one is claimed dynamically), but will be respected if specified
#endif

// PICO_CONFIG: PICO_STDIO_USB_ENABLE_IRQ_BACKGROUND_TASK, Enable/disable the use of a background task to call tud_task(), type=bool, default=1 if the application is not using tinyUSB directly, group=pico_stdio_usb
#ifndef PICO_STDIO_USB_ENABLE_IRQ_BACKGROUND_TASK
#if !LIB_TINYUSB_HOST && !LIB_TINYUSB_DEVICE
#define PICO_STDIO_USB_ENABLE_IRQ_BACKGROUND_TASK 1
#else
#define PICO_STDIO_USB_ENABLE_IRQ_BACKGROUND_TASK 0
#endif
#endif

// PICO_CONFIG: PICO_STDIO_USB_ENABLE_TINYUSB_INIT, Enable/disable calling tinyUSB tusb_init() during initialization, type=bool, default=1 if the application is not using tinyUSB directly, group=pico_stdio_usb
#ifndef PICO_STDIO_USB_ENABLE_TINYUSB_INIT
#if !LIB_TINYUSB_HOST && !LIB_TINYUSB_DEVICE
#define PICO_STDIO_USB_ENABLE_TINYUSB_INIT 1
#else
#define PICO_STDIO_USB_ENABLE_TINYUSB_INIT 0
#endif
#endif

// PICO_CONFIG: PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS, Maximum number of milliseconds to wait during initialization for a CDC connection from the host (negative means indefinite) during initialization, default=0, group=pico_stdio_usb
#ifndef PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS
#define PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS 0
#endif

// PICO_CONFIG: PICO_STDIO_USB_POST_CONNECT_WAIT_DELAY_MS, Number of extra milliseconds to wait when using PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS after a host CDC connection is detected (some host terminals seem to sometimes lose transmissions sent right after connection), default=50, group=pico_stdio_usb
#ifndef PICO_STDIO_USB_POST_CONNECT_WAIT_DELAY_MS
#define PICO_STDIO_USB_POST_CONNECT_WAIT_DELAY_MS 50
#endif

// PICO_CONFIG: PICO_STDIO_USB_DEINIT_DELAY_MS, Number of milliseconds to wait before deinitializing stdio_usb, default=110, group=pico_stdio_usb
#ifndef PICO_STDIO_USB_DEINIT_DELAY_MS
#define PICO_STDIO_USB_DEINIT_DELAY_MS 110
#endif

// PICO_CONFIG: PICO_STDIO_USB_USE_DEFAULT_DESCRIPTORS, Whether `pico_stdio_usb` provides the USB descriptors needed for USB communication, type=bool, default=1 if the application is not using tinyUSB directly, group=pico_stdio_usb
#ifndef PICO_STDIO_USB_USE_DEFAULT_DESCRIPTORS
#if !LIB_TINYUSB_HOST && !LIB_TINYUSB_DEVICE
#define PICO_STDIO_USB_USE_DEFAULT_DESCRIPTORS 1
#else
#define PICO_STDIO_USB_USE_DEFAULT_DESCRIPTORS 0
#endif
#endif

// PICO_CONFIG: PICO_STDIO_USB_CONNECTION_WITHOUT_DTR, Disable use of DTR for connection checking meaning connection is assumed to be valid, type=bool, default=0, group=pico_stdio_usb
#ifndef PICO_STDIO_USB_CONNECTION_WITHOUT_DTR
#define PICO_STDIO_USB_CONNECTION_WITHOUT_DTR 0
#endif

// PICO_CONFIG: PICO_STDIO_USB_DEVICE_SELF_POWERED, Set USB device as self powered device, type=bool, default=0, group=pico_stdio_usb
#ifndef PICO_STDIO_USB_DEVICE_SELF_POWERED
#define PICO_STDIO_USB_DEVICE_SELF_POWERED 0
#endif

// PICO_CONFIG: PICO_STDIO_USB_SUPPORT_CHARS_AVAILABLE_CALLBACK, Enable USB STDIO support for stdio_set_chars_available_callback. Can be disabled to make use of USB CDC RX callback elsewhere, type=bool, default=1, group=pico_stdio_usb
#ifndef PICO_STDIO_USB_SUPPORT_CHARS_AVAILABLE_CALLBACK
#define PICO_STDIO_USB_SUPPORT_CHARS_AVAILABLE_CALLBACK 1
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern stdio_driver_t stdio_usb;

/*! \brief Explicitly initialize USB stdio and add it to the current set of stdin drivers
 *  \ingroup pico_stdio_usb
 *
 *  \ref PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS can be set to cause this method to wait for a CDC connection
 *  from the host before returning, which is useful if you don't want any initial stdout output to be discarded
 *  before the connection is established.
 *
 *  \return true if the USB CDC was initialized, false if an error occurred
 */
bool stdio_usb_init(void);

/*! \brief Explicitly deinitialize USB stdio and remove it from the current set of stdin drivers
 *  \ingroup pico_stdio_usb
 *
 *  \return true if the USB CDC was deinitialized, false if an error occurred
 */
bool stdio_usb_deinit(void);

/*! \brief Check if there is an active stdio CDC connection to a host
 *  \ingroup pico_stdio_usb
 *
 *  \return true if stdio is connected over CDC
 */
bool stdio_usb_connected(void);

#if PICO_STDIO_USB_SUPPORT_CHARS_AVAILABLE_CALLBACK
/*! \brief Explicitly calls the registered USB stdio chars_available_callback
 *  \ingroup pico_stdio_usb
 *
 * This method is normally called by the internal USB stdio background thread when there is new USB CDC
 * data available to read. However, if the internal background thread is disabled (e.g. when the user
 * directly links tinyUSB), the user will need to implement their own background thread and call this
 * method directly.
 */
void stdio_usb_call_chars_available_callback(void);
#endif

#ifdef __cplusplus
}
#endif

#endif
