/*
 * Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "boot/picobin.h"
            
#if PICO_CRT0_IMAGE_TYPE_TBYB
#define CRT0_TBYB_FLAG PICOBIN_IMAGE_TYPE_EXE_TBYB_BITS
#else
#define CRT0_TBYB_FLAG 0
#endif

.section .wifi_fw, "a"
.global _start
_start:
.word 0
.word 0
.word 0
.word 0

.p2align 2
embedded_block:
.word PICOBIN_BLOCK_MARKER_START

.byte PICOBIN_BLOCK_ITEM_1BS_IMAGE_TYPE
.byte 0x1 // 1 word
.hword PICOBIN_IMAGE_TYPE_IMAGE_TYPE_AS_BITS(EXE) | \
       PICOBIN_IMAGE_TYPE_EXE_CPU_AS_BITS(RISCV) | \
       PICOBIN_IMAGE_TYPE_EXE_CHIP_AS_BITS(RP2350) | \
       CRT0_TBYB_FLAG

// Entry point into SRAM
.byte PICOBIN_BLOCK_ITEM_1BS_ENTRY_POINT
.byte 0x3 // word size to next item
.byte 0 // pad
.byte 0 // pad
.word _start
.word 0x20082000 // stack pointer

_lm_item:
.byte PICOBIN_BLOCK_ITEM_LOAD_MAP
.byte 7
.byte 0 // pad
.byte 2 // 2 entries
// To sign the firmware
.word (_start - _lm_item)
.word _start
.word (firmware_end - _start)
// But clear SRAM if actually running this, so it doesn't boot
.word 0
.word _start
.word 0x00082000

.byte PICOBIN_BLOCK_ITEM_2BS_LAST
.hword (embedded_block_end - embedded_block - 16 ) / 4 // total size of all
.byte 0
.word 0
.word PICOBIN_BLOCK_MARKER_END
embedded_block_end:

#include "firmware_blob.S"

firmware_end:
