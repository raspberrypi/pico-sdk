/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _BOOT_PICOBOOT_H
#define _BOOT_PICOBOOT_H

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#ifndef NO_PICO_PLATFORM
#include "pico/platform.h"
#endif

/** \file picoboot.h
*  \defgroup boot_picoboot_headers boot_picoboot_headers
*
* \brief Header file for the PICOBOOT USB interface exposed by an RP2xxx chip in BOOTSEL mode
*/

#include "picoboot_constants.h"

#define PICOBOOT_MAGIC 0x431fd10bu

// --------------------------------------------
// CONTROL REQUESTS FOR THE PICOBOOT INTERFACE
// --------------------------------------------

// size 0 OUT - un-stall EPs and reset
#define PICOBOOT_IF_RESET 0x41

// size 16 IN - return the status of the last command
#define PICOBOOT_IF_CMD_STATUS 0x42

// --------------------------------------------------
// COMMAND REQUESTS SENT TO THE PICOBOOT OUT ENDPOINT
// --------------------------------------------------
//
// picoboot_cmd structure of size 32 is sent to OUT endpoint
// transfer_length bytes are transferred via IN/OUT
// device responds on success with 0 length ACK packet set via OUT/IN
// device may stall the transferring endpoint in case of error

enum picoboot_cmd_id {
    PC_EXCLUSIVE_ACCESS = 0x1,
    PC_REBOOT = 0x2, // RP2040 only
    PC_FLASH_ERASE = 0x3,
    PC_READ = 0x84, // either ROM or RAM or FLASH
    PC_WRITE = 0x5, // either RAM or FLASH (does no erase)
    PC_EXIT_XIP = 0x6, // has no additional parameters (bCmdSize == 0)
    PC_ENTER_CMD_XIP = 0x7, // has no additional parameters (bCmdSize == 0)
    PC_EXEC = 0x8, // RP2040 only
    PC_VECTORIZE_FLASH = 0x9, // RP2040 only
    // RP2350 only below here
    PC_REBOOT2 = 0xa,
    PC_GET_INFO = 0x8b,
    PC_OTP_READ = 0x8c,
    PC_OTP_WRITE = 0xd,
};

enum picoboot_status {
    PICOBOOT_OK = 0,
    PICOBOOT_UNKNOWN_CMD = 1,
    PICOBOOT_INVALID_CMD_LENGTH = 2,
    PICOBOOT_INVALID_TRANSFER_LENGTH = 3,
    PICOBOOT_INVALID_ADDRESS = 4,
    PICOBOOT_BAD_ALIGNMENT = 5,
    PICOBOOT_INTERLEAVED_WRITE = 6,
    PICOBOOT_REBOOTING = 7,
    PICOBOOT_UNKNOWN_ERROR = 8,
    PICOBOOT_INVALID_STATE = 9,
    PICOBOOT_NOT_PERMITTED = 10,
    PICOBOOT_INVALID_ARG = 11,
    PICOBOOT_BUFFER_TOO_SMALL = 12,
    PICOBOOT_PRECONDITION_NOT_MET = 13,
    PICOBOOT_MODIFIED_DATA = 14,
    PICOBOOT_INVALID_DATA = 15,
    PICOBOOT_NOT_FOUND = 16,
    PICOBOOT_UNSUPPORTED_MODIFICATION = 17,
};

/*! \brief Parameters for a reboot command (RP2040 only)
 *  \ingroup boot_picoboot_headers
 *
 * Sent as the argument payload of a PC_REBOOT command.
 */
struct __packed picoboot_reboot_cmd {
    uint32_t dPC; ///< Program counter to reboot to; 0 means reset into the regular boot path, otherwise must be a RAM address
    uint32_t dSP; ///< Stack pointer value at reboot; ignored unless dPC is a RAM address
    uint32_t dDelayMS; ///< Delay in milliseconds before rebooting
};

/*! \brief Parameters for an extended reboot command (not available on RP2040)
 *  \ingroup boot_picoboot_headers
 *
 * Sent as the argument payload of a PC_REBOOT2 command.
 */
struct __packed picoboot_reboot2_cmd {
    uint32_t dFlags; ///< Reboot flags controlling the boot path
    uint32_t dDelayMS; ///< Delay in milliseconds before rebooting
    uint32_t dParam0; ///< First reboot parameter (interpretation depends on flags)
    uint32_t dParam1; ///< Second reboot parameter (interpretation depends on flags)
};

/*! \brief Parameters for a command that operates on a single address (RP2040 only)
 *  \ingroup boot_picoboot_headers
 *
 * Sent as the argument payload of PC_EXEC and PC_VECTORIZE_FLASH commands,
 * which are not supported on RP2350.
 */
struct __packed picoboot_address_only_cmd {
    uint32_t dAddr; ///< Target address
};

/*! \brief Parameters for a command that operates on an address range
 *  \ingroup boot_picoboot_headers
 *
 * Sent as the argument payload of PC_READ, PC_WRITE, and PC_FLASH_ERASE commands.
 */
