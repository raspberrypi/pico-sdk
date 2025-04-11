/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico.h"
#include "hardware/address_mapped.h"
#include "hardware/regs/sysinfo.h"

#define MANUFACTURER_RPI 0x926
#define PART_RP4 0x4

uint8_t rp2350_chip_version(void) {
    // First register of sysinfo is chip id
    uint32_t chip_id = *((io_ro_32*)(SYSINFO_BASE + SYSINFO_CHIP_ID_OFFSET));
    uint32_t __unused manufacturer = chip_id & SYSINFO_CHIP_ID_MANUFACTURER_BITS;
    uint32_t __unused part = (chip_id & SYSINFO_CHIP_ID_PART_BITS) >> SYSINFO_CHIP_ID_PART_LSB;
    assert(manufacturer == MANUFACTURER_RPI && part == PART_RP4);
    // 0 == A0, 1 == A1, 2 == A2
    uint32_t version = (chip_id & SYSINFO_CHIP_ID_REVISION_BITS) >> SYSINFO_CHIP_ID_REVISION_LSB;
    version = (version & 3u) | ((version & 8u) >> 1);
    return (uint8_t)version;
}