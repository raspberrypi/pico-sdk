/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "hardware/flash.h"
#include "pico/bootrom.h"
#include "boot/picobin.h"
#include "boot/picoboot.h"
#include "boot/uf2.h"
#include "pico/flash_image.h"

// Placeholder: FIXME HERE TODO This define should end up in an official place
// For ROM code to use, see RP2350 datasheet, section 5.4
#define BOOTROM_WORKAREA_SIZE           (4 * 1024)  // 4kB

// Placeholder: FIXME HERE TODO These might move in time to the uf2.h file
#define UF2_BLOCK_SIZE                  512
#define UF2_DATA_HEADER_SIZE            32
#define UF2_DATA_PAGE_SIZE              256
#define UF2_DATA_WORD_OFFSET_TO_E10_VAL ((UF2_DATA_HEADER_SIZE + UF2_DATA_PAGE_SIZE) / 4)

// The flash page and the UF2 payload must be the same size; fail hard if not.
static_assert(UF2_DATA_PAGE_SIZE == FLASH_PAGE_SIZE, "");

// The largest flash device RP2350 supports is 16 MBytes which is 4096 sectors.
// Using one erased/not erased bit for each sector, 4096 bits requires 512 bytes.
#define ERASE_MAP_SIZE_BYTES            (4096/8)

#define FLASH_ERASE_SECTOR_MASK         ((uint32_t)(~(FLASH_SECTOR_SIZE - 1)))
#define STATUS_DISCARD_UF2_BLOCK        1  // Can't be PICO_OK or PICO_ERROR_xx

// Macros to extract the first and last sector numbers from a partition
// permissions_and_location field
#define FIRST_SECTOR_NUMBER(p)          \
    ((uint32_t)(((p) & PICOBIN_PARTITION_LOCATION_FIRST_SECTOR_BITS) \
        >> PICOBIN_PARTITION_LOCATION_FIRST_SECTOR_LSB))
#define LAST_SECTOR_NUMBER(p)           \
    ((uint32_t)(((p) & PICOBIN_PARTITION_LOCATION_LAST_SECTOR_BITS) \
        >> PICOBIN_PARTITION_LOCATION_LAST_SECTOR_LSB))

typedef union {
    struct uf2_block uf2_block;
    uint8_t bytes[UF2_BLOCK_SIZE];
    uint32_t words[UF2_BLOCK_SIZE / 4];
} rx_data_block_u;

// Size: ~5k + small bits, see PICO_FLASH_IMAGE_WORKAREA_SIZE
typedef struct {
    uint8_t  brworkarea[BOOTROM_WORKAREA_SIZE];      // 4k
    uint8_t  sector_erase_map[ERASE_MAP_SIZE_BYTES]; // 512 bytes
    rx_data_block_u rxed_data;                       // 512 bytes
    uint32_t  rxed_data_index;
    fimg_op_state_e op_state;
    uint32_t total_num_blocks;
    uint32_t rxed_block_count;
    uint32_t update_family_id;
    uint32_t flash_write_address;
    uint32_t update_start_addr;
    uint32_t dnld_crc32;
} flash_image_state_t;

static_assert(sizeof(flash_image_state_t) <= PICO_FLASH_IMAGE_WORKAREA_SIZE,
    "defined storage is too small for flash_image_state_t");

// Flag bit definitions for ROM flash API
static const cflash_flags_t cflash_erase = {
    .flags = (CFLASH_OP_VALUE_ERASE << CFLASH_OP_LSB) |
             (CFLASH_SECLEVEL_VALUE_SECURE << CFLASH_SECLEVEL_LSB) |
             (CFLASH_ASPACE_VALUE_STORAGE << CFLASH_ASPACE_LSB)
};

static const cflash_flags_t cflash_program = {
    .flags = (CFLASH_OP_VALUE_PROGRAM << CFLASH_OP_LSB) |
             (CFLASH_SECLEVEL_VALUE_SECURE << CFLASH_SECLEVEL_LSB) |
             (CFLASH_ASPACE_VALUE_STORAGE << CFLASH_ASPACE_LSB)
};

// module state pointer
static flash_image_state_t *fimg_state = NULL;


