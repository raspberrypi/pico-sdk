/*
 * Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

extern int cyw43_wifi_fw_len;
extern int cyw43_clm_len;

// Only used by older cyw43-driver versions (before v2.0.0), kept so they still build
#define CYW43_WIFI_FW_LEN (cyw43_wifi_fw_len)
#define CYW43_CLM_LEN (cyw43_clm_len)
extern uintptr_t fw_data;

// The CLM follows the firmware, starting on a 512 byte boundary (see cyw43_firmware.py)
#define CYW43_PARTITION_CLM_ADDR (fw_data + ((cyw43_wifi_fw_len + 511) & ~511))

// Normally the firmware and CLM sizes are known at compile time. With the firmware in a
// partition, they're read from the partition at runtime instead. Newer cyw43-driver versions
// get the sizes with sizeof, so these macros cast the data to variable length array types:
// sizeof of a VLA is evaluated at runtime, so it gives the lengths read from the partition
#define cyw43_chipset_firmware_blob (*(const uint8_t (*)[cyw43_wifi_fw_len])fw_data)
#define cyw43_chipset_clm_blob (*(const uint8_t (*)[cyw43_clm_len])CYW43_PARTITION_CLM_ADDR)

#include "boot/picobin.h"
#include "pico/bootrom.h"
