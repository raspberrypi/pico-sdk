/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_BINARY_INFO_STRUCTURE_H
#define _PICO_BINARY_INFO_STRUCTURE_H

// NOTE: This file may be included by non SDK code, so does not use SDK includes

// NOTE: ALL CHANGES MUST BE BACKWARDS COMPATIBLE

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#ifndef __packed
#define __packed __attribute__((packed))
#endif

typedef struct _binary_info_core binary_info_t;

#define BINARY_INFO_TYPE_RAW_DATA 1
#define BINARY_INFO_TYPE_SIZED_DATA 2
#define BINARY_INFO_TYPE_BINARY_INFO_LIST_ZERO_TERMINATED 3
#define BINARY_INFO_TYPE_BSON 4
#define BINARY_INFO_TYPE_ID_AND_INT 5
#define BINARY_INFO_TYPE_ID_AND_STRING 6
// traditional block device
#define BINARY_INFO_TYPE_BLOCK_DEVICE 7
#define BINARY_INFO_TYPE_PINS_WITH_FUNC 8
#define BINARY_INFO_TYPE_PINS_WITH_NAME 9
#define BINARY_INFO_TYPE_NAMED_GROUP 10
#define BINARY_INFO_TYPE_PTR_INT32_WITH_NAME 11
#define BINARY_INFO_TYPE_PTR_STRING_WITH_NAME 12
#define BINARY_INFO_TYPE_PINS64_WITH_FUNC 13
#define BINARY_INFO_TYPE_PINS64_WITH_NAME 14

// note plan is to reserve c1 = 0->31 for "collision tags"; i.e.
// for which you should always use random IDs with the binary_info,
// giving you 4 + 8 + 32 = 44 bits to avoid collisions
#define BINARY_INFO_MAKE_TAG(c1, c2) ((((uint)c2&0xffu)<<8u)|((uint)c1&0xffu))

// Raspberry Pi defined. do not use
#define BINARY_INFO_TAG_RASPBERRY_PI BINARY_INFO_MAKE_TAG('R','P')

#define BINARY_INFO_ID_RP_PROGRAM_NAME 0x02031c86
#define BINARY_INFO_ID_RP_PROGRAM_VERSION_STRING 0x11a9bc3a
#define BINARY_INFO_ID_RP_PROGRAM_BUILD_DATE_STRING 0x9da22254
#define BINARY_INFO_ID_RP_BINARY_END 0x68f465de
#define BINARY_INFO_ID_RP_PROGRAM_URL 0x1856239a
#define BINARY_INFO_ID_RP_PROGRAM_DESCRIPTION 0xb6a07c19
#define BINARY_INFO_ID_RP_PROGRAM_FEATURE 0xa1f4b453
#define BINARY_INFO_ID_RP_PROGRAM_BUILD_ATTRIBUTE 0x4275f0d3
#define BINARY_INFO_ID_RP_SDK_VERSION 0x5360b3ab
#define BINARY_INFO_ID_RP_PICO_BOARD 0xb63cffbb
#define BINARY_INFO_ID_RP_BOOT2_NAME 0x7f8882e1

#if PICO_ON_DEVICE
#define bi_ptr_of(x) x *
#else
#define bi_ptr_of(x) uint32_t
#endif

/*! \brief Common header fields shared by all binary info entries
 *  \ingroup pico_binary_info
 *
 * Every binary info structure begins with this core header, which identifies
 * the entry type and the tag namespace it belongs to.
 */
typedef struct __packed _binary_info_core {
        uint16_t type; ///< Binary info entry type (one of BINARY_INFO_TYPE_*)
        uint16_t tag;  ///< Namespace tag identifying the entry's owner
} binary_info_core_t;

/*! \brief Binary info entry carrying raw, uninterpreted byte data
 *  \ingroup pico_binary_info
 */
typedef struct __packed _binary_info_raw_data {
        struct _binary_info_core core;  ///< Common binary info header
        uint8_t bytes[1];               ///< Raw byte payload (variable length)
} binary_info_raw_data_t;

