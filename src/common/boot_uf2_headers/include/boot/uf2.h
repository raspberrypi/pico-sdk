/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _BOOT_UF2_H
#define _BOOT_UF2_H

#include <stdint.h>
#include <assert.h>

/** \file uf2.h
*  \defgroup boot_uf2_headers boot_uf2_headers
*
* \brief Header file for the UF2 format supported by a RP2xxx chip in BOOTSEL mode
*/

#define UF2_MAGIC_START0 0x0A324655u
#define UF2_MAGIC_START1 0x9E5D5157u
#define UF2_MAGIC_END    0x0AB16F30u

#define UF2_FLAG_NOT_MAIN_FLASH          0x00000001u
#define UF2_FLAG_FILE_CONTAINER          0x00001000u
#define UF2_FLAG_FAMILY_ID_PRESENT       0x00002000u
#define UF2_FLAG_MD5_PRESENT             0x00004000u
#define UF2_FLAG_EXTENSION_FLAGS_PRESENT 0x00008000u

// Extra family IDs
#define CYW43_FIRMWARE_FAMILY_ID    0xe48bff55u

// Bootrom supported family IDs
#define RP2040_FAMILY_ID            0xe48bff56u
#define ABSOLUTE_FAMILY_ID          0xe48bff57u
#define DATA_FAMILY_ID              0xe48bff58u
#define RP2350_ARM_S_FAMILY_ID      0xe48bff59u
#define RP2350_RISCV_FAMILY_ID      0xe48bff5au
#define RP2350_ARM_NS_FAMILY_ID     0xe48bff5bu
#define BOOTROM_FAMILY_ID_MIN       RP2040_FAMILY_ID
#define BOOTROM_FAMILY_ID_MAX       RP2350_ARM_NS_FAMILY_ID

// Defined for backwards compatibility
#define FAMILY_ID_MAX               BOOTROM_FAMILY_ID_MAX

// 04 e3 57 99
#define UF2_EXTENSION_RP2_IGNORE_BLOCK 0x9957e304

/*! \brief A single 512-byte UF2 block as written to a RP2xxx device in BOOTSEL mode
 *  \ingroup boot_uf2_headers
 *
 * Each block forms one sector of a UF2 file and carries up to 476 bytes of payload data
 * along with addressing and sequencing metadata in a fixed 32-byte header.
 */
struct uf2_block {
    // 32 byte header
    uint32_t magic_start0; ///< First magic number identifying a UF2 block (UF2_MAGIC_START0)
    uint32_t magic_start1; ///< Second magic number identifying a UF2 block (UF2_MAGIC_START1)
    uint32_t flags;        ///< Block flags (e.g. UF2_FLAG_FAMILY_ID_PRESENT)
    uint32_t target_addr;  ///< Address in flash or memory at which to write this block's payload
    uint32_t payload_size; ///< Number of valid data bytes in the payload field
    uint32_t block_no;     ///< Zero-based index of this block within the UF2 file
    uint32_t num_blocks;   ///< Total number of blocks in the UF2 file
    uint32_t file_size;    ///< When UF2_FLAG_FAMILY_ID_PRESENT is set in flags, this contains the UF2 family ID
    uint8_t  data[476];    ///< Raw payload data written to target_addr
    uint32_t magic_end;    ///< Magic number marking the end of the block (UF2_MAGIC_END)
};

static_assert(sizeof(struct uf2_block) == 512, "uf2_block not sector sized");

#endif