struct __packed picoboot_range_cmd {
    uint32_t dAddr; ///< Start address of the range
    uint32_t dSize; ///< Size of the range in bytes
};

// remains defined for backwards compatibility with RP2350 bootrom builds
struct __packed picoboot_exec2_cmd {
    uint32_t dummy;
};

/*! \brief Exclusivity level for a PC_EXCLUSIVE_ACCESS command
 *  \ingroup boot_picoboot_headers
 */
enum picoboot_exclusive_type {
    NOT_EXCLUSIVE = 0,   ///< No restriction on USB Mass Storage operation
    EXCLUSIVE,           ///< Disable USB Mass Storage writes (any active UF2 download will be aborted)
    EXCLUSIVE_AND_EJECT  ///< Lock out USB Mass Storage by marking the drive media as not present (eject the drive)
};

/*! \brief Parameters for an exclusive-access command
 *  \ingroup boot_picoboot_headers
 *
 * Sent as the argument payload of a PC_EXCLUSIVE_ACCESS command.
 */
struct __packed picoboot_exclusive_cmd {
    uint8_t bExclusive; ///< Exclusivity level; one of the picoboot_exclusive_type values
};

/*! \brief Parameters for an OTP read or write command (not available on RP2040)
 *  \ingroup boot_picoboot_headers
 *
 * Sent as the argument payload of PC_OTP_READ and PC_OTP_WRITE commands.
 */
struct __packed picoboot_otp_cmd {
    uint16_t wRow; ///< OTP row index to start from
    uint16_t wRowCount; ///< Number of rows to transfer
    uint8_t bEcc; ///< Non-zero to use ECC (16-bit per register); zero for raw 24-bit access (stored as 32-bit)
};

/*! \brief Parameters for a get-info command (not available on RP2040)
 *  \ingroup boot_picoboot_headers
 *
 * Sent as the argument payload of a PC_GET_INFO command.
 */
struct __packed picoboot_get_info_cmd {
    uint8_t bType; ///< Info type selector
    uint8_t bParam; ///< Unused
    uint16_t wParam; ///< Unused
    uint32_t dParams[3]; ///< Additional parameters for the selected info type
};

// little endian
/*! \brief A PICOBOOT command packet sent to the OUT endpoint
 *  \ingroup boot_picoboot_headers
 *
 * A 32-byte packet written to the PICOBOOT OUT endpoint to initiate any
 * supported command.  After sending this packet, transfer_length bytes are
 * exchanged via the IN or OUT endpoint as appropriate, followed by a
 * zero-length ACK packet.
 */
struct __packed __aligned(4) picoboot_cmd {
    uint32_t dMagic; ///< Must be PICOBOOT_MAGIC
    uint32_t dToken; ///< Caller-chosen identifier used to correlate this command with its status response
    uint8_t bCmdId; ///< Command identifier from picoboot_cmd_id; top bit set indicates an IN transfer
    uint8_t bCmdSize; ///< Number of valid argument bytes within the args union
    uint16_t _unused; ///< Reserved; must be zero
    uint32_t dTransferLength; ///< Length of the subsequent IN/OUT data transfer, or 0 if none
    union {
        uint8_t args[16]; ///< Raw argument bytes
        struct picoboot_reboot_cmd reboot_cmd; ///< Arguments for PC_REBOOT
        struct picoboot_range_cmd range_cmd; ///< Arguments for PC_READ, PC_WRITE, and PC_FLASH_ERASE
        struct picoboot_address_only_cmd address_only_cmd; ///< Arguments for PC_EXEC and PC_VECTORIZE_FLASH
        struct picoboot_exclusive_cmd exclusive_cmd; ///< Arguments for PC_EXCLUSIVE_ACCESS
        struct picoboot_reboot2_cmd reboot2_cmd; ///< Arguments for PC_REBOOT2
        struct picoboot_otp_cmd otp_cmd; ///< Arguments for PC_OTP_READ and PC_OTP_WRITE
        struct picoboot_get_info_cmd get_info_cmd; ///< Arguments for PC_GET_INFO
    };
};
static_assert(32 == sizeof(struct picoboot_cmd), "picoboot_cmd must be 32 bytes big");

/*! \brief Status response returned by the PICOBOOT_IF_CMD_STATUS control request
 *  \ingroup boot_picoboot_headers
 *
 * A 16-byte structure read back from the device to determine the outcome of
 * the most recently issued command.
 */
struct __packed __aligned(4) picoboot_cmd_status {
    uint32_t dToken; ///< Token copied from the corresponding picoboot_cmd
    uint32_t dStatusCode; ///< Result code; one of the picoboot_status values
    uint8_t bCmdId; ///< Command identifier of the command this status relates to
    uint8_t bInProgress; ///< Non-zero if the command is still being processed
    uint8_t _pad[6]; ///< Padding to reach 16 bytes
};

static_assert(16 == sizeof(struct picoboot_cmd_status), "picoboot_cmd_status must be 16 bytes big");

#endif