#define CRC32_INIT                  ((uint32_t)-1l)

// This uses the "reversed" polynomial and shift direction
static uint32_t crc32_chunk(uint32_t crc, uint8_t *bytp, uint count) {

    while(count--) {
        uint32_t byte32 = (uint32_t)*bytp++;

        for (uint8_t bit = 8; bit; bit--, byte32 >>= 1) {
            crc = (crc >> 1) ^ (((crc ^ byte32) & 1ul) ? 0xEDB88320ul : 0ul);
        }
    }
    return crc;
}


static void find_flash_erase_map_byte_and_bit(uint32_t sector_address,
                                                     uint8_t **byte_ptr,
                                                     uint8_t *bit_mask) {
    uint32_t sec_num = (sector_address - XIP_BASE) / FLASH_SECTOR_SIZE;
    *byte_ptr = (uint8_t *)(fimg_state->sector_erase_map + (sec_num / 8));
    *bit_mask = (uint8_t)(1 << (sec_num % 8));
}


static inline bool check_sector_has_been_erased(uint32_t sector_address) {
    uint8_t *byte_ptr;
    uint8_t bit_mask;
    find_flash_erase_map_byte_and_bit(sector_address, &byte_ptr, &bit_mask);
    return  (0 != (*byte_ptr & bit_mask));
}


static inline void mark_sector_as_erased(uint32_t sector_address) {
    uint8_t *byte_ptr;
    uint8_t bit_mask;
    find_flash_erase_map_byte_and_bit(sector_address, &byte_ptr, &bit_mask);
    *byte_ptr |= bit_mask;
}


// vital: 'on the fly erasing'
static int erase_sector_if_required(uint32_t flash_sector_address) {

    if (check_sector_has_been_erased(flash_sector_address)) {
        return PICO_OK;
    }

    FIMG_DEBUG_PRINTF("programming: %08lx\r", flash_sector_address);

    if (PICO_OK != rom_flash_op(cflash_erase,
                                flash_sector_address,
                                FLASH_SECTOR_SIZE,
                                NULL)) {
        return PICO_ERROR_GENERIC;
    }

    mark_sector_as_erased(flash_sector_address);

    return PICO_OK;
}


static int program_flash_page(uint32_t flash_write_address, uint8_t *data) {

    uint32_t flash_sector_address = flash_write_address & FLASH_ERASE_SECTOR_MASK;
    int status = erase_sector_if_required(flash_sector_address);

    if (PICO_OK == status) {
        if (PICO_OK != rom_flash_op(cflash_program,
                                    flash_write_address,
                                    FLASH_PAGE_SIZE,
                                    data)) {
            status = PICO_ERROR_GENERIC;
        }
    }

    return status;
}


// Check the invariant portions of all incoming uf2 blocks
static int validate_uf2_block_header(void) {

    if (UF2_MAGIC_START0 != fimg_state->rxed_data.uf2_block.magic_start0) {
        return PICO_ERROR_INVALID_DATA;
    }
    if (UF2_MAGIC_START1 != fimg_state->rxed_data.uf2_block.magic_start1) {
        return PICO_ERROR_INVALID_DATA;
    }
    if (UF2_MAGIC_END != fimg_state->rxed_data.uf2_block.magic_end) {
        return PICO_ERROR_INVALID_DATA;
    }
    if (FLASH_PAGE_SIZE != fimg_state->rxed_data.uf2_block.payload_size) {
        return PICO_ERROR_INVALID_DATA;
    }
    if (0 == (UF2_FLAG_FAMILY_ID_PRESENT & fimg_state->rxed_data.uf2_block.flags)) {
        return PICO_ERROR_INVALID_DATA;
    }

    return PICO_OK;
}


static bool check_for_rp2350_e10_block(void) {

    if (0 == (UF2_FLAG_EXTENSION_FLAGS_PRESENT & fimg_state->rxed_data.uf2_block.flags)) {
        return false;
    }
    if (ABSOLUTE_FAMILY_ID != fimg_state->rxed_data.uf2_block.file_size) {
        return false;
    }
    if (UF2_EXTENSION_RP2_IGNORE_BLOCK !=
        *(uint32_t*)&fimg_state->rxed_data.words[UF2_DATA_WORD_OFFSET_TO_E10_VAL]) {
        return false;
    }

    return true;
}


