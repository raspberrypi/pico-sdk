/*
 * Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#if !PICO_RP2040
#include "pico/asm_helper.S"
#include "pico/float.h"

pico_default_asm_setup

.macro float_section name
#if PICO_FLOAT_IN_RAM
.section RAM_SECTION_NAME(\name), "ax"
#else
.section SECTION_NAME(\name), "ax"
#endif
.endm

float_section int2float
regular_func int2float
    vmov s0, r0
    vcvt.f32.s32 s0, s0
#if !defined(__ARM_PCS_VFP)
	vmov r0, s0
#endif
	bx lr

float_section uint2float
regular_func uint2float
	vmov s0, r0
	vcvt.f32.u32 s0, s0
#if !defined(__ARM_PCS_VFP)
	vmov r0, s0
#endif
	bx lr

float_section float2int
regular_func float2int
#if !defined(__ARM_PCS_VFP)
	vmov s0, r0
#endif
	vcvtm.s32.f32 s0, s0
	vmov r0, s0
	bx lr

float_section float2int_z
regular_func float2int_z
#if !defined(__ARM_PCS_VFP)
	vmov s0, r0
#endif
	vcvt.s32.f32 s0, s0
	vmov r0, s0
	bx lr

float_section float2uint
regular_func float2uint
regular_func float2uint_z
#if !defined(__ARM_PCS_VFP)
	vmov s0, r0
#endif
	vcvt.u32.f32 s0, s0
	vmov r0, s0
	bx lr

float_section float2fix_z
regular_func float2fix_z
#if defined(__ARM_PCS_VFP)
  mov r1, r0
  vmov r0, s0
#endif
float2fix_z_entry:
  ubfx r2, r0, #23, #8
  cbz r2, 2f // input is zero or denormal
  cmp r2, #0xff
  beq 3f // input infinite or nan
  adds r2, r1
  ble 2f // modified input is denormal so zero
  cmp r2, #0xff
  bge 3f // modified input is infinite
1:
  bfi r0, r2, #23, #8
  vmov s0, r0
  vcvt.s32.f32 s0, s0
  vmov r0, s0
  bx lr
2:
  movs r0, #0
  bx lr
3:
  mvn r1, #0x80000000
  add r0, r1, r0, lsr#31 @ so -Inf → 0x80000000, +Inf → 0x7fffffff
  bx lr

float_section float2fix
regular_func float2fix
#if defined(__ARM_PCS_VFP)
  mov r1, r0
  vmov r0, s0
#endif
  ubfx r2, r0, #23, #8
  cbz r2, 2f // input is zero or denormal
  cmp r2, #0xff
  beq 3f // input infinite or nan
  adds r2, r1
  ble 2f // modified input is denormal so zero
  cmp r2, #0xff
  bge 3f // modified input is infinite
1:
  bfi r0, r2, #23, #8
  vmov s0, r0
  vcvtm.s32.f32 s0, s0
  vmov r0, s0
  bx lr
2:
  movs r0, #0
  bx lr
3:
  mvn r1, #0x80000000
  add r0, r1, r0, lsr#31 @ so -Inf → 0x80000000, +Inf → 0x7fffffff
  bx lr

float_section float2ufix
regular_func float2ufix
regular_func float2ufix_z
#if defined(__ARM_PCS_VFP)
  mov r1, r0
  vmov r0, s0
#endif
  ubfx r2, r0, #23, #8
  cbz r2, 2f // input is zero or denormal
  cmp r2, #0xff
  beq 3f // input infinite or nan
  adds r2, r1
  ble 2f // modified input is denormal so zero
  cmp r2, #0xff
  bge 3f // modified input is infinite
1:
  bfi r0, r2, #23, #8
  vmov s0, r0
  vcvtm.u32.f32 s0, s0
  vmov r0, s0
  bx lr
2:
  movs r0, #0
  bx lr
3:
  mvn r0, r0, asr #31
  bx lr
#endif
