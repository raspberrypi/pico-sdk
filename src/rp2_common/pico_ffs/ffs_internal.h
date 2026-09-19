/**
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Definitions shared between `pico_ffs` SDK library and the utility `ffsgen`
 */

#include <stddef.h>

// If *not* an SDK build, this is assumed to be a "host build" i.e. for `ffsgen`
#ifndef PICO_SDK_VERSION_MAJOR
#include <stdint.h>
#ifndef uint
typedef unsigned int uint;
#endif
// The ffsgen build can't include `ffs.h` which also defines this.
// File IDs must be *less* than this value.  Do NOT change on a whim;
// ffs is currently dimensioned to use an 8-bit value internally.
#define FFS_MAX_VALID_FILE_ID           254 // 8-bit file_id value
#endif

#define FILE_ID_CHUNK_HEADER            (FFS_MAX_VALID_FILE_ID + 1) // = 255

 // Magic tell-tales for validating the file structure header
#define FILE_HDR_ASCII_MAGIC            0x654c6946  // 'FiLe'
#define FILE_HDR_BINARY_MAGIC           0xbe83f5c0

#define FILE_STATUS_NASCENT             0xff   // must be virgin flash state
#define FILE_STATUS_VALID               0x56   // 50/50 1s & 0s, 'V'alid
#define FILE_STATUS_DEPRECATED          0x00   // All zeros, in need of erasure
#define VIRGIN_FLASH_WORD_VALUE         0xffffffff

// File structures always start on a 32-bit boundaries
#define ALIGN_FORWARD(_o, _a)           (((_o) + ((_a)-1)) & ~((_a)-1))
#define ROUND_UP_TO_NEXT_32B_WORD(_o)   ALIGN_FORWARD(((uint32_t)(_o)), 4)

// Note: Ensure the structure adds up to an integer number of 32-bit
// words to avoid any surprises from sizeof(...) rounding up lengths.
typedef struct {
    uint32_t magic[2];
    uint16_t data_len;
    uint8_t  file_id;
    uint8_t  status;
    uint32_t crc0;
    uint32_t crc1;
}  __attribute__((packed)) ffs_file_hdr_t;

#define OFFSET_OF_HDR_STATUS_BYTE()     ((uint)offsetof(ffs_file_hdr_t, status))
#define OFFSET_OF_HDR_CRC1_WORD()       ((uint)offsetof(ffs_file_hdr_t, crc1))

extern const uint32_t file_magic[2];

uint32_t crc32_block_of_bytes(uint32_t crc32, const void *ptr, uint length);
uint32_t file_hdr_calc_crc0(ffs_file_hdr_t *pfile_hdr, const char *data, uint data_len);
uint32_t file_hdr_calc_crc1(ffs_file_hdr_t *pfile_hdr);
void     file_header_generate(void *fhdr_buff, uint file_id, const char *wrdata, uint data_len);
void     file_header_validate(ffs_file_hdr_t *pfile_hdr);