static int read_partition_info(void) {

    resident_partition_t uf2_target_partition;

    rom_flash_flush_cache();

    int target_part_idx = rom_get_uf2_target_partition(fimg_state->brworkarea,
                                                       BOOTROM_WORKAREA_SIZE,
                                                       fimg_state->update_family_id,
                                                       &uf2_target_partition);
    if (0 > target_part_idx) {
        FIMG_DEBUG_PRINTF("error - %s() target_part_idx = %i\n", __FUNCTION__, target_part_idx);
        return target_part_idx;  // -ve means error ...
    }

    uint32_t first_sector_number = FIRST_SECTOR_NUMBER(uf2_target_partition.permissions_and_location);
    uint32_t last_sector_number = LAST_SECTOR_NUMBER(uf2_target_partition.permissions_and_location);
    uint32_t total_sector_count = 1 + last_sector_number - first_sector_number;
    uint32_t maximum_code_size = FLASH_SECTOR_SIZE * total_sector_count;
    uint32_t update_size = UF2_DATA_PAGE_SIZE * fimg_state->total_num_blocks;

    // check if the image is too large for the partition
    if (update_size > maximum_code_size) {
        FIMG_DEBUG_PRINTF("error - %s() returned: %i\n", __FUNCTION__, PICO_ERROR_BUFFER_TOO_SMALL);
        return PICO_ERROR_BUFFER_TOO_SMALL;
    }

    // execution and programming start address is the start of the code
    uint32_t code_start_offset = FLASH_SECTOR_SIZE * first_sector_number;
    fimg_state->update_start_addr = XIP_BASE + code_start_offset;

    FIMG_DEBUG_PRINTF("info - update partition: %d, family id: %08lx, start addr: %08lx, max "
        "code size: %08lx\n", target_part_idx, fimg_state->update_family_id,
        fimg_state->update_start_addr, maximum_code_size);

    return PICO_OK;
}


static int process_a_uf2_block(void) {

    // validate the block's header
    int status = validate_uf2_block_header();
    if (PICO_OK != status) {
        return status;
    }

    if (fimg_state->rxed_block_count == 0) { // First block?

        if (check_for_rp2350_e10_block()) {
            return STATUS_DISCARD_UF2_BLOCK;  // ignore erata mitigation block
        }

         // If a specific family ID is specified, it must match the family ID in the block
        if (fimg_state->update_family_id != 0 &&
            fimg_state->update_family_id != fimg_state->rxed_data.uf2_block.file_size) {
            FIMG_DEBUG_PRINTF("error - %s() returned: %i\n", __FUNCTION__, PICO_ERROR_NOT_PERMITTED);
            return PICO_ERROR_NOT_PERMITTED;
        }

        // Snapshot metadata
        fimg_state->total_num_blocks = fimg_state->rxed_data.uf2_block.num_blocks;
        fimg_state->update_family_id = fimg_state->rxed_data.uf2_block.file_size;

        status = read_partition_info();
        if (PICO_OK != status) {
            return status;
        }
    }

    else { // Subsequent blocks: (re)check block metadata
        if (fimg_state->update_family_id != fimg_state->rxed_data.uf2_block.file_size) {
            return PICO_ERROR_INVALID_DATA;
        }
        if (fimg_state->total_num_blocks != fimg_state->rxed_data.uf2_block.num_blocks) {
            return PICO_ERROR_INVALID_DATA;
        }
    }

    // roll crc32 algo over the latest block to program
    fimg_state->dnld_crc32 = crc32_chunk(fimg_state->dnld_crc32,
                                         fimg_state->rxed_data.bytes,
                                         UF2_BLOCK_SIZE);

    // per-block, calculate write address as the target address relative to the start of the code
    uint32_t target_offset = fimg_state->rxed_data.uf2_block.target_addr - XIP_BASE;
    fimg_state->flash_write_address = fimg_state->update_start_addr + target_offset;

    // showtime: program the block's data to flash
    status = program_flash_page(fimg_state->flash_write_address,
                                fimg_state->rxed_data.uf2_block.data);

    if (PICO_OK == status) {
        fimg_state->rxed_block_count++;
    }

    return status;
}


void fimg_set_op_state(int new_state) {

    FIMG_DEBUG_PRINTF("flash_image state changed: %i -> %i\n", fimg_state->op_state, new_state);

    fimg_state->op_state = new_state;
}


// Most of the API uses this to validate the incoming state pointer arg.
static int validate_fimg_state_ptr(void *state) {

    if (NULL == fimg_state) {
        return PICO_ERROR_NOT_PERMITTED;  // no local state data to operate on
    }

    if (NULL == state) {
        return PICO_ERROR_INVALID_ARG;
    }

    if (fimg_state != (flash_image_state_t *) state) {
        return PICO_ERROR_NOT_FOUND;  // not the same memory area!
    }

    return PICO_OK;
}

//----------------------------------------------------------------------------

int pico_flash_image_supply_storage(void *storage, uint storage_size) {

    int status = PICO_OK;

    if (NULL == storage) {
        status = PICO_ERROR_INVALID_ARG;
    }

    if (PICO_OK == status && storage_size < sizeof(flash_image_state_t)) {
        // The supplied work area is too small to hold the required state data
        status = PICO_ERROR_BUFFER_TOO_SMALL;
    }

    if (PICO_OK == status && NULL != fimg_state &&
        fimg_state != (flash_image_state_t *) storage) {
        // *Different* storage already supplied
        status = PICO_ERROR_RESOURCE_IN_USE;
    }

    if (PICO_OK != status) {
        FIMG_DEBUG_PRINTF("error - %s() returned: %i, storage: %08x, fimg_state: %08x\n",
            __FUNCTION__, status, (uint) storage, (uint) fimg_state);
        return status;
    }

    fimg_state = (flash_image_state_t *) storage;
    memset(fimg_state, 0, sizeof(flash_image_state_t));
    fimg_set_op_state(FIMG_STATE_IDLE);

    return PICO_OK;
}


int pico_flash_image_remove_storage(void *storage) {

    int status = validate_fimg_state_ptr(storage);

    if (PICO_OK == status) {
        fimg_state = NULL;
    }

    else {
        FIMG_DEBUG_PRINTF("error - %s() returned: %i\n", __FUNCTION__, status);
    }

    return status;
}


bool pico_flash_image_check_has_storage(void) {

    if (NULL == fimg_state) {
        return false;
    }

    return true;
}


int pico_flash_image_get_op_status(fimg_update_info_t *fimg_update_info) {

    if (NULL == fimg_update_info) {
        FIMG_DEBUG_PRINTF("error - %s() returned: %i\n", __FUNCTION__, PICO_ERROR_INVALID_ARG);
        return PICO_ERROR_INVALID_ARG;
    }

    memset(fimg_update_info, 0, sizeof(fimg_update_info_t));

    if (NULL == fimg_state) {
        fimg_update_info->op_state = FIMG_STATE_NO_STORAGE;
        return PICO_OK;
    }

    fimg_update_info->op_state = fimg_state->op_state;

    if (FIMG_STATE_WRITE_ERROR == fimg_state->op_state ||
        FIMG_STATE_WRITE_SUCCESS == fimg_state->op_state ||
        FIMG_STATE_WRITING_IN_PROGRESS == fimg_state->op_state) {
        fimg_update_info->update_family_id = fimg_state->update_family_id;
        fimg_update_info->total_num_blocks = fimg_state->total_num_blocks;
        fimg_update_info->rxed_block_count = fimg_state->rxed_block_count;
        fimg_update_info->rxed_data_index  = fimg_state->rxed_data_index;
    }

    return PICO_OK;
}


int pico_flash_image_reset_to_idle(void *state) {

    int status = validate_fimg_state_ptr(state);
    if (PICO_OK != status) {
        FIMG_DEBUG_PRINTF("error - %s() returned: %i\n", __FUNCTION__, status);
        return status;
    }

    fimg_set_op_state(FIMG_STATE_IDLE);

    return PICO_OK;
}


