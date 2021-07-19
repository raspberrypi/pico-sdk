/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cstdio>
#include <map>
#include <vector>
#include <cstring>
#include <cstdarg>
#include <algorithm>
#include "boot/uf2.h"
#include "elf.h"

typedef unsigned int uint;

#define ERROR_ARGS -1
#define ERROR_FORMAT -2
#define ERROR_INCOMPATIBLE -3
#define ERROR_READ_FAILED -4
#define ERROR_WRITE_FAILED -5

static char error_msg[512];
static bool verbose;

static uint32_t crc32_calculator (uint8_t *pbuf, uint_fast32_t size, uint32_t init);

static int fail(int code, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(error_msg, sizeof(error_msg), format, args);
    va_end(args);
    return code;
}

static int fail_read_error() {
    return fail(ERROR_READ_FAILED, "Failed to read input file");
}

static int fail_write_error() {
    return fail(ERROR_WRITE_FAILED, "Failed to write output file");
}

// we require 256 (as this is the page size supported by the device)
#define LOG2_PAGE_SIZE 8u
#define PAGE_SIZE (1u << LOG2_PAGE_SIZE)

struct address_range {
    enum type {
        CONTENTS,     // may have contents
        NO_CONTENTS,  // must be uninitialized
        IGNORE        // will be ignored
    };
    address_range(uint32_t from, uint32_t to, type type) : from(from), to(to), type(type) {}
    address_range() : address_range(0, 0, IGNORE) {}
    type type;
    uint32_t to;
    uint32_t from;
};

typedef std::vector<address_range> address_ranges;

#define MAIN_RAM_START        0x20000000u
#define MAIN_RAM_END          0x20042000u
#define FLASH_START           0x10000000u
#define FLASH_END             0x15000000u
#define XIP_SRAM_START        0x15000000u
#define XIP_SRAM_END          0x15004000u
#define MAIN_RAM_BANKED_START 0x21000000u
#define MAIN_RAM_BANKED_END   0x21040000u
#define BOOT2_SIZE_BYTES      0x00000100u

const address_ranges rp2040_address_ranges_flash {
    address_range(FLASH_START, FLASH_END, address_range::type::CONTENTS),
    address_range(MAIN_RAM_START, MAIN_RAM_END, address_range::type::NO_CONTENTS),
    address_range(MAIN_RAM_BANKED_START, MAIN_RAM_BANKED_END, address_range::type::NO_CONTENTS)
};

const address_ranges rp2040_address_ranges_ram {
    address_range(MAIN_RAM_START, MAIN_RAM_END, address_range::type::CONTENTS),
    address_range(XIP_SRAM_START, XIP_SRAM_END, address_range::type::CONTENTS),
    address_range(0x00000000u, 0x00004000u, address_range::type::IGNORE) // for now we ignore the bootrom if present
};

struct page_fragment {
    page_fragment(uint32_t file_offset, uint32_t page_offset, uint32_t bytes) : file_offset(file_offset), page_offset(page_offset), bytes(bytes) {}
    uint32_t file_offset;
    uint32_t page_offset;
    uint32_t bytes;
};

static int usage() {
    fprintf(stderr, "Usage: elf2uf2 (-v) <input ELF file> <output UF2 file>\n");
    return ERROR_ARGS;
}

static int read_and_check_elf32_header(FILE *in, elf32_header& eh_out) {
    if (1 != fread(&eh_out, sizeof(eh_out), 1, in)) {
        return fail(ERROR_READ_FAILED, "Unable to read ELF header");
    }
    if (eh_out.common.magic != ELF_MAGIC) {
        return fail(ERROR_FORMAT, "Not an ELF file");
    }
    if (eh_out.common.version != 1 || eh_out.common.version2 != 1) {
        return fail(ERROR_FORMAT, "Unrecognized ELF version");
    }
    if (eh_out.common.arch_class != 1 || eh_out.common.endianness != 1) {
        return fail(ERROR_INCOMPATIBLE, "Require 32 bit little-endian ELF");
    }
    if (eh_out.eh_size != sizeof(struct elf32_header)) {
        return fail(ERROR_FORMAT, "Invalid ELF32 format");
    }
    if (eh_out.common.machine != EM_ARM) {
        return fail(ERROR_FORMAT, "Not an ARM executable");
    }
    if (eh_out.common.abi != 0) {
        return fail(ERROR_INCOMPATIBLE, "Unrecognized ABI");
    }
    if (eh_out.flags & EF_ARM_ABI_FLOAT_HARD) {
        return fail(ERROR_INCOMPATIBLE, "HARD-FLOAT not supported");
    }
    return 0;
}

