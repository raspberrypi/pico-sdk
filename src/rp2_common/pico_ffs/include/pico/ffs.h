/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __FFS_H__
#define __FFS_H__

#include "pico.h"

#ifdef __cplusplus
extern "C" {
#endif

/** \file pico/ffs.h
 *  \defgroup pico_ffs pico_ffs
 *
 * \brief femto flash filing system API
 *  Please see pico_ffs/README_ffs.md for design details.
 */

#if !PICO_RP2040
// PICO_CONFIG: FFS_DATA_PARTITION_ID, ID of the partition used for the femto filing system. This must match the ID used in the partition table JSON, type=int, default=0x746d656673665f6f, group=pico_ffs
#ifndef FFS_DATA_PARTITION_ID
// The default value is a fixed magic identifier reserved for the femto filing system partition
#define FFS_DATA_PARTITION_ID  0x746d656673665f6f
#endif

#else
// On RP2040 there's no flash partitioning, the ffs flash area needs to be hardcoded.
// The defaults make it minimum size and right at the end of the flash memory.
// PICO_CONFIG: FFS_RP2040_FLASH_END_OFFSET, End of ffs flash area + 1, type=int, default=Offset to end of flash + 1, group=pico_ffs
#ifndef FFS_RP2040_FLASH_END_OFFSET
#define FFS_RP2040_FLASH_END_OFFSET  PICO_FLASH_SIZE_BYTES
#endif
// PICO_CONFIG: FFS_RP2040_FLASH_START_OFFSET, Start offset of ffs flash area, type=int, default=Two flash sectors less than end of flash + 1, group=pico_ffs
#ifndef FFS_RP2040_FLASH_START_OFFSET
// Note: This defines the minimum ffs flash area - two sectors
#define FFS_RP2040_FLASH_START_OFFSET (PICO_FLASH_SIZE_BYTES - (2 * FLASH_SECTOR_SIZE))
#endif
#endif

// Used by the API functions: File IDs MUST be less than or equal to this value.
// NOTE: Do NOT change this on a whim, ffs uses an 8-bit type internally!
#define FFS_MAX_VALID_FILE_ID           254

// Maximum length of an individual file that can be specified *on the API*
// The veracity of this is protected by a static_assert() within ffs_internal.c
#define FFS_API_MAX_WRITE_LENGTH        UINT16_MAX

// Structure used by ffs_list() to return stored file information
typedef struct {
    uint file_id;
    uint data_len;
} file_info_t;


/*! \brief Perform 'femto filing system' start of day initialisation
 *  \ingroup pico_ffs
 *
 * Initializes ffs module variables.
 * RP2350: Reads ffs partition location in flash, RP2040 uses hard coded dimensions.
 * Scans ffs flash area to establish start and end of any file records in flash.
 * Will complete interrupted file writes and interrupted coalescing operations.
 *
 * \return PICO_OK on success or PICO_ERROR_xxx
 */
int ffs_initialise(void);


/*! \brief Writes a file to flash.
 *  \ingroup pico_ffs
 *
 * Note: The data length is checked against both the maximum size the ffs's
 * internal data type can accommodate and also the (remaining) available
 * free flash memory size.
 * To ensure integrity, any previous version of the same file will be
 * nullified after successful completion of the new write operation.
 *
 * \param file_id  The file ID to write. This must be <= FFS_MAX_VALID_FILE_ID
 * \param wrdata   Pointer to char data to be written to flash
 * \param data_len Length of data to be written.
 * \return         PICO_OK on success or PICO_ERROR_xxx
 */
int ffs_write(uint file_id, const char *wrdata, uint data_len);


/*! \brief Locate file for reading; set pointer to the data start, return length
 *  \ingroup pico_ffs
 *
 * If the requested file is found in flash, the supplied pointer will be set
 * to the start address of the specified file's data.  The data length, which
 * can be 0, is the return value.  The caller can de-reference the pointer
 * to read the data, up-to the returned length.  If the file is *not* found,
 * the pointer will not be set and a suitable PICO_ERROR_xxx code returned.
 *
 * \param file_id  The file ID to read. This must be <= FFS_MAX_VALID_FILE_ID
 * \param filedata  A pointer to a pointer to hold the file's data start address.
 * \return          If >= 0, the data length of the file in flash.
 *                  If < 0, one of the PICO_ERROR_xxx codes.
 */
int ffs_read(uint file_id, const char **filedata);


/*! \brief Deletes a file, specified by its file ID.
 *  \ingroup pico_ffs
 *
 * \param file_id  The file ID to delete. This must be <= FFS_MAX_VALID_FILE_ID
 * \return PICO_OK on success or PICO_ERROR_xxx
 */
int ffs_delete(uint file_id);


/*! \brief Returns a list of file_info structures up to the maximum number specified.
 *  \ingroup pico_ffs
 *
 * \param file_info Pointer to array of file_info_t data to receive the result
 * \param max_infos Maximum number of file_info_t that can be returned
 * \return          Number of file info records returned on success or PICO_ERROR_xxx
 */
int ffs_list(file_info_t file_info[], uint max_infos);

#ifdef __cplusplus
}
#endif

#endif
