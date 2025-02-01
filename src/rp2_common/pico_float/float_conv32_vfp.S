/*
 * Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#if !PICO_RP2040
#include "pico/asm_helper.S"

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
	vmov s15, r0
	vcvt.f32.s32 s15, s15
	vmov r0, s15
	bx lr

float_section uint2float
regular_func uint2float
	vmov s15, r0
	vcvt.f32.u32 s15, s15
	vmov r0, s15
	bx lr

float_section float2int
regular_func float2int
	vmov s15, r0
	vcvtm.s32.f32 s15, s15
	vmov r0, s15
	bx lr

float_section float2int_z
regular_func float2int_z
	vmov s15, r0
	vcvt.s32.f32 s15, s15
	vmov r0, s15
	bx lr

float_section float2uint
regular_func float2uint
regular_func float2uint_z
	vmov s15, r0
	vcvt.u32.f32 s15, s15
	vmov r0, s15
	bx lr

float_section float2fix_z
regular_func float2fix_z
  ubfx r2, r0, #23, #8
  adds r2, r1
  asrs r3, r2, #8
  beq 1f
  ite pl
  movpl r2, #0xff
  movmi r2, #0
1:
  bfi r0, r2, #23, #8
  b float2int_z

float_section float2fix
regular_func float2fix
  lsls r2, r0, #1
  // r0 = abs(zero)                   => r1 = 0x00000000
  // r0 = abs(denornaml)              => r1 = 0x00xxxxxx
  // r0 = abs(1.0f)                   => r1 = 0x7f000000
  // r0 = abs(inf/nan)                => r1 = 0xffxxxxxx
  bls float2fix_z // input positive or zero or -zero are ok for fix_z
  lsrs r2, #24
  beq float2fix_z // input denormal will be flushed to zero
  rsbs r3, r1, #0x7f
  subs r2, r3
  bcc 1f // iunput <1.0f means we need to subtract 1
  // mask off all but fractional bits
  lsls r2, r0, r2
  lsls r2, #9
  beq float2fix_z // input is integer
1:
  push {lr}
  bl float2fix_z
  subs r0, #1
  sbcs r1, r1, #0
  pop {pc}

float_section float2ufix
regular_func float2ufix
regular_func float2ufix_z
  ubfx r2, r0, #23, #8
  adds r2, r1
  asrs r3, r2, #8
  beq 1f
  ite pl
  movpl r2, #0xff
  movmi r2, #0
1:
  bfi r0, r2, #23, #8
  b float2uint_z
#endif
