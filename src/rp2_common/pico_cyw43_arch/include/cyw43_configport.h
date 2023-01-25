/*
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// This header is included by cyw43_driver to setup its environment

#ifndef _CYW43_CONFIGPORT_H
#define _CYW43_CONFIGPORT_H

#include "pico.h"

#ifdef PICO_CYW43_ARCH_HEADER
#include __XSTRING(PICO_CYW43_ARCH_HEADER)
#else
#if PICO_CYW43_ARCH_POLL
#include "pico/cyw43_arch/arch_poll.h"
#elif PICO_CYW43_ARCH_THREADSAFE_BACKGROUND
#include "pico/cyw43_arch/arch_threadsafe_background.h"
#elif PICO_CYW43_ARCH_FREERTOS
#include "pico/cyw43_arch/arch_freertos.h"
#else
#error must specify support pico_cyw43_arch architecture type or set PICO_CYW43_ARCH_HEADER
#endif
#endif

#ifndef CYW43_HOST_NAME
#define CYW43_HOST_NAME "PicoW"
#endif

#ifndef CYW43_GPIO
#define CYW43_GPIO 1
#endif

#ifndef CYW43_LOGIC_DEBUG
#define CYW43_LOGIC_DEBUG 0
#endif

#ifndef CYW43_USE_OTP_MAC
#define CYW43_USE_OTP_MAC 1
#endif

#ifndef CYW43_NO_NETUTILS
#define CYW43_NO_NETUTILS 1
#endif

#ifndef CYW43_IOCTL_TIMEOUT_US
#define CYW43_IOCTL_TIMEOUT_US 1000000
#endif

#ifndef CYW43_USE_STATS
#define CYW43_USE_STATS 0
#endif

// todo should this be user settable?
#ifndef CYW43_HAL_MAC_WLAN0
#define CYW43_HAL_MAC_WLAN0 0
#endif

#ifndef STATIC
#define STATIC static
#endif

#ifndef CYW43_USE_SPI
#define CYW43_USE_SPI 1
#endif

#ifndef CYW43_SPI_PIO
#define CYW43_SPI_PIO 1
#endif

#endif