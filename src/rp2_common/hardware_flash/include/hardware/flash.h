/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _HARDWARE_FLASH_H
#define _HARDWARE_FLASH_H

#include "pico.h"

/** \file flash.h
 *  \defgroup hardware_flash hardware_flash
 *
 * \brief Low level flash programming and erase API
 *
 * Note these functions are *unsafe* if you are using both cores, and the other
 * is executing from flash concurrently with the operation. In this case, you
 * must perform your own synchronisation to make sure that no XIP accesses take
 * place during flash programming. One option is to use the
 * \ref multicore_lockout functions.
 *
 * Likewise they are *unsafe* if you have interrupt handlers or an interrupt
 * vector table in flash, so you must disable interrupts before calling in
 * this case.
 *
 * If PICO_NO_FLASH=1 is not defined (i.e. if the program is built to run from
 * flash) then these functions will make a static copy of the second stage
 * bootloader in SRAM, and use this to reenter execute-in-place mode after
 * programming or erasing flash, so that they can safely be called from
 * flash-resident code.
 *
 * \subsection flash_example Example
 * \include flash_program.c
 */

// PICO_CONFIG: PARAM_ASSERTIONS_ENABLED_HARDWARE_FLASH, Enable/disable assertions in the hardware_flash module, type=bool, default=0, group=hardware_flash
#ifndef PARAM_ASSERTIONS_ENABLED_HARDWARE_FLASH
#ifdef PARAM_ASSERTIONS_ENABLED_FLASH // backwards compatibility with SDK < 2.0.0
#define PARAM_ASSERTIONS_ENABLED_HARDWARE_FLASH PARAM_ASSERTIONS_ENABLED_FLASH
#else
#define PARAM_ASSERTIONS_ENABLED_HARDWARE_FLASH 0
#endif
#endif
#define FLASH_PAGE_SIZE (1u << 8)
#define FLASH_SECTOR_SIZE (1u << 12)
#define FLASH_BLOCK_SIZE (1u << 16)

#ifndef FLASH_UNIQUE_ID_SIZE_BYTES
#define FLASH_UNIQUE_ID_SIZE_BYTES 8
#endif

// PICO_CONFIG: PICO_FLASH_SIZE_BYTES, size of primary flash in bytes, type=int, default=Usually provided via board header, group=hardware_flash

#ifdef __cplusplus
extern "C" {
#endif

/*! \brief  Erase areas of flash
 *  \ingroup hardware_flash
 *
 * \param flash_offs Offset into flash, in bytes, to start the erase. Must be aligned to a 4096-byte flash sector.
 * \param count Number of bytes to be erased. Must be a multiple of 4096 bytes (one sector).
 *
 * @note Erasing a flash sector sets all the bits in all the pages in that sector to one.
 * You can then "program" flash pages in the sector to turn some of the bits to zero.
 * Once a bit is set to zero it can only be changed back to one by erasing the whole sector again.
 */
void flash_range_erase(uint32_t flash_offs, size_t count);

/*! \brief  Program flash
 *  \ingroup hardware_flash
 *
 * \param flash_offs Flash address of the first byte to be programmed. Must be aligned to a 256-byte flash page.
 * \param data Pointer to the data to program into flash
 * \param count Number of bytes to program. Must be a multiple of 256 bytes (one page).
 *
 * @note: Programming a flash page effectively changes some of the bits from one to zero.
 * The only way to change a zero bit back to one is to "erase" the whole sector that the page resides in.
 * So you may need to make sure you have called flash_range_erase before calling flash_range_program.
 */

void flash_range_program(uint32_t flash_offs, const uint8_t *data, size_t count);

/*! \brief Get flash unique 64 bit identifier
 *  \ingroup hardware_flash
 *
 * Use a standard 4Bh RUID instruction to retrieve the 64 bit unique
 * identifier from a flash device attached to the QSPI interface. Since there
 * is a 1:1 association between the MCU and this flash, this also serves as a
 * unique identifier for the board.
 *
 *  \param id_out Pointer to an 8-byte buffer to which the ID will be written
 */
void flash_get_unique_id(uint8_t *id_out);

/*! \brief Execute bidirectional flash command
 *  \ingroup hardware_flash
 *
 * Low-level function to execute a serial command on a flash device attached
 * to the QSPI interface. Bytes are simultaneously transmitted and received
 * from txbuf and to rxbuf. Therefore, both buffers must be the same length,
 * count, which is the length of the overall transaction. This is useful for
 * reading metadata from the flash chip, such as device ID or SFDP
 * parameters.
 *
 * The XIP cache is flushed following each command, in case flash state
 * has been modified. Like other hardware_flash functions, the flash is not
 * accessible for execute-in-place transfers whilst the command is in
 * progress, so entering a flash-resident interrupt handler or executing flash
 * code on the second core concurrently will be fatal. To avoid these pitfalls
 * it is recommended that this function only be used to extract flash metadata
 * during startup, before the main application begins to run: see the
 * implementation of pico_get_unique_id() for an example of this.
 *
 *  \param txbuf Pointer to a byte buffer which will be transmitted to the flash
 *  \param rxbuf Pointer to a byte buffer where data received from the flash will be written. txbuf and rxbuf may be the same buffer.
 *  \param count Length in bytes of txbuf and of rxbuf
 */
void flash_do_cmd(const uint8_t *txbuf, uint8_t *rxbuf, size_t count);

void flash_flush_cache(void);

#if !PICO_RP2040
typedef enum {
    FLASH_DEVINFO_SIZE_NONE = 0x0,
    FLASH_DEVINFO_SIZE_8K = 0x1,
    FLASH_DEVINFO_SIZE_16K = 0x2,
    FLASH_DEVINFO_SIZE_32K = 0x3,
    FLASH_DEVINFO_SIZE_64K = 0x4,
    FLASH_DEVINFO_SIZE_128K = 0x5,
    FLASH_DEVINFO_SIZE_256K = 0x6,
    FLASH_DEVINFO_SIZE_512K = 0x7,
    FLASH_DEVINFO_SIZE_1M = 0x8,
    FLASH_DEVINFO_SIZE_2M = 0x9,
    FLASH_DEVINFO_SIZE_4M = 0xa,
    FLASH_DEVINFO_SIZE_8M = 0xb,
    FLASH_DEVINFO_SIZE_16M = 0xc,
    FLASH_DEVINFO_SIZE_MAX = 0xc
} flash_devinfo_size_t;

/*! \brief Convert a flash/PSRAM size enum to an integer size in bytes
 *  \ingroup hardware_flash
 */
static inline uint32_t flash_devinfo_size_to_bytes(flash_devinfo_size_t size) {
    if (size == FLASH_DEVINFO_SIZE_NONE) {
        return 0;
    } else {
        return 4096u << (uint)size;
    }
}

/*! \brief Convert an integer flash/PSRAM size in bytes to a size enum, as
  !  stored in OTP and used by the ROM.
 *  \ingroup hardware_flash
 */
static inline flash_devinfo_size_t flash_devinfo_bytes_to_size(uint32_t bytes) {
    // Must be zero or a power of two
    valid_params_if(HARDWARE_FLASH, (bytes & (bytes - 1)) == 0u);
    uint sectors = bytes / 4096u;
    if (sectors <= 1u) {
        return FLASH_DEVINFO_SIZE_NONE;
    } else {
        return (flash_devinfo_size_t) __builtin_ctz(sectors);
    }
}

/*! \brief Get the size of the QSPI device attached to chip select cs, according to FLASH_DEVINFO
 *  \ingroup hardware_flash
 *
 * \param cs Chip select index: 0 is QMI chip select 0 (QSPI CS pin), 1 is QMI chip select 1.
 *
 * The bootrom reads the FLASH_DEVINFO OTP data entry from OTP into boot RAM during startup. This
 * contains basic information about the flash device which can be queried without communicating
 * with the external device.(There are several methods to determine the size of a QSPI device over
 * QSPI, but none are universally supported.)
 *
 * Since the FLASH_DEVINFO information is stored in boot RAM at runtime, it can be updated. Updates
 * made in this way persist until the next reboot. The ROM uses this device information to control
 * some low-level flash API behaviour, such as issuing an XIP exit sequence to CS 1 if its size is
 * nonzero.
 *
 * If the macro PICO_FLASH_SIZE_BYTES is specified, this overrides the value for chip select 0. This
 * can be specified in a board header if a board is always equipped with the same size of flash.
 */
flash_devinfo_size_t flash_devinfo_get_cs_size(uint cs);

/*! \brief Update the size of the QSPI device attached to chip select cs in the runtime copy
 *         of FLASH_DEVINFO.
 *
 *  \ingroup hardware_flash
 *
 * \param cs Chip select index: 0 is QMI chip select 0 (QSPI CS pin), 1 is QMI chip select 1.
 *
 * \param size The size of the attached device, or FLASH_DEVINFO_SIZE_NONE if there is none on this
 *  chip select.
 *
 * The bootrom maintains a copy in boot RAM of the FLASH_DEVINFO information read from OTP during
 * startup. This function updates that copy to reflect runtime information about the sizes of
 * attached QSPI devices.
 *
 * This controls the behaviour of some ROM flash APIs, such as bounds checking addresses for
 * erase/programming in the checked_flash_op() API, or issuing an XIP exit sequence to CS 1 in
 * flash_exit_xip() if the size is nonzero.
 */
void flash_devinfo_set_cs_size(uint cs, flash_devinfo_size_t size);

/*! \brief Check whether all attached devices support D8h block erase with 64k size, according to
 *         FLASH_DEVINFO.
 *
 *  \ingroup hardware_flash
 *
 * This controls whether checked_flash_op() ROM API uses D8h 64k block erase where possible, for
 * faster erase times. If not, this ROM API always uses 20h 4k sector erase.
 *
 * The bootrom loads this flag from the OTP FLASH_DEVINFO data entry during startup, and stores it
 * in boot RAM. You can update the boot RAM copy based on runtime knowledge of the attached QSPI
 * devices.
 */
bool flash_devinfo_get_d8h_erase_supported(void);

/*! \brief Specify whether all attached devices support D8h block erase with 64k size, in the
 *         runtime copy of FLASH_DEVINFO
 *
 *  \ingroup hardware_flash
 *
 * This function updates the boot RAM copy of OTP FLASH_DEVINFO. The flag passed here is visible to
 * ROM APIs, and is also returned in the next call to flash_devinfo_get_d8h_erase_supported()
 */
void flash_devinfo_set_d8h_erase_supported(bool supported);

/*! \brief Check the GPIO allocated for each chip select, according to FLASH_DEVINFO
 *  \ingroup hardware_flash
 *
 * \param cs Chip select index (only the value 1 is supported on RP2350)
 */
uint flash_devinfo_get_cs_gpio(uint cs);

/*! \brief Update the GPIO allocated for each chip select in the runtime copy of FLASH_DEVINFO
 *  \ingroup hardware_flash
 *
 * \param cs Chip select index (only the value 1 is supported on RP2350)
 *
 * \param gpio GPIO index (must be less than NUM_BANK0_GPIOS)
 */
void flash_devinfo_set_cs_gpio(uint cs, uint gpio);

#endif // !PICO_RP2040

#ifdef __cplusplus
}
#endif

#endif
