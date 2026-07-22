/**
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>
#include "ffs_internal.h"

// If *not* an SDK build, this is assumed to be a "host build" i.e. for `ffsgen`
#ifndef PICO_SDK_VERSION_MAJOR
#include <stdint.h>
#endif

#define CRC32_INIT                      ((uint32_t)-1l)

const uint32_t file_magic[2] = { FILE_HDR_ASCII_MAGIC,  FILE_HDR_BINARY_MAGIC };

 // For comments on the algo, see: pico-examples/dma/sniff_crc/sniff_crc.c
uint32_t crc32_block_of_bytes(uint32_t crc32, const void *ptr, uint length) {

    const uint8_t *bytep = (const uint8_t *)ptr;

    while (length--) {
        uint32_t byte32 = (uint32_t)*bytep++;
        for (uint8_t bit = 8; bit; bit--, byte32 >>= 1) {
            crc32 = (crc32 >> 1) ^ (((crc32 ^ byte32) & 1ul) ? 0xEDB88320ul : 0ul);
        }
    }

    return crc32;
}


uint32_t file_hdr_calc_crc0(ffs_file_hdr_t *pfile_hdr, const char *data, uint data_len) {

    const uint crc0_hdr_len = OFFSET_OF_HDR_STATUS_BYTE();

    // crc the header as far as the `status` field
    uint32_t crc0 = crc32_block_of_bytes(CRC32_INIT, pfile_hdr, crc0_hdr_len);

    if (NULL == data || 0 == data_len) {
        return crc0;
    }

    // Roll the crc over the data in the file
    return crc32_block_of_bytes(crc0, data, data_len);
}


uint32_t file_hdr_calc_crc1(ffs_file_hdr_t *pfile_hdr) {

    const uint crc1_crc_len = OFFSET_OF_HDR_CRC1_WORD() - OFFSET_OF_HDR_STATUS_BYTE();

    // crc the the status and crc0 fields. Use crc0 as the seed because it
    // includes the first part of the header and data - no need to recalculate.
    return crc32_block_of_bytes(pfile_hdr->crc0, &pfile_hdr->status, crc1_crc_len);
}

// Really not sure *why* I needed this!
#ifndef static_assert
#define static_assert _Static_assert
#endif

void file_header_generate(void *fhdr_buff, uint file_id, const char *wrdata, uint data_len) {

    ffs_file_hdr_t *pfile_hdr = (ffs_file_hdr_t *)fhdr_buff;

    memset(fhdr_buff, 0xFF, sizeof(ffs_file_hdr_t));
    memcpy(&pfile_hdr->magic, file_magic, sizeof(file_magic));

    // In exactly ONE place, set the elements of the `ffs_file_hdr_t` that (currently)
    // have a specific size that is less than the publically published API.
    // Go bang if this code it out of step with the data types in `ffs_file_hdr_t`
    // Also see FFS_API_MAX_WRITE_LENGTH
    static_assert(sizeof(pfile_hdr->file_id) == sizeof(uint8_t), "");
    static_assert(sizeof(pfile_hdr->data_len) == sizeof(uint16_t), "");

    pfile_hdr->file_id  = (uint8_t)  (file_id &  0xff);
    pfile_hdr->data_len = (uint16_t) (data_len & 0xffff);

    pfile_hdr->status   = FILE_STATUS_NASCENT;

    pfile_hdr->crc0 = file_hdr_calc_crc0(pfile_hdr, wrdata, data_len);
}


void file_header_validate(ffs_file_hdr_t *pfile_hdr) {

    pfile_hdr->status = FILE_STATUS_VALID; // set as valid
    pfile_hdr->crc1 = file_hdr_calc_crc1(pfile_hdr);
}