int check_address_range(const address_ranges& valid_ranges, uint32_t addr, uint32_t vaddr, uint32_t size, bool uninitialized, address_range &ar) {
    for(const auto& range : valid_ranges) {
        if (range.from <= addr && range.to >= addr + size) {
            if (range.type == address_range::type::NO_CONTENTS && !uninitialized) {
                return fail(ERROR_INCOMPATIBLE, "ELF contains memory contents for uninitialized memory");
            }
            ar = range;
            if (verbose) {
                printf("%s segment %08x->%08x (%08x->%08x)\n", uninitialized ? "Uninitialized" : "Mapped", addr,
                   addr + size, vaddr, vaddr+size);
            }
            return 0;
        }
    }
    return fail(ERROR_INCOMPATIBLE, "Memory segment %08x->%08x is outside of valid address range for device", addr, addr+size);
}

int read_and_check_elf32_ph_entries(FILE *in, const elf32_header &eh, const address_ranges& valid_ranges, std::map<uint32_t, std::vector<page_fragment>>& pages) {
    if (eh.ph_entry_size != sizeof(elf32_ph_entry)) {
        return fail(ERROR_FORMAT, "Invalid ELF32 program header");
    }
    if (eh.ph_num) {
        std::vector<elf32_ph_entry> entries(eh.ph_num);
        if (fseek(in, eh.ph_offset, SEEK_SET)) {
            return fail_read_error();
        }
        if (eh.ph_num != fread(&entries[0], sizeof(struct elf32_ph_entry), eh.ph_num, in)) {
            return fail_read_error();
        }
        for(uint i=0;i<eh.ph_num;i++) {
            elf32_ph_entry& entry = entries[i];
            if (entry.type == PT_LOAD && entry.memsz) {
                address_range ar;
                int rc;
                uint mapped_size = std::min(entry.filez, entry.memsz);
                if (mapped_size) {
                    rc = check_address_range(valid_ranges, entry.paddr, entry.vaddr, mapped_size, false, ar);
                    if (rc) return rc;
                    // we don't download uninitialized, generally it is BSS and should be zero-ed by crt0.S, or it may be COPY areas which are undefined
                    if (ar.type != address_range::type::CONTENTS) {
                        if (verbose) printf("  ignored\n");
                        continue;
                    }
                    uint addr = entry.paddr;
                    uint remaining = mapped_size;
                    uint file_offset = entry.offset;
                    while (remaining) {
                        uint off = addr & (PAGE_SIZE - 1);
                        uint len = std::min(remaining, PAGE_SIZE - off);
                        auto &fragments = pages[addr - off]; // list of fragments
                        // note if filesz is zero, we want zero init which is handled because the
                        // statement above creates an empty page fragment list
                        // check overlap with any existing fragments
                        for (const auto &fragment : fragments) {
                            if ((off < fragment.page_offset + fragment.bytes) !=
                                ((off + len) <= fragment.page_offset)) {
                                fail(ERROR_FORMAT, "In memory segments overlap");
                            }
                        }
                        fragments.push_back(
                                page_fragment{file_offset,off,len});
                        addr += len;
                        file_offset += len;
                        remaining -= len;
                    }
                }
                if (entry.memsz > entry.filez) {
                    // we have some uninitialized data too
                    rc = check_address_range(valid_ranges, entry.paddr + entry.filez, entry.vaddr + entry.filez, entry.memsz - entry.filez, true,
                                             ar);
                    if (rc) return rc;
                }
            }
        }
    }
    return 0;
}

int realize_page(FILE *in, const std::vector<page_fragment> &fragments, uint8_t *buf, uint buf_len) {
    assert(buf_len >= PAGE_SIZE);
    for(auto& frag : fragments) {
        assert(frag.page_offset >= 0 && frag.page_offset < PAGE_SIZE && frag.page_offset + frag.bytes <= PAGE_SIZE);
        if (fseek(in, frag.file_offset, SEEK_SET)) {
            return fail_read_error();
        }
        if (1 != fread(buf + frag.page_offset, frag.bytes, 1, in)) {
            return fail_read_error();
        }
    }
    return 0;
}

