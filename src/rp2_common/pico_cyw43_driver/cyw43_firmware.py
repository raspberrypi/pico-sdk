#!/usr/bin/env python3
#
# Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
#
# SPDX-License-Identifier: BSD-3-Clause

import sys
import re

assert len(sys.argv) == 3

cyw43_wifi_fw_len = -1
cyw43_clm_len = -1

with open(sys.argv[1], "r") as f:
    data = f.read()
    statements = data.split(";")
    for line in statements[1].split("\n"):
        if "#define CYW43_WIFI_FW_LEN" in line:
            matches = re.search(r"#define\s+\S+\s+\((\S+)\)", line)
            cyw43_wifi_fw_len = int(matches[1])
        elif "#define CYW43_CLM_LEN" in line:
            matches = re.search(r"#define\s+\S+\s+\((\S+)\)", line)
            cyw43_clm_len = int(matches[1])
        if cyw43_wifi_fw_len > 0 and cyw43_clm_len > 0:
            break
    data = statements[0]
    bits = data.split(",")
    bits[0] = bits[0].split("{")[-1]
    bits[-1] = bits[-1].split("}")[0]
    for i in range(len(bits)):
        bits[i] = bits[i].strip()
        bits[i] = bits[i].strip(',')
        bits[i] = int(bits[i], base=0)

data = (
    cyw43_wifi_fw_len.to_bytes(4, 'little', signed=True) +
    cyw43_clm_len.to_bytes(4, 'little', signed=True) +
    bytearray(bits)
)

with open(sys.argv[2], "w") as f:
    for b in data:
        f.write(f".byte 0x{b:02x}\n")
