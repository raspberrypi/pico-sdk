#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
"""Check M33 double table alignment in an unlinked ELF object (no hardware needed).

Clang's integrated assembler does not implicitly align .long data. Checking
only symbol offsets, or addresses in a linked image, can therefore miss an
under-aligned input section. Both the section and its table symbol must be
word-aligned for the M33 table loads.
"""

import argparse
from pathlib import Path
import struct
import sys


TABLE_NAMES = {"rtwopi", "trigtab"}


def check_object(path, section_prefix):
    data = path.read_bytes()
    # This source is assembled only for little-endian, 32-bit Arm. Reading the
    # few required ELF fields directly avoids an extra Python package/tool.
    header = struct.unpack_from("<16sHHIIIIIHHHHHH", data)
    if header[0][:7] != b"\x7fELF\x01\x01\x01" or header[1:3] != (1, 40):
        raise ValueError("expected an ELF32 little-endian Arm relocatable object")
    section_offset, section_size, section_count, names_index = header[6], header[11], header[12], header[13]
    if section_size != 40 or not 0 < names_index < section_count:
        raise ValueError("invalid or unsupported ELF section table")
    sections = [struct.unpack_from("<10I", data, section_offset + i * section_size)
                for i in range(section_count)]

    def section_data(section):
        offset, size = section[4:6]
        if offset + size > len(data):
            raise ValueError("truncated ELF section")
        return data[offset:offset + size]

    def string_at(strings, offset):
        return strings[offset:strings.index(b"\0", offset)].decode("ascii")

    section_names = section_data(sections[names_index])
    found = set()
    for section in sections:
        if section[1] != 2:  # SHT_SYMTAB
            continue
        if section[9] != 16 or section[5] % 16:
            raise ValueError("invalid ELF symbol table")
        strings = section_data(sections[section[6]])
        for name, value, size, info, other, index in struct.iter_unpack("<IIIBBH", section_data(section)):
            name = string_at(strings, name)
            if name not in TABLE_NAMES:
                continue
            if name in found or not 0 < index < section_count:
                raise ValueError(f"{name}: missing or ambiguous section definition")
            table_section = sections[index]
            section_name = string_at(section_names, table_section[0])
            if section_name != section_prefix + name:
                raise ValueError(f"{name}: unexpected section {section_name}")
            alignment = table_section[8]
            if alignment < 4 or alignment % 4 or value % 4:
                raise ValueError(f"{name}: section alignment {alignment}, symbol offset {value:#x}; "
                                 "both must be word-aligned")
            found.add(name)
    if found != TABLE_NAMES:
        raise ValueError("missing table symbols: " + ", ".join(sorted(TABLE_NAMES - found)))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--section-prefix", required=True, choices=(".text.", ".time_critical."))
    parser.add_argument("objects", type=Path, nargs="+")
    args = parser.parse_args()
    for path in args.objects:
        try:
            check_object(path, args.section_prefix)
        except (OSError, ValueError, IndexError, struct.error) as error:
            print(f"{path}: {error}", file=sys.stderr)
            return 1
        print(f"{path}: rtwopi and trigtab are word-aligned")
    return 0


if __name__ == "__main__":
    sys.exit(main())
