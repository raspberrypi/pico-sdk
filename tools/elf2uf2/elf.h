/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _ELF_H
#define _ELF_H

#include <stdint.h>

#define ELF_MAGIC 0x464c457fu

#define EM_ARM 0x28u

#define EF_ARM_ABI_FLOAT_HARD 0x00000400u

#define PT_LOAD 0x00000001u

/* Note, only little endian ELFs handled */
#pragma pack(push, 1)
struct elf_header {
    le_uint32_t magic;
    uint8_t     arch_class;
    uint8_t     endianness;
    uint8_t     version;
    uint8_t     abi;
    uint8_t     abi_version;
    uint8_t     _pad[7];
    le_uint16_t type;
    le_uint16_t machine;
    le_uint32_t version2;
};

struct elf32_header {
    struct elf_header common;
    le_uint32_t entry;
    le_uint32_t ph_offset;
    le_uint32_t sh_offset;
    le_uint32_t flags;
    le_uint16_t eh_size;
    le_uint16_t ph_entry_size;
    le_uint16_t ph_num;
    le_uint16_t sh_entry_size;
    le_uint16_t sh_num;
    le_uint16_t sh_str_index;
};

struct elf32_ph_entry {
    le_uint32_t type;
    le_uint32_t offset;
    le_uint32_t vaddr;
    le_uint32_t paddr;
    le_uint32_t filez;
    le_uint32_t memsz;
    le_uint32_t flags;
    le_uint32_t align;
};
#pragma pack(pop)

#endif
