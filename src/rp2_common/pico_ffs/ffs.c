/**
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>

#include "pico/stdlib.h"
#include "pico/flash.h"
#include "hardware/flash.h"
#include "pico/bootrom.h"
#include "boot/picobin.h"
#include "ffs_internal.h"
#include "pico/ffs.h"

#define NUMBER_OF_CHUNKS                2

#define PT_ID_LOCATION_AND_FLAGS \
    (PT_INFO_PARTITION_LOCATION_AND_FLAGS | PT_INFO_PARTITION_ID)
#define PT_SINGLE_LOCATION_AND_FLAGS \
    (PT_INFO_PARTITION_LOCATION_AND_FLAGS | PT_INFO_SINGLE_PARTITION)

// Used to mark flash offset addresses as 'not in use'
#define NOT_A_VALID_FLASH_OFFSET        0xffffffff

// The programming API uses *offsets* from the flash start.
// Flash reading (obviously) requires the actual memory address.
#ifdef PICO_RP2040
#define FL_OFFSET_TO_READ_ADDR(_o)      ((void *)(XIP_BASE + (uint32_t)(_o)))
#else // RP2350 may be using address mapping, we need to avoid remapped addresses
#define FL_OFFSET_TO_READ_ADDR(_o)      \
    ((void *)(XIP_NOCACHE_NOALLOC_NOTRANSLATE_BASE + (uint32_t)(_o)))
#endif

// Convert an unaligned flash offset into its page start offset
#define ALIGN_TO_START(_o, _a)          ((_o) & ~((_a)-1))
#define FL_OFFSET_TO_PAGE_START(_o)     ALIGN_TO_START((_o), FLASH_PAGE_SIZE)

typedef enum {
    FILE_FOUND_VALID   = 0
  , FILE_FOUND_NASCENT = 1 // means finalisation did not complete
  , FILE_FOUND_DEPRECATED = 2
  , VIRGIN_FLASH       = 3
  , UNEXPECTED_DATA    = 4
} file_found_e;

typedef struct {
    uint32_t increment;
    uint32_t check_offset;
    uint32_t end_offset;
    file_found_e found_filter;
    file_found_e file_found;
} flash_iterator_t;

typedef struct {
    uint8_t  cached_flash_page[FLASH_PAGE_SIZE];
    uint32_t wr_page_cache_offset;
    uint32_t start_offset;
    uint32_t end_offset;
    uint32_t chunk_size;
    uint32_t next_write;
    bool initialised;
} ffs_data_t;

static ffs_data_t ffs_data = { 0 };

#ifndef PICO_RP2040
// This code copied from: pico-sdk/src/rp2_common/pico_cyw43_driver/cyw43_driver.c
static int get_ffs_partition_info(void) {

    // maximum of 16 partitions, each with maximum of 4 words returned, plus 1
    uint32_t buffer[(16 * 4) + 1] = { };

    int num_p = rom_get_partition_table_info(buffer, count_of(buffer), PT_ID_LOCATION_AND_FLAGS);
    if (0 >= num_p || PT_ID_LOCATION_AND_FLAGS != buffer[0]) {
        return PICO_ERROR_GENERIC;
    }

    int picked_p = -1;
    int p = 0;
    int i = 1;

    while (i < num_p) {
        i++;
        uint32_t flags_and_permissions = buffer[i++];
        bool has_id = (flags_and_permissions & PICOBIN_PARTITION_FLAGS_HAS_ID_BITS);
        if (has_id) {
            uint64_t id = 0;
            id |= buffer[i++];
            id |= ((uint64_t)(buffer[i++]) << 32ull);
            if (id == FFS_DATA_PARTITION_ID) {
                picked_p = p;
                break;
            }
        }
        p++;
    }

    if (0 > picked_p) {
        return PICO_ERROR_GENERIC;
    }

    int ret = rom_get_partition_table_info(buffer, count_of(buffer), PT_SINGLE_LOCATION_AND_FLAGS | (picked_p << 24));
    if (3 != ret || PT_SINGLE_LOCATION_AND_FLAGS != buffer[0]) {
        return PICO_ERROR_NOT_FOUND;
    }

    uint32_t loc_and_perms = buffer[1];
    ffs_data.start_offset = ((loc_and_perms >> PICOBIN_PARTITION_LOCATION_FIRST_SECTOR_LSB) & 0x1fffu) * FLASH_SECTOR_SIZE;
    ffs_data.end_offset = (((loc_and_perms >> PICOBIN_PARTITION_LOCATION_LAST_SECTOR_LSB) & 0x1fffu) + 1) * FLASH_SECTOR_SIZE;

    return PICO_OK;
}
#endif // !PICO_RP2040


static uint32_t calc_this_chunk_start(uint32_t offset) {

    uint32_t this_chunk_start = ffs_data.start_offset;

    if (offset >= ffs_data.start_offset + ffs_data.chunk_size
        && offset < ffs_data.end_offset) {  // handle "wrap"
        this_chunk_start += ffs_data.chunk_size;
    }

    return this_chunk_start;
}


static uint32_t calc_next_chunk_start(uint32_t offset) {

    uint32_t next_chunk_start = ffs_data.start_offset;

    if (offset < ffs_data.start_offset + ffs_data.chunk_size) {
        next_chunk_start += ffs_data.chunk_size;
    }

    return next_chunk_start;
}


static void call_flash_range_erase(void *param) {

    uint32_t offset = ((uintptr_t*)param)[0];
    uint32_t length = ((uintptr_t*)param)[1];

    flash_range_erase(offset, length);
}


static void call_flash_range_program(void *param) {

    uint32_t offset = ((uintptr_t*)param)[0];
    const uint8_t *data = (const uint8_t *)((uintptr_t*)param)[1];

    flash_range_program(offset, data, FLASH_PAGE_SIZE);
}


static int flash_erase_chunk_sectors(uint32_t offset) {

    uintptr_t params[] = { offset, ffs_data.chunk_size };
    int ret = flash_safe_execute(call_flash_range_erase, params, UINT32_MAX);

    return ret;
}


static int flash_program_a_page(uint32_t offset, const void *page) {

    uintptr_t params[] = { offset, (uintptr_t)page };
    int ret = flash_safe_execute(call_flash_range_program, params, UINT32_MAX);

    return ret;
}


static inline void flash_read_into_cache(uint32_t offset) {
    memcpy(ffs_data.cached_flash_page, FL_OFFSET_TO_READ_ADDR(offset), FLASH_PAGE_SIZE);
}


static bool flash_write_cache_flush(void) {

    bool error = false;

    // Check if the cache page has data to write in it
    if (NOT_A_VALID_FLASH_OFFSET != ffs_data.wr_page_cache_offset) {
        if (0 > flash_program_a_page(ffs_data.wr_page_cache_offset,
                                     ffs_data.cached_flash_page)) {
            error = true;
        }

        ffs_data.wr_page_cache_offset = NOT_A_VALID_FLASH_OFFSET;
    }

    return error;
}


// NOTE: This function automatically advances the write address used as
// data is written so that callers can easily store consecutive items.
// This function can leave un-programmed data in the write cache which
// can be explicitly written by calling flash_write_cache_flush()
static bool flash_write_via_cache(uint32_t *ptr_to_wr_offset,
                                  const void *data,
                                  uint16_t length) {

    const uint8_t *rd_ptr = (const uint8_t *)data;
    bool error = false;

    while (0 != length && false == error) {

        uint32_t flash_page_start = FL_OFFSET_TO_PAGE_START(*ptr_to_wr_offset);

        // check for a page in ram cache to write into flash
        if (NOT_A_VALID_FLASH_OFFSET == ffs_data.wr_page_cache_offset) {
            flash_read_into_cache(flash_page_start);
            ffs_data.wr_page_cache_offset = flash_page_start;
        }

        // Check if new offset is within the current page
        else if (flash_page_start != ffs_data.wr_page_cache_offset) {
            if (0 > flash_program_a_page(ffs_data.wr_page_cache_offset,
                                         ffs_data.cached_flash_page)) {
                error = true;
            }

            ffs_data.wr_page_cache_offset = NOT_A_VALID_FLASH_OFFSET;
            continue;
        }

        // copy 1 character of data into the flash page holding buffer
        ffs_data.cached_flash_page[*ptr_to_wr_offset % FLASH_PAGE_SIZE] = *rd_ptr++;
        (*ptr_to_wr_offset)++;
        length--;
    }

    return error;
}


// NOTE: This function automatically advances the write address used as
// data is written so that callers can easily store consecutive items.
// This function will program data into flash leaving the cache clear.
static bool flash_write_and_flush_cache(uint32_t *ptr_to_wr_offset,
                                        const void *data,
                                        uint16_t length) {

    bool error = flash_write_via_cache(ptr_to_wr_offset, data, length);

    if (flash_write_cache_flush()) {
        error = true;
    }

    return error;
}


static bool mark_file_as_deprecated(uint32_t file_offset) {

    uint32_t file_status_offset = file_offset + OFFSET_OF_HDR_STATUS_BYTE();
    const uint8_t status = FILE_STATUS_DEPRECATED;

    return flash_write_and_flush_cache(&file_status_offset, &status, sizeof(status));
}


static file_found_e check_for_file_at_address(ffs_file_hdr_t *pfile_hdr) {

    if (VIRGIN_FLASH_WORD_VALUE == pfile_hdr->magic[0]) {
        return VIRGIN_FLASH;
    }

    if (0 != memcmp(&pfile_hdr->magic, file_magic, sizeof(file_magic))) {
        return UNEXPECTED_DATA;
    }

    // sanity check length to avoid a broken header possibly bricking the crc check!
    uint32_t data_len = pfile_hdr->data_len;
    if (FFS_API_MAX_WRITE_LENGTH < data_len) {
        return UNEXPECTED_DATA;
    }

    // Calculate the crc0 value which comprises the header up
    // until the status field and also the actual file data.
    void *data = (void *)((int)pfile_hdr + sizeof(ffs_file_hdr_t));
    uint32_t crc0 = file_hdr_calc_crc0(pfile_hdr, data, data_len);

    // check crc0, the "residue" should be 0 with an valid header & data, ignoring file status
    if (0 != crc32_block_of_bytes(crc0, &pfile_hdr->crc0, sizeof(pfile_hdr->crc0))) {
        return UNEXPECTED_DATA;
    }

    uint32_t crc1 = file_hdr_calc_crc1(pfile_hdr);

    // check crc1, the "residue" will be 0 for a valid file
    if (0 == crc32_block_of_bytes(crc1, &pfile_hdr->crc1, sizeof(pfile_hdr->crc1))) {
        if (FILE_STATUS_VALID == pfile_hdr->status) {
            return FILE_FOUND_VALID;
        }
    }

    // At this point we have a bad crc1 but still a good crc0
    if (FILE_STATUS_NASCENT == pfile_hdr->status)  {
        return FILE_FOUND_NASCENT;
    }

    if (FILE_STATUS_DEPRECATED == pfile_hdr->status)  {
        return FILE_FOUND_DEPRECATED;
    }

    return UNEXPECTED_DATA;
}


static void *flash_iterator_get_next_file(flash_iterator_t *iterator) {

    while (1) {

        iterator->check_offset += iterator->increment;  // Note: must be 0 first time
        iterator->check_offset = ROUND_UP_TO_NEXT_32B_WORD(iterator->check_offset);

        if (iterator->check_offset >= iterator->end_offset) {
            return NULL;
        }

        ffs_file_hdr_t *pfile_hdr = (ffs_file_hdr_t *)FL_OFFSET_TO_READ_ADDR(iterator->check_offset);
        iterator->file_found = check_for_file_at_address(pfile_hdr);

        // Vital: Set increment value for next loop round *before* the (possible) return of file ptr
        if (FILE_FOUND_NASCENT == iterator->file_found ||
            FILE_FOUND_VALID   == iterator->file_found ||
            FILE_FOUND_DEPRECATED == iterator->file_found) {
            // file checked well enough to trust the data length field
            iterator->increment = sizeof(ffs_file_hdr_t) + pfile_hdr->data_len;
        }
        else {
            iterator->increment = 4;
        }

        // If the caller is sufficiently interested, return them the header pointer
        if (iterator->found_filter >= iterator->file_found) {
            return pfile_hdr;
        }
    }
}


static uint search_for_specified_valid_file(uint file_id) {

    flash_iterator_t iterator;
    iterator.check_offset = ffs_data.start_offset; // All Flash
    iterator.end_offset   = ffs_data.end_offset;
    iterator.increment    = 0;
    iterator.found_filter = FILE_FOUND_VALID;
    ffs_file_hdr_t *pfile_hdr;

    while ((pfile_hdr = flash_iterator_get_next_file(&iterator)) != NULL) {

        if (pfile_hdr->file_id == file_id) {
            return iterator.check_offset;
        }
    }

    return NOT_A_VALID_FLASH_OFFSET;
}


static int remove_valid_file_if_found(uint file_id) {

    uint existing_file_in_flash = search_for_specified_valid_file(file_id);

    if (NOT_A_VALID_FLASH_OFFSET == existing_file_in_flash) {
        return PICO_ERROR_NOT_FOUND;
    }

    if (mark_file_as_deprecated(existing_file_in_flash)) {
        return PICO_ERROR_GENERIC;
    }

    return PICO_OK;
}


static bool write_new_file_to_flash(uint file_id, const char *wrdata, uint data_len) {

    // The file header structure contains words, it must start on a word boundary
    ffs_data.next_write = ROUND_UP_TO_NEXT_32B_WORD(ffs_data.next_write);

    if (ffs_data.next_write >= ffs_data.end_offset) {
        return true;
    }

    // Step 1: Write file header to flash including crc0 with a 'nascent' status
    ffs_file_hdr_t hdr_buff;
    file_header_generate(&hdr_buff, file_id, wrdata, data_len);

    uint32_t header_offset = ffs_data.next_write;
    bool error = flash_write_via_cache(&ffs_data.next_write, &hdr_buff, sizeof(ffs_file_hdr_t));

    // Ensure header and data, along with crc0 are actually *programmed* in flash.
    if (flash_write_and_flush_cache(&ffs_data.next_write, wrdata, data_len)) {
        error = true;
    }

    // Step 2 If there is a currently "valid" version of the file, remove it by voiding the record
    (void) remove_valid_file_if_found(hdr_buff.file_id);

    // Step 3: Finalise new file by marking it as 'valid' status / setting crc1
    file_header_validate(&hdr_buff);
    if (flash_write_and_flush_cache(&header_offset, &hdr_buff, sizeof(ffs_file_hdr_t))) {
        error = true;
    }

    return error;
}


static bool chunk_erase_and_set_nascent_header(uint32_t offset) {

   bool error = flash_erase_chunk_sectors(offset);

    ffs_file_hdr_t hdr_buff;
    file_header_generate(&hdr_buff, FILE_ID_CHUNK_HEADER, NULL, 0);

    if (flash_write_and_flush_cache(&offset, &hdr_buff, sizeof(ffs_file_hdr_t))) {
        error = true;
    }

    return error;
}


static bool chunk_set_header_as_valid(uint32_t offset) {

    ffs_file_hdr_t hdr_buff;
    ffs_file_hdr_t *pfile_hdr = (ffs_file_hdr_t *)FL_OFFSET_TO_READ_ADDR(offset);

    memcpy(&hdr_buff, pfile_hdr, sizeof(ffs_file_hdr_t));
    file_header_validate(&hdr_buff);

    return flash_write_and_flush_cache(&offset, &hdr_buff, sizeof(ffs_file_hdr_t));
}

// Vitally important function:
// 0) Ensure any junk chunks are erased and set as nascent.
// 1) In the case of two nascent chunks, mark the first valid.
// 2) In the case of a deprecated chunk, complete the file copy, erase and set as nascent.
// 3) Return the offset to the start of the valid chunk.
static uint32_t find_or_make_a_valid_chunk(bool *complete_file_move) {

    *complete_file_move = false;

    file_found_e file_found[NUMBER_OF_CHUNKS];

    // Reading chunk header status - erase and set junk chunks nascent
    uint32_t chunk_offset = ffs_data.start_offset;
    for (int i = 0; i < NUMBER_OF_CHUNKS; i++, chunk_offset += ffs_data.chunk_size) {

        ffs_file_hdr_t *pfile_hdr = (ffs_file_hdr_t *)FL_OFFSET_TO_READ_ADDR(chunk_offset);
        file_found[i] = check_for_file_at_address(pfile_hdr);

        if (FILE_FOUND_DEPRECATED != file_found[i] &&
            FILE_FOUND_NASCENT != file_found[i] &&
            FILE_FOUND_VALID != file_found[i]) {
            (void)chunk_erase_and_set_nascent_header(chunk_offset);
            file_found[i] = FILE_FOUND_NASCENT;
        }
    }

    if (FILE_FOUND_NASCENT == file_found[0] &&
        FILE_FOUND_NASCENT == file_found[1]) {
        // Special case on boot with empty flash, make chunk 0 valid
        (void)chunk_set_header_as_valid(ffs_data.start_offset);
        return ffs_data.start_offset;
    }

    chunk_offset = ffs_data.start_offset;
    for (int i = 0; i < NUMBER_OF_CHUNKS; i++, chunk_offset += ffs_data.chunk_size) {

        // Handle Valid & Nascent *or* Nascent & Valid
        if (FILE_FOUND_VALID == file_found[i] && FILE_FOUND_NASCENT == file_found[i^1]) {
            return chunk_offset;
        }

        // Handle Valid & Deprecated *or* Deprecated & Valid
        if (FILE_FOUND_VALID == file_found[i] && FILE_FOUND_DEPRECATED == file_found[i^1]) {
            *complete_file_move = true;
            return chunk_offset;
        }
    }

    return 0;
}


// This is an important function, run during initialisation
static uint32_t get_next_flash_write_offset(uint32_t chunk_start) {

    flash_iterator_t iterator;
    iterator.check_offset = chunk_start;
    iterator.end_offset   = chunk_start + ffs_data.chunk_size;
    iterator.increment    = 0;
    iterator.found_filter = UNEXPECTED_DATA;  // filter for
    ffs_file_hdr_t *pfile_hdr;

    while ((pfile_hdr = flash_iterator_get_next_file(&iterator)) != NULL) {

        if (VIRGIN_FLASH == iterator.file_found) {
            break;  // result!
        }

        if (FILE_FOUND_NASCENT == iterator.file_found && FILE_ID_CHUNK_HEADER != pfile_hdr->file_id) {

            // Remove any "valid" copy, the nascent one will replace this
            (void) remove_valid_file_if_found(pfile_hdr->file_id);

            ffs_file_hdr_t hdr_buff;
            memcpy(&hdr_buff, pfile_hdr, sizeof(ffs_file_hdr_t));

            file_header_validate(&hdr_buff);
            uint32_t finalise_offset = iterator.check_offset; // disposable variable
            (void) flash_write_and_flush_cache(&finalise_offset, &hdr_buff, sizeof(ffs_file_hdr_t));
        }

        else if (UNEXPECTED_DATA == iterator.file_found) {
            //FIXME HERE TODO
        }
    }

    return iterator.check_offset;
}


static bool move_existing_files_to_latest_chunk(void) {

    uint error_count = 0;

    // NOTE: Only check the (now) old chunk for files
    uint32_t chunk_to_read_erase = calc_next_chunk_start(ffs_data.next_write);

    flash_iterator_t iterator;
    iterator.check_offset = chunk_to_read_erase;
    iterator.end_offset   = chunk_to_read_erase + ffs_data.chunk_size;
    iterator.increment    = 0;
    iterator.found_filter = FILE_FOUND_VALID; // ONLY valid files
    ffs_file_hdr_t *pfile_hdr;

    while ((pfile_hdr = flash_iterator_get_next_file(&iterator)) != NULL) {

        if (FILE_ID_CHUNK_HEADER == pfile_hdr->file_id) {
            continue; //Do NOT move chunk headers between chunks
        }

        // Data, if any, starts immediately after the file header structure
        const char *wrdata = (const char *)((uint32_t)pfile_hdr + (uint32_t)sizeof(ffs_file_hdr_t));
        if (write_new_file_to_flash(pfile_hdr->file_id, wrdata, pfile_hdr->data_len)) {
            error_count++;
        }
    }

    if (chunk_erase_and_set_nascent_header(chunk_to_read_erase)) {
        error_count++;
    }

    if (error_count) {
        return true;
    }

    return false;
}


static uint list_files(file_info_t file_info[], uint max_infos) {

    flash_iterator_t iterator;
    // Only search in the current, active chunk
    iterator.check_offset = calc_this_chunk_start(ffs_data.next_write);
    iterator.end_offset   = iterator.check_offset + ffs_data.chunk_size;
    iterator.increment    = 0;
    iterator.found_filter = FILE_FOUND_VALID; // ONLY valid files

    uint files_found = 0;
    while (files_found < max_infos) {

        ffs_file_hdr_t *pfile_hdr = flash_iterator_get_next_file(&iterator);
        if (NULL == pfile_hdr) {
            break; // no more files!
        }

        // Ignore the chunk header record  - it's not a user accessible r/w file.
        if (FILE_ID_CHUNK_HEADER == pfile_hdr->file_id) {
            continue;
        }

        // Add file details to the list, bump counter.
        file_info[files_found].file_id  = (uint)pfile_hdr->file_id;
        file_info[files_found].data_len = (uint)pfile_hdr->data_len;
        files_found++;
    }

    return files_found;
}


static uint32_t calc_total_size_of_files_except(uint ignore_file_id) {

    flash_iterator_t iterator;
    // Only search in the current, active chunk
    iterator.check_offset = calc_this_chunk_start(ffs_data.next_write);
    iterator.end_offset   = iterator.check_offset + ffs_data.chunk_size;
    iterator.increment    = 0;
    iterator.found_filter = FILE_FOUND_VALID; // ONLY valid files
    ffs_file_hdr_t *pfile_hdr;

    uint32_t file_size_less_one_file = 0;

    // Sum the "in flash" sizes of all files except the specified one
    while ((pfile_hdr = flash_iterator_get_next_file(&iterator)) != NULL) {

        if (ignore_file_id != (uint) pfile_hdr->file_id) {
            file_size_less_one_file += (uint32_t)sizeof(ffs_file_hdr_t);
            file_size_less_one_file += ROUND_UP_TO_NEXT_32B_WORD(pfile_hdr->data_len);
        }
    }

    return file_size_less_one_file;
}

//----------------------------------------------------------------------------

int ffs_initialise(void) {

    ffs_data.initialised = false;
    ffs_data.wr_page_cache_offset = NOT_A_VALID_FLASH_OFFSET;

#ifndef PICO_RP2040
    int status = get_ffs_partition_info();
    if (PICO_OK != status) {
        return status;
    }
#else
    // For now, there are no partitions on RP2040, ffs area is harded coded
    ffs_data.start_offset = FFS_RP2040_FLASH_START_OFFSET;
    ffs_data.end_offset = FFS_RP2040_FLASH_END_OFFSET;
#endif

    // VITAL maths!
    uint32_t ffs_size = ffs_data.end_offset - ffs_data.start_offset;
    ffs_data.chunk_size = ffs_size / 2;

    if (FLASH_SECTOR_SIZE > ffs_data.chunk_size) {
        return PICO_ERROR_BUFFER_TOO_SMALL;
    }

    if ((ffs_data.chunk_size % FLASH_SECTOR_SIZE) ||
        (ffs_data.start_offset % FLASH_SECTOR_SIZE)) {
        return PICO_ERROR_BAD_ALIGNMENT;
    }

    bool complete_file_move;

    uint32_t valid_chunk = find_or_make_a_valid_chunk(&complete_file_move);
    ffs_data.next_write = get_next_flash_write_offset(valid_chunk);

    if (complete_file_move) {
        move_existing_files_to_latest_chunk();
    }

    ffs_data.initialised = true;

    return PICO_OK;
}


int ffs_read(uint file_id, const char **filedata) {

    if (!ffs_data.initialised) {
        return PICO_ERROR_INVALID_STATE;
    }

    if (NULL == filedata) {
        return PICO_ERROR_INVALID_ARG;
    }

    if (FFS_MAX_VALID_FILE_ID < file_id) {
        return PICO_ERROR_INVALID_ARG;
    }

    uint32_t file_offset = search_for_specified_valid_file(file_id);
    if (NOT_A_VALID_FLASH_OFFSET == file_offset) {
        return PICO_ERROR_NOT_FOUND;
    }

    ffs_file_hdr_t *pfile_hdr = (ffs_file_hdr_t *)FL_OFFSET_TO_READ_ADDR(file_offset);

    // Data, if any, starts immediately after the file header structure
    *filedata = (const char *)((uint32_t)pfile_hdr + (uint32_t)sizeof(ffs_file_hdr_t));

    return pfile_hdr->data_len; // Note: could be 0
}


int ffs_delete(uint file_id) {

    if (!ffs_data.initialised) {
        return PICO_ERROR_INVALID_STATE;
    }

    if (FFS_MAX_VALID_FILE_ID < file_id) {
        return PICO_ERROR_INVALID_ARG;
    }

    return remove_valid_file_if_found(file_id);
}


int ffs_write(uint file_id, const char *wrdata, uint data_len) {

    bool error = false;

    if (!ffs_data.initialised) {
        return PICO_ERROR_INVALID_STATE;
    }

    if (NULL == wrdata) {
        return PICO_ERROR_INVALID_ARG;
    }

    if (FFS_MAX_VALID_FILE_ID < file_id) {
        return PICO_ERROR_INVALID_ARG;
    }

    if (FFS_API_MAX_WRITE_LENGTH < data_len) {
        return PICO_ERROR_INVALID_ARG;
    }

    bool mv_files_to_new_chunk = false;

    uint32_t new_filesize = (uint32_t)sizeof(ffs_file_hdr_t) + ROUND_UP_TO_NEXT_32B_WORD(data_len);
    uint32_t end_offset = ROUND_UP_TO_NEXT_32B_WORD(ffs_data.next_write + new_filesize);
    uint32_t current_chunk = calc_this_chunk_start(ffs_data.next_write);
    uint32_t chunk_after_write = calc_this_chunk_start(end_offset);

    if (chunk_after_write != current_chunk) {
        // New chunk: First check we can keep all existing (valid) files.
        // The extra +4 is vital to avoid round-up causing an unhandled wrap.
        uint32_t current_size = calc_total_size_of_files_except(file_id);
        if (ffs_data.chunk_size < current_size + new_filesize + 3) {
            return PICO_ERROR_INSUFFICIENT_RESOURCES;
        }

        // Size ok, prepare to write in the new chunk
        ffs_data.next_write = get_next_flash_write_offset(chunk_after_write);
        mv_files_to_new_chunk = true;

        // mark current (soon to be old) chunk as deprecated
        if (mark_file_as_deprecated(current_chunk)) {
            error = true;
        }

        // mark new chunk as valid
        if (chunk_set_header_as_valid(chunk_after_write)) {
            error = true;
        }
    }

    if (write_new_file_to_flash(file_id, wrdata, data_len)) {
        error = true;
    }

    if (mv_files_to_new_chunk) {
        if (move_existing_files_to_latest_chunk()) {
            error = true;
        }
    }

    if (error) {
        return PICO_ERROR_GENERIC;
    }

    return PICO_OK;
}


int ffs_list(file_info_t file_info[], uint max_infos) {

    if (!ffs_data.initialised) {
        return PICO_ERROR_INVALID_STATE;
    }

    if (NULL == file_info) {
        return PICO_ERROR_INVALID_ARG;
    }

    return (int) list_files(file_info, max_infos);
}


char *ffs_get_string(uint8_t file_id) {
    const char *data;
    int rc = ffs_read(file_id, &data);
    if (rc >= 0 && data) {
        return strndup(data, rc);
    }
    return NULL;
}


// Updates the FFS file data if the new value is different from the existing value.
// If the file does not exist, it is created.
int ffs_update_string(uint8_t file_id, const char *data) {
    int data_len = strlen(data);
    const char *existing_data;
    int rc = ffs_read(file_id, &existing_data);
    if (rc == 0 && existing_data) {
        if (memcmp(existing_data, data, data_len + 1) == 0)
            return 0;
    }

    rc = ffs_write(file_id, data, data_len + 1); // Include the null terminator
    return rc;
}