static bool is_address_valid(const address_ranges& valid_ranges, uint32_t addr) {
    for(const auto& range : valid_ranges) {
        if (range.from <= addr && range.to > addr) {
            return true;
        }
    }
    return false;
}

static bool is_address_initialized(const address_ranges& valid_ranges, uint32_t addr) {
    for(const auto& range : valid_ranges) {
        if (range.from <= addr && range.to > addr) {
            return address_range::type::CONTENTS == range.type;
        }
    }
    return false;
}

static bool is_address_mapped(const std::map<uint32_t, std::vector<page_fragment>>& pages, uint32_t addr) {
    uint32_t page = addr & ~(PAGE_SIZE - 1);
    if (!pages.count(page)) return false;
    // todo check actual address within page
    return true;
}

int elf2uf2(FILE *in, FILE *out) {
    elf32_header eh;
    std::map<uint32_t, std::vector<page_fragment>> pages;
    int rc = read_and_check_elf32_header(in, eh);
    bool ram_style = false;
    address_ranges valid_ranges = {};
    if (!rc) {
        ram_style = is_address_initialized(rp2040_address_ranges_ram, eh.entry);
        if (verbose) {
            if (ram_style) {
                printf("Detected RAM binary\n");
            } else {
                printf("Detected FLASH binary\n");
            }
        }
        valid_ranges = ram_style ? rp2040_address_ranges_ram : rp2040_address_ranges_flash;
        rc = read_and_check_elf32_ph_entries(in, eh, valid_ranges, pages);
    }
    if (rc) return rc;
    if (pages.empty()) {
        return fail(ERROR_INCOMPATIBLE, "The input file has no memory pages");
    }
    uint page_num = 0;
    if (ram_style) {
        uint32_t expected_ep_main_ram = UINT32_MAX;
        uint32_t expected_ep_xip_sram = UINT32_MAX;
        for(auto& page_entry : pages) {
            if ( ((page_entry.first >= MAIN_RAM_START) && (page_entry.first < MAIN_RAM_END)) && (page_entry.first < expected_ep_main_ram) ) {
                expected_ep_main_ram = page_entry.first | 0x1;
            } else if ( ((page_entry.first >= XIP_SRAM_START) && (page_entry.first < XIP_SRAM_END)) && (page_entry.first < expected_ep_xip_sram) ) { 
                expected_ep_xip_sram = page_entry.first | 0x1;
            }
        }
        uint32_t expected_ep = (UINT32_MAX != expected_ep_main_ram) ? expected_ep_main_ram : expected_ep_xip_sram;
        if (eh.entry == expected_ep_xip_sram) {
            return fail(ERROR_INCOMPATIBLE, "B0/B1 Boot ROM does not support direct entry into XIP_SRAM\n");
        } else if (eh.entry != expected_ep) {
            return fail(ERROR_INCOMPATIBLE, "A RAM binary should have an entry point at the beginning: %08x (not %08x)\n", expected_ep, eh.entry);
        }
        static_assert(0 == (MAIN_RAM_START & (PAGE_SIZE - 1)), "");
        // currently don't require this as entry point is now at the start, we don't know where reset vector is
#if 0
        uint8_t buf[PAGE_SIZE];
        rc = realize_page(in, pages[MAIN_RAM_START], buf, sizeof(buf));
        if (rc) return rc;
        uint32_t sp = ((uint32_t *)buf)[0];
        uint32_t ip = ((uint32_t *)buf)[1];
        if (!is_address_mapped(pages, ip)) {
            return fail(ERROR_INCOMPATIBLE, "Vector table at %08x is invalid: reset vector %08x is not in mapped memory",
                MAIN_RAM_START, ip);
        }
        if (!is_address_valid(valid_ranges, sp - 4)) {
            return fail(ERROR_INCOMPATIBLE, "Vector table at %08x is invalid: stack pointer %08x is not in RAM",
                        MAIN_RAM_START, sp);
        }
#endif
    } 
    
    uf2_block block;
    block.magic_start0 = UF2_MAGIC_START0;
    block.magic_start1 = UF2_MAGIC_START1;
    block.flags = UF2_FLAG_FAMILY_ID_PRESENT;
    block.payload_size = PAGE_SIZE;
    block.num_blocks = (uint32_t)pages.size();
    block.file_size = RP2040_FAMILY_ID;
    block.magic_end = UF2_MAGIC_END;
    for(auto& page_entry : pages) {
        block.target_addr = page_entry.first;
        block.block_no = page_num++;
        if (verbose) {
            printf("Page %d / %d %08x\n", block.block_no, block.num_blocks, block.target_addr);
        }
        memset(block.data, 0, sizeof(block.data));
        rc = realize_page(in, page_entry.second, block.data, sizeof(block.data));
        if (rc) return rc;

        if (!ram_style) {
            if (block.target_addr == FLASH_START && block.payload_size >= BOOT2_SIZE_BYTES - 4) {
                uint32_t crc = crc32_calculator(block.data, BOOT2_SIZE_BYTES - 4, 0xFFFFFFFF);
                (*(uint32_t *)&(block.data[BOOT2_SIZE_BYTES - 4])) = crc;
            }
        }
        if (1 != fwrite(&block, sizeof(uf2_block), 1, out)) {
            return fail_write_error();
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    int arg = 1;
    if (arg < argc && !strcmp(argv[arg], "-v")) {
        verbose = true;
        arg++;
    }
    if (argc < arg + 2) {
        return usage();
    }
    const char *in_filename = argv[arg++];
    FILE *in = fopen(in_filename, "rb");
    if (!in) {
        fprintf(stderr, "Can't open input file '%s'\n", in_filename);
        return ERROR_ARGS;
    }
    const char *out_filename = argv[arg++];
    FILE *out = fopen(out_filename, "wb");
    if (!out) {
        fprintf(stderr, "Can't open output file '%s'\n", out_filename);
        return ERROR_ARGS;
    }

    int rc = elf2uf2(in, out);
    fclose(in);
    fclose(out);
    if (rc) {
        remove(out_filename);
        if (error_msg[0]) {
            fprintf(stderr, "ERROR: %s\n", error_msg);
        }
    }
    return rc;
}


static const uint32_t crc32_table_0x04c11db7[] = {
    0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9,
    0x130476dc, 0x17c56b6b, 0x1a864db2, 0x1e475005,
    0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
    0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd,
    0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9,
    0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
    0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011,
    0x791d4014, 0x7ddc5da3, 0x709f7b7a, 0x745e66cd,
    0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
    0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5,
    0xbe2b5b58, 0xbaea46ef, 0xb7a96036, 0xb3687d81,
    0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
    0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49,
    0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95,
    0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
    0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d,
    0x34867077, 0x30476dc0, 0x3d044b19, 0x39c556ae,
    0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
    0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16,
    0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca,
    0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
    0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02,
    0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1, 0x53dc6066,
    0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
    0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e,
    0xbfa1b04b, 0xbb60adfc, 0xb6238b25, 0xb2e29692,
    0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
    0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a,
    0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e,
    0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
    0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686,
    0xd5b88683, 0xd1799b34, 0xdc3abded, 0xd8fba05a,
    0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
    0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb,
    0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f,
    0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
    0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47,
    0x36194d42, 0x32d850f5, 0x3f9b762c, 0x3b5a6b9b,
    0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
    0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623,
    0xf12f560e, 0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7,
    0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
    0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f,
    0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3,
    0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
    0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b,
    0x9b3660c6, 0x9ff77d71, 0x92b45ba8, 0x9675461f,
    0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
    0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640,
    0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c,
    0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
    0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24,
    0x119b4be9, 0x155a565e, 0x18197087, 0x1cd86d30,
    0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
    0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088,
    0x2497d08d, 0x2056cd3a, 0x2d15ebe3, 0x29d4f654,
    0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
    0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c,
    0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18,
    0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
    0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0,
    0x9abc8bd5, 0x9e7d9662, 0x933eb0bb, 0x97ffad0c,
    0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
    0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
};


static uint32_t crc32_calculator (uint8_t *pbuf, uint_fast32_t size, uint32_t init)
{
    uint32_t crc_result = init;

    assert(NULL != pbuf);
    assert(size > 0);
 
    do  {
        crc_result = (crc_result << 8) ^ 
                    crc32_table_0x04c11db7[((crc_result >> 24) ^ (*pbuf)) & 0xFF];
        pbuf++;
    } while(--size);

    return crc_result;
}