int pico_flash_image_config_for_update(void *state, uint32_t update_family_id) {

    int status = validate_fimg_state_ptr(state);
    if (PICO_OK != status) {
        FIMG_DEBUG_PRINTF("error - %s() returned: %i\n", __FUNCTION__, status);
        return status;
    }

    // Allow only from the expected states
    if (FIMG_STATE_IDLE != fimg_state->op_state &&
        FIMG_STATE_WRITE_ERROR != fimg_state->op_state &&
        FIMG_STATE_WRITE_SUCCESS != fimg_state->op_state) {
        return PICO_ERROR_NOT_PERMITTED;
    }

    fimg_set_op_state(FIMG_STATE_WAITING_FOR_DATA);

    // initialise internal state variables
    fimg_state->update_family_id = update_family_id;
    fimg_state->rxed_block_count = 0;
    fimg_state->rxed_data_index = 0;
    fimg_state->dnld_crc32 = CRC32_INIT;

    // initialise all the erased sector map to 'NOT erased'
    memset(fimg_state->sector_erase_map, 0, sizeof(fimg_state->sector_erase_map));

    return PICO_OK;
}


int pico_flash_image_write_data(void *state, const char *data, uint length) {

    int status = validate_fimg_state_ptr(state);
    if (PICO_OK != status) {
        FIMG_DEBUG_PRINTF("error - %s() returned: %i\n", __FUNCTION__, status);
        return status;
    }

    if (FIMG_STATE_WRITING_IN_PROGRESS != fimg_state->op_state) {

        if (FIMG_STATE_WAITING_FOR_DATA == fimg_state->op_state) {
            fimg_set_op_state(FIMG_STATE_WRITING_IN_PROGRESS);
            // First time only, now drop through to the programming loop
        }
        else {
            // wrong state, return error, but don't change state
            FIMG_DEBUG_PRINTF("error - %s() returned: %i\n", __FUNCTION__, PICO_ERROR_NOT_PERMITTED);
            return PICO_ERROR_NOT_PERMITTED;
        }
    }

    // while data to write and no error ...
    while (0 < length && PICO_OK == status) {

        // Copy in as many bytes as we have and/or we can fit into the uf2 block buffer
        uint32_t space_left = UF2_BLOCK_SIZE - fimg_state->rxed_data_index;
        uint32_t bytes_to_copy = length > space_left ? space_left : length;

        memcpy(&fimg_state->rxed_data.bytes[fimg_state->rxed_data_index], data, bytes_to_copy);

        length -= bytes_to_copy;
        data  += bytes_to_copy;
        fimg_state->rxed_data_index += bytes_to_copy;

        if (UF2_BLOCK_SIZE == fimg_state->rxed_data_index) {

            status = process_a_uf2_block();

            // In all cases, reset buffer index, the block data has been consumed
            fimg_state->rxed_data_index = 0;

            if (STATUS_DISCARD_UF2_BLOCK == status) {
                status = PICO_OK;  // reset status to OK, the block has been discarded
                continue;
            }

            // check if we have now received all the expected blocks
            if (PICO_OK == status &&
                fimg_state->rxed_block_count == fimg_state->total_num_blocks) {
                // ... and we're done
                fimg_set_op_state(FIMG_STATE_WRITE_SUCCESS);
                FIMG_DEBUG_PRINTF("info - download success, crc32: %08lx\n", fimg_state->dnld_crc32);
            }
        }
    }

    if (PICO_OK != status) {
        fimg_set_op_state(FIMG_STATE_WRITE_ERROR);  // halt further programming
    }

    return status;
}


int pico_flash_image_check_write_complete(void *state, bool *complete, uint32_t *update_start_addr) {

    int status = validate_fimg_state_ptr(state);
    if (PICO_OK != status) {
        FIMG_DEBUG_PRINTF("error - %s() returned: %i\n", __FUNCTION__, status);
        return status;
    }

    if (NULL == complete) {
        return PICO_ERROR_INVALID_ARG;
    }

    if (NULL == update_start_addr) {
        return PICO_ERROR_INVALID_ARG;
    }

    if (FIMG_STATE_WRITE_SUCCESS == fimg_state->op_state) {
        *complete = true;
        *update_start_addr = fimg_state->update_start_addr;
    }
    else {
        *complete = false;
        *update_start_addr = 0;
    }

    return PICO_OK;
}

