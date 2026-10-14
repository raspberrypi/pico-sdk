/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_FLASH_IMAGE_H
#define _PICO_FLASH_IMAGE_H

#include "pico.h"

#ifdef __cplusplus
extern "C" {
#endif

// FIXME HERE TODO  Post integration, the `FIMG_DEBUG_PRINTF()` debug mechanism
// will be removed entirely.  For now, it provides essential insight into this
// module's internal operation.
#define DBG_PIO_UART_OUTPUT_GPIO        16  // Define to enable PIO UART output on gpio

#ifdef DBG_PIO_UART_OUTPUT_GPIO
extern void flashlog_printf(const char *fmt, ...)  __attribute__ ((format (gnu_printf, 1, 2)));
#define FIMG_DEBUG_PRINTF(...)          flashlog_printf(__VA_ARGS__)

#else
#define FIMG_DEBUG_PRINTF(...)          ((void) 0)
#endif // DBG_PIO_UART_OUTPUT_GPIO or !

// To start initialisation, users of this library must supply a pointer to a
// "work area" of this size (or more).  The work area is used to hold state
// data for the module.  This is sized to accommodate the module's internal
// `flash_image_state_t` structure and the size is trip-wired using a
// static_assert in the flash_image.c file.
#define PICO_FLASH_IMAGE_WORKAREA_SIZE  5248  // 5.125 kB

// Operating states, see comment below for details
typedef enum {
    FIMG_STATE_NO_STORAGE = 0,
    FIMG_STATE_IDLE = 1,
    FIMG_STATE_WAITING_FOR_DATA = 2,
    FIMG_STATE_WRITING_IN_PROGRESS = 3,
    FIMG_STATE_WRITE_SUCCESS = 4,
    FIMG_STATE_WRITE_ERROR = 5
} fimg_op_state_e;

// Structure used to return the current state information, see pico_flash_image_get_op_status()
typedef struct {
    fimg_op_state_e op_state;   // The current operational state of the flash uf2 module.
    uint32_t update_family_id;  // The family_id of the update.
    uint32_t total_num_blocks;  // The total number of blocks to be updated.
    uint32_t rxed_block_count;  // The number of blocks received & processed (written to flash) thus far.
    uint32_t rxed_data_index;   // Count of the uf2 block byte data buffered but not yet written to flash (0-511).
} fimg_update_info_t;


/** \file pico/flash_image.h
 *  \defgroup pico_flash_image pico_flash_image
 *
 * NOTE: A raw/binary mode format API will be added in due course.
 *
 * A common library for writing a stream of octets, organised in `uf2`
 * blocks, into a flash partition.  Very typically, the image programmed will
 * be a new operating image or some other image required to persist in flash.
 *
 * The library will erase each flash sector as required when the data to be
 * written to flash reaches an address within a sector - if it has not yet been
 * erased.  The design supports programming integral uf2 blocks arriving in any
 * order, the blocks need *not* be in a monotonically increasing address order.
 *
 * Optionally, the specific *image type* permitted to be programmed can be
 * specified so that only uf2 blocks arriving with that specific `family_id`
 * will be programmed into flash.  The desired family_id is specified when
 * calling pico_flash_image_config_for_update().
 *
 * The selected family_id value will be checked against the field in the first
 * incoming uf2 block and if they do not match, programming will fail with an
 * error code, leaving the flash contents totally unchanged.
 *
 * If 0 is used as the family_id argument, programming of any incoming image
 * type is permitted, as extracted from the first incoming uf2 block header.
 *
 * Once a progamming sequence has started, the library checks for consistency
 * in the various fields of the uf2 blocks.  Programming will halt with an
 * error code if any inconsistency is detected.  In this case the flash
 * contents will be left in an undefined state.
 *
 * User application software is responsible for rebooting, or not, the device
 * after new image(s) has/have been programmed.  Newly programmed images
 * can be "tried" by the bootrom during the subsequent boot up using the
 * "try before you buy" mechanism, please see the RP2350 datasheet.
 *
 * The library maintains an internal operating state which can be checked by
 * application code using the pico_flash_image_get_op_status() function.
 * The operating states are:
 *
 * FIMG_STATE_NO_STORAGE    Non functional, no state data area has been supplied.
 *                          The user must call pico_flash_image_supply_storage()
 *                          to supply a memory area for the module's state data
 *                          before any other API functions can be used.
 *
 * FIMG_STATE_IDLE          Waiting for update initialisation (following supply
 *                          of a memory area for the module's state data).
 *                          The user must call pico_flash_image_config_for_update()
 *                          before any data can be written to flash.
 *
 * FIMG_STATE_WAITING_FOR_DATA  Update initialisation done, waiting for data.
 *                          The user must now call pico_flash_image_write_data(),
 *                          to start to write data to flash.
 *
 * FIMG_STATE_WRITING_IN_PROGRESS Flash update is in progress.
 *                          The user can call pico_flash_image_write_data(),
 *                          repeatedly, to write data to flash.
 *
 * FIMG_STATE_WRITE_SUCCESS Programming has completed successfully.
 *                          Another programming session can be started using
 *                          pico_flash_image_config_for_update(), or if programming
 *                          is complete, the state storage can be removed by
 *                          calling pico_flash_image_remove_storage()
 *
 * FIMG_STATE_WRITE_ERROR   An error has occurred during data transfer
 *                          and/or flash programming.  For subsequent
 *                          operation user must call
 *                          pico_flash_image_config_for_update() before a new
 *                          programming session can be started *or*
 *                          pico_flash_image_remove_storage() to de-initialise
 *                          the module.  The flash memory will be left in an
 *                          undefined state.
 */


/*! \brief Check if the flash image module has been assigned state data.
 *  \ingroup pico_flash_image
 *
 * This function can be called by application code to establish if the flash
 * image module has been assigned state data memory, allowing its operation,
 * either by the build being configured with a static allocation or with after
 * a previous call by application code to pico_flash_image_supply_storage()
 *
 * \return true     The module does have defined or configured state data
 *         false    The module does *not* have state data to operate with
 */
bool pico_flash_image_check_has_storage(void);


/*! \brief Supply a pointer to memory which is used to hold module state data.
 *  \ingroup pico_flash_image
 *
 * This function must be called once by application code before the rest of the
 * module's functional API will become usable.  The memory area provided must
 * be at  least `PICO_FLASH_IMAGE_WORKAREA_SIZE` in size, see definition above.
 *
 * NOTE: This function can be repeatedly called with the *same* storage area
 *       address argument _without_ a `PICO_ERROR_RESOURCE_IN_USE` type error
 *       being returned.
 *       The "re-supply" of the same area however will result in the operating
 *       state being set to `FIMG_STATE_IDLE` as this is the desired behaviour
 *       when a "new" storage area is supplied.
 *
 * \param state     Pointer to a work area used for the module's state data.
 * \param storage_size Size of the work area provided.
 *
 * \return PICO_OK  Following successful initialisation, an error code otherwise
 *         PICO_ERROR_INVALID_ARG      The supplied pointer to state data is NULL,
 *         PICO_ERROR_RESOURCE_IN_USE  Module already has state data allocation,
 *                                     and *not* the same as the supplied argument.
 *         PICO_ERROR_BUFFER_TOO_SMALL The supplied state data area is too small.
 */
int pico_flash_image_supply_storage(void *state, uint storage_size);


/*! \brief Remove the work area memory from the flash_image module's usage.
 *  \ingroup pico_flash_image
 *
 * This function may be called by application code after the module has been
 * used to remove the work area memory from the module's usage, effectively
 * de-initialising the module.
 *
 * Removal of the work area will allow malloced memory to be freed, if
 * required by the application and will also stop any further use of the
 * API until the module has again been supplied with a storage area.
 *
 * \param state     Pointer to a work area used for the module's state data.
 *
 * \return PICO_OK  On successful removal, an error code otherwise
 *         PICO_ERROR_INVALID_ARG       The supplied state pointer is NULL,
 *         PICO_ERROR_NOT_PERMITTED     No state data area to remove,
 *         PICO_ERROR_NOT_FOUND         Not the correct state date address.
 */
int pico_flash_image_remove_storage(void *state);


/*! \brief Get information about the current status of the flash uf2 module
 *  \ingroup pico_flash_image
 *
 * The function returns the current operational state of the flash uf2 module via
 * the fimg_update_info_t structure.
 *
 * If the module is updating, has successfully completed an update or has
 * encountered an update error, the function will also return the update's
 * family_id, the total number of blocks to be updated, the number of blocks
 * received and the count of the uf2 block's byte data buffered but not yet
 * written to flash.
 *
 * If the module is not updating, or has not just finished an update, 0 will
 * be returned in all information elements *other* than op_state.
 *
 * \return PICO_OK  Following filling the update info structure, an error code:
 *         PICO_ERROR_INVALID_ARG       The supplied pointer to update info is NULL.
 */
int pico_flash_image_get_op_status(fimg_update_info_t *fimg_update_info);


/*! \brief Stop the programming sequence and reset the module to idle state
 *  \ingroup pico_flash_image
 *
 * This function can be called at any time during the programming sequence to
 * stop the programming sequence and reset the module to idle state.
 *
 * NOTE: The flash partition contents will be left in an undefined state.
 *
 * \param state     Pointer to module's state holding work area
 *
 * \return PICO_OK  Following successful reset to idle, an error code otherwise
 *         PICO_ERROR_INVALID_ARG       The supplied state pointer is NULL,
 *         PICO_ERROR_NOT_PERMITTED     No state data area to operate on,
 *         PICO_ERROR_NOT_FOUND         Not the correct state data address.
 */
int pico_flash_image_reset_to_idle(void *state);


/*! \brief Initialise flash uf2 module ready for uf2 formatted stream to write.
 *  \ingroup pico_flash_image
 *
 * This function must be called exactly once before flash programming can
 * commence and after pico_flash_image_supply_storage() has been called.
 *
 * \param state     Pointer to module's state holding work area
 * \param update_family_id  If 0, no filter is applied.  Otherwise, only uf2
 *                          images with this family_id will be permitted to be
 *                          programmed.
 *
 * \return PICO_OK  Following successful initialisation, an error code otherwise
 *         PICO_ERROR_INVALID_ARG       The supplied state pointer is NULL,
 *         PICO_ERROR_NOT_PERMITTED     No state data area to operate on,
 *         PICO_ERROR_NOT_FOUND         Not the correct state data address.
 */
int pico_flash_image_config_for_update(void *state, uint32_t update_family_id);


/*! \brief Buffer and/or program all the incoming octet data to flash.
 *  \ingroup pico_flash_image
 *
 * The incoming data stream must be in uf2 block format, the data is buffered.
 * Each time a complete uf2 block is assembled, it is checked and the data
 * portion written to flash.  Flash sectors will be erased as necessary.
 *
 * Any remaining uf2 block data is buffered and will subsequently be written
 * to flash when that block is completed by the arrival of more data.
 *
 * The supplied data can be any length and the function will block until all
 * the data has been buffered and/or written - thus consuming all the data.
 *
 * This function can only be called after pico_flash_image_config_for_update()
 * It can then be called repeatedly until all the data has been written.
 *
 * Any errors will be reported by the function returning an error code.
 * The flash contents will be left in an undefined state.
 *
 * \param state     Pointer to module's state holding work area
 * \param data      Pointer to incoming (uf2 formatted) data
 * \param length    Length of incoming data
 *
 * \return PICO_OK  Following successful buffering/programing, or an error code:
 *         PICO_ERROR_INVALID_ARG       The supplied state pointer is NULL,
 *         PICO_ERROR_NOT_PERMITTED     No state data area to operate on,
 *         PICO_ERROR_NOT_FOUND         Not the correct state data address,
 *         PICO_ERROR_INVALID_DATA      Incoming data is not correctly formatted,
 *         PICO_ERROR_GENERIC           Flash erase or programming error occurred.
 */
int pico_flash_image_write_data(void *state, const char *data, uint length);


/*! \brief Check if the programming sequence is complete
 *  \ingroup pico_flash_image
 *
 * This function should be called after all octets have been programmed in flash.
 *
 * \param state     Pointer to module's state holding work area
 * \param complete  Pointer to a bool to indicate if the update is complete.
 * \param update_start_addr  Pointer to a uint32_t ready to hold the execution
 *                           address of the (just updated) image.
 *
 * \return PICO_OK  Following successful finalisation, an error code otherwise
 *         PICO_ERROR_INVALID_ARG       The supplied state pointer is NULL,
 *         PICO_ERROR_NOT_PERMITTED     No state data area to operate on,
 *         PICO_ERROR_NOT_FOUND         Not the correct state data address.
 */
int pico_flash_image_check_write_complete(void *state,
                                          bool *complete,
                                          uint32_t *update_start_addr);

#ifdef __cplusplus
}
#endif

#endif // _PICO_FLASH_IMAGE_H
