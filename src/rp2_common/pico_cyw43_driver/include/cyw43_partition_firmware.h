/*
 * Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

extern int cyw43_wifi_fw_len;
extern int cyw43_clm_len;

#define CYW43_WIFI_FW_LEN (cyw43_wifi_fw_len)
#define CYW43_CLM_LEN (cyw43_clm_len)
extern uintptr_t fw_data;

#include "boot/picobin.h"
#include "pico/bootrom.h"