/*! \brief Binary info entry carrying a length-prefixed byte buffer
 *  \ingroup pico_binary_info
 */
typedef struct __packed _binary_info_sized_data {
        struct _binary_info_core core;  ///< Common binary info header
        uint32_t length;                ///< Number of bytes in the payload
        uint8_t bytes[1];               ///< Byte payload (variable length)
} binary_info_sized_data_t;

/*! \brief Binary info entry holding a null-terminated list of binary info pointers
 *  \ingroup pico_binary_info
 */
typedef struct __packed _binary_info_list_zero_terminated {
        struct _binary_info_core core;       ///< Common binary info header
        bi_ptr_of(binary_info_t) list;       ///< Pointer to the null-terminated list of binary info entries
} binary_info_list_zero_terminated_t;

/*! \brief Binary info entry associating a 32-bit integer value with an ID
 *  \ingroup pico_binary_info
 */
typedef struct __packed _binary_info_id_and_int {
        struct _binary_info_core core;  ///< Common binary info header
        uint32_t id;                    ///< Identifier for the value (one of BINARY_INFO_ID_*)
        int32_t value;                  ///< The integer value associated with the ID
} binary_info_id_and_int_t;

/*! \brief Binary info entry associating a string value with an ID
 *  \ingroup pico_binary_info
 */
typedef struct __packed _binary_info_id_and_string {
        struct _binary_info_core core;       ///< Common binary info header
        uint32_t id;                         ///< Identifier for the value (one of BINARY_INFO_ID_*)
        bi_ptr_of(const char) value;         ///< Pointer to the null-terminated string value
} binary_info_id_and_string_t;

/*! \brief Binary info entry holding a pointer to a named 32-bit integer variable
 *  \ingroup pico_binary_info
 */
typedef struct __packed _binary_info_ptr_int32_with_name {
        struct _binary_info_core core;       ///< Common binary info header
        int32_t id;                          ///< Identifier for the variable
        bi_ptr_of(const int) value;          ///< Pointer to the integer variable
        bi_ptr_of(const char) label;         ///< Pointer to the null-terminated display label
} binary_info_ptr_int32_with_name_t;

/*! \brief Binary info entry holding a pointer to a named string variable
 *  \ingroup pico_binary_info
 */
typedef struct __packed _binary_info_ptr_string_with_name {
        struct _binary_info_core core;       ///< Common binary info header
        int32_t id;                          ///< Identifier for the variable
        bi_ptr_of(const char) value;         ///< Pointer to the string variable
        bi_ptr_of(const char) label;         ///< Pointer to the null-terminated display label
        uint32_t len;                        ///< Maximum length of the string buffer
} binary_info_ptr_string_with_name_t;

/*! \brief Binary info entry describing a block device in the binary
 *  \ingroup pico_binary_info
 */
typedef struct __packed _binary_info_block_device {
        struct _binary_info_core core;       ///< Common binary info header
        bi_ptr_of(const char) name;          ///< optional static name (independent of what is formatted)
        uint32_t address;                    ///< Start address of the block device in flash
        uint32_t size;                       ///< Size of the block device in bytes
        bi_ptr_of(binary_info_t) extra;      ///< additional info
        uint16_t flags;                      ///< Block device capability flags (BINARY_INFO_BLOCK_DEV_FLAG_*)
} binary_info_block_device_t;

#define BI_PINS_ENCODING_RANGE 1
#define BI_PINS_ENCODING_MULTI 2

/*! \brief Binary info entry describing one or more GPIO pins and their assigned function, supporting pin numbers <32, and up to 5 pins
 *  \ingroup pico_binary_info
 */
typedef struct __packed _binary_info_pins_with_func {
    struct _binary_info_core core;  ///< Common binary info header
    // p4_5 : p3_5 : p2_5 : p1_5 : p0_5 : func_4 : 010_3 //individual pins p0,p1,p2,p3,p4 ... if fewer than 5 then duplicate last pin
    //                    phi_5 : plo_5 : func_4 : 001_3 // pin range plo-phi inclusive
    uint32_t pin_encoding;          ///< Encoded pin numbers and function (see BI_PINS_ENCODING_* for format)
} binary_info_pins_with_func_t;

/*! \brief Binary info entry describing one or more GPIO pins and their assigned function, supporting pin numbers <256, and up to 7 pins
 *  \ingroup pico_binary_info
 */
typedef struct __packed _binary_info_pins64_with_func {
    struct _binary_info_core core;  ///< Common binary info header
    // p6_8 : p5_8 : p4_8 : p3_8 : p2_8 : p1_8 : p0_8 : func_5 : 010_3 //individual pins p0,p1,p2 ... if fewer than 7 then duplicate last pin
    //                    phi_8 : plo_8 : func_5 : 001_3 // pin range plo-phi inclusive
    uint64_t pin_encoding;          ///< Encoded pin numbers and function for more than 32 pins (see BI_PINS_ENCODING_* for format)
} binary_info_pins64_with_func_t;

/*! \brief Binary info entry associating a human-readable label with a set of GPIO pins
 *  \ingroup pico_binary_info
 */
typedef struct __packed _binary_info_pins_with_name {
    struct _binary_info_core core;       ///< Common binary info header
    uint32_t pin_mask;                   ///< Bitmask of GPIO pins covered by this entry
    bi_ptr_of(const char) label;         ///< Pointer to the null-terminated pin label
} binary_info_pins_with_name_t;

/*! \brief Binary info entry associating a human-readable label with a set of GPIO pins (up to 64)
 *  \ingroup pico_binary_info
 */
typedef struct __packed _binary_info_pins64_with_name {
    struct _binary_info_core core;       ///< Common binary info header
    uint64_t pin_mask;                   ///< Bitmask of GPIO pins covered by this entry (up to 64 pins)
    bi_ptr_of(const char) label;         ///< Pointer to the null-terminated pin label
} binary_info_pins64_with_name_t;

#define BI_NAMED_GROUP_SHOW_IF_EMPTY   0x0001  // default is to hide
#define BI_NAMED_GROUP_SEPARATE_COMMAS 0x0002  // default is newlines
#define BI_NAMED_GROUP_SORT_ALPHA      0x0004  // default is no sort
#define BI_NAMED_GROUP_ADVANCED        0x0008  // if set, then only shown in say info -a

/*! \brief Binary info entry defining a named group of related binary info entries
 *  \ingroup pico_binary_info
 *
 * Named groups allow tools to present related binary info entries together
 * under a common label, with configurable display behaviour.
 */
typedef struct __packed _binary_info_named_group {
    struct _binary_info_core core;       ///< Common binary info header
    uint32_t parent_id;                  ///< ID of the parent group (0 for top-level)
    uint16_t flags;                      ///< Display flags for the group (BI_NAMED_GROUP_*)
    uint16_t group_tag;                  ///< Tag namespace used by entries within this group
    uint32_t group_id;                   ///< Unique identifier for this group
    bi_ptr_of(const char) label;         ///< Pointer to the null-terminated group label
} binary_info_named_group_t;

enum {
    BINARY_INFO_BLOCK_DEV_FLAG_READ = 1 << 0, // if not readable, then it is basically hidden, but tools may choose to avoid overwriting it
    BINARY_INFO_BLOCK_DEV_FLAG_WRITE = 1 << 1,
    BINARY_INFO_BLOCK_DEV_FLAG_REFORMAT = 1 << 2, // may be reformatted..

    BINARY_INFO_BLOCK_DEV_FLAG_PT_UNKNOWN = 0 << 4, // unknown free to look
    BINARY_INFO_BLOCK_DEV_FLAG_PT_MBR = 1 << 4, // expect MBR
    BINARY_INFO_BLOCK_DEV_FLAG_PT_GPT = 2 << 4, // expect GPT
    BINARY_INFO_BLOCK_DEV_FLAG_PT_NONE = 3 << 4, // no partition table
};

#ifdef __cplusplus
}
#endif
#endif
