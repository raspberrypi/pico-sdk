#!/usr/bin/env python3
#
# Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# Converts a cyw43-driver firmware header into the blob stored in the Wi-Fi firmware
# partition. The blob layout is independent of the cyw43-driver version:
#
#   int32_t wifi_fw_len
#   int32_t clm_len
#   wifi firmware (wifi_fw_len bytes), padded to a multiple of 512 bytes
#   clm (clm_len bytes), padded to a multiple of 4 bytes
#
# Older cyw43-driver headers have a single array already in this layout (after the lengths),
# with the lengths in CYW43_WIFI_FW_LEN and CYW43_CLM_LEN. Newer ones have separate
# <name>_firmware and <name>_clm arrays.

import sys
import re

assert len(sys.argv) == 3

CLM_ALIGN = 512


def pad(data, alignment):
    return data + bytes(-len(data) % alignment)


def parse_define(name, text):
    match = re.search(r"#define\s+" + name + r"\s+\((\d+)\)", text)
    return int(match[1]) if match else None


with open(sys.argv[1], "r") as f:
    text = f.read()

# Map each C array initialiser in the header, e.g. "name[123] ... = { 0x00, 0x01, ... }", to its bytes
arrays = {}
for match in re.finditer(r"\b(\w+)\s*\[\s*\d*\s*\][^=;]*=\s*\{([^}]*)\}", text):
    arrays[match[1]] = bytes(int(b, base=0) for b in match[2].replace(",", " ").split())

# Find the arrays whose names end in _firmware and _clm (None if there isn't one)
firmware = next((v for k, v in arrays.items() if k.endswith("_firmware")), None)
clm = next((v for k, v in arrays.items() if k.endswith("_clm")), None)

if firmware is not None and clm is not None:
    # Newer headers: separate firmware and CLM arrays, so combine them in the old layout
    cyw43_wifi_fw_len = len(firmware)
    cyw43_clm_len = len(clm)
    blob = pad(firmware, CLM_ALIGN) + clm
elif len(arrays) == 1:
    # Older headers: one array already combined, with the lengths in #defines
    cyw43_wifi_fw_len = parse_define("CYW43_WIFI_FW_LEN", text)
    cyw43_clm_len = parse_define("CYW43_CLM_LEN", text)
    if cyw43_wifi_fw_len is None or cyw43_clm_len is None:
        sys.exit("%s: missing CYW43_WIFI_FW_LEN or CYW43_CLM_LEN" % sys.argv[1])
    blob = next(iter(arrays.values()))
else:
    sys.exit("%s: expected <name>_firmware and <name>_clm arrays, or a single combined array" % sys.argv[1])

expected_len = len(pad(bytes(cyw43_wifi_fw_len), CLM_ALIGN)) + cyw43_clm_len
if len(blob) < expected_len:
    sys.exit("%s: firmware data is %d bytes, expected at least %d" % (sys.argv[1], len(blob), expected_len))

data = pad(
    cyw43_wifi_fw_len.to_bytes(4, 'little', signed=True) +
    cyw43_clm_len.to_bytes(4, 'little', signed=True) +
    blob,
    4
)

with open(sys.argv[2], "w") as f:
    for b in data:
        f.write(f".byte 0x{b:02x}\n")
