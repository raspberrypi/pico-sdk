/*
 * Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/asm_helper.S"

#if !HAS_DOUBLE_COPROCESSOR
#error attempt to compile float_aeabi_dcp when there is no DCP
#else

#include "hardware/dcp_instr.inc.S"
#include "hardware/dcp_canned.inc.S"

pico_default_asm_setup

// todo factor out save/restore (there is a copy in double code)

.macro float_section name
#if PICO_FLOAT_IN_RAM
.section RAM_SECTION_NAME(\name), "ax"
#else
.section SECTION_NAME(\name), "ax"
#endif
.endm

.macro float_wrapper_section func
float_section WRAPPER_FUNC_NAME(\func)
.endm

// ============== STATE SAVE AND RESTORE ===============

.macro saving_func type func, opt_label1='-', opt_label2='-'
  // Note we are usually 32-bit aligned already at this point, as most of the
  // function bodies contain exactly two 16-bit instructions: bmi and bx lr.
  // We want the PCMP word-aligned.
.p2align 2
  // When the engaged flag is set, branch back here to invoke save routine and
  // hook lr with the restore routine, then fall back through to the entry
  // point. The engaged flag will be clear when checked a second time.
1:
  push {lr}              // 16-bit instruction
  bl generic_save_state  // 32-bit instruction
  b 1f                   // 16-bit instruction
.ifnc \opt_label1,'-'
regular_func \opt_label1
.endif
.ifnc \opt_label2,'-'
regular_func \opt_label2
.endif
  // This is the actual entry point:
\type\()_func \func
  PCMP apsr_nzcv
  bmi 1b
1:
.endm

.macro saving_func_return
  bx lr
.endm

float_section __rp2350_dcp_engaged_state_save_restore
.thumb_func
generic_save_state:
  sub sp, #24
  push {r0, r1}
  // do save here
  PXMD r0, r1
  strd r0, r1, [sp, #8 + 0]
  PYMD r0, r1
  strd r0, r1, [sp, #8 + 8]
  REFD r0, r1
  strd r0, r1, [sp, #8 + 16]
  pop {r0, r1}
  blx lr
  // <- wrapped function returns here
  // fall through into restore:
.thumb_func
generic_restore_state:
  // do restore here
  pop {r12, r14}
  WXMD r12, r14
  pop {r12, r14}
  WYMD r12, r14
  pop {r12, r14}
  WEFD r12, r14
  pop {pc}

// ============== ARITHMETIC FUNCTIONS ===============

float_wrapper_section __aeabi_fadd
saving_func wrapper __aeabi_fadd
  dcp_fadd_m r0,r0,r1
  saving_func_return

float_wrapper_section __aeabi_fsub
saving_func wrapper __aeabi_fsub
  dcp_fsub_m r0,r0,r1
  saving_func_return

float_wrapper_section __aeabi_frsub
saving_func wrapper __aeabi_frsub
  dcp_fsub_m r0,r1,r0
  saving_func_return

float_wrapper_section __aeabi_fmul
saving_func wrapper __aeabi_fmul
  dcp_fmul_m r0,r0,r1,r0,r1
  saving_func_return

float_section fdiv_fast
saving_func regular fdiv_fast
  dcp_fdiv_fast_m r0,r0,r1,r0,r1,r2
  saving_func_return

float_wrapper_section __aeabi_fdiv
saving_func wrapper __aeabi_fdiv
@ with correct rounding
  dcp_fdiv_m r0,r0,r1,r0,r1,r2,r3
  saving_func_return

float_section sqrtf_fast
saving_func regular sqrtf_fast
  dcp_fsqrt_fast_m r0,r0,r0,r1,r2,r3
  saving_func_return

float_wrapper_section sqrtf
saving_func wrapper sqrtf
@ with correct rounding
  dcp_fsqrt_m r0,r0,r0,r1,r2,r3
  saving_func_return

float_section fclassify
saving_func regular fclassify
  dcp_fclassify_m apsr_nzcv,r0
  saving_func_return

// ============== CONVERSION FUNCTIONS ===============

float_wrapper_section __aeabi_f2d
saving_func wrapper __aeabi_f2d float2double
  dcp_float2double_m r0,r1,r0
  saving_func_return

float_wrapper_section __aeabi_i2f
saving_func  wrapper __aeabi_i2f int2float
@ with rounding
  dcp_int2float_m r0,r0
  saving_func_return

float_wrapper_section __aeabi_ui2f
saving_func wrapper __aeabi_ui2f uint2float
@ with rounding
  dcp_uint2float_m r0,r0
  saving_func_return

float_section float2fix_z
regular_func float2fix_z
  ubfx r2, r0, #23, #8
  cbz r2, 2f // input is zero or denormal
  cmp r2, #0xff
  beq 3f // input infinite or nan
  adds r2, r1
  ble 2f // modified input is denormal so zero
  cmp r2, #0xff
  beq 3f // modified input is infinite
1:
  bfi r0, r2, #23, #8
  b float2int_z_entry
2:
  movs r0, #0
  bx lr
3:
  mvn r1, #0x80000000
  add r0, r1, r0, lsr#31 @ so -Inf → 0x80000000, +Inf → 0x7fffffff
  bx lr

float_wrapper_section __aeabi_f2iz
saving_func wrapper __aeabi_f2iz float2int_z
@ with truncation towards 0
float2int_z_entry:
  dcp_float2int_m r0,r0
  saving_func_return

float_section __aeabi_f2ufix
regular_func float2ufix
regular_func float2ufix_z
  ubfx r2, r0, #23, #8
  cbz r2, 2f // input is zero or denormal
  cmp r2, #0xff
  beq 3f // input infinite or nan
  adds r2, r1
  ble 2f // modified input is denormal so zero
  cmp r2, #0xff
  beq 3f // modified input is infinite
1:
  bfi r0, r2, #23, #8
  b float2uint_z_entry
2:
  movs r0, #0
  bx lr
3:
  mvn r0, r0, asr #31
  bx lr

float_wrapper_section __aeabi_f2uiz
saving_func wrapper __aeabi_f2uiz float2uint_z float2uint
@ with truncation towards 0
float2uint_z_entry:
  dcp_float2uint_m r0,r0
  saving_func_return

float_section conv_f2fix
saving_func regular float2fix
  ubfx r2, r0, #23, #8
  cbz r2, 2f // input is zero or denormal
  cmp r2, #0xff
  beq 3f // input infinite or nan
  adds r2, r1
  ble 2f // modified input is denormal so zero
  cmp r2, #0xff
  beq 3f // modified input is infinite
1:
  bfi r0, r2, #23, #8
  b float2int_entry
2:
  movs r0, #0
  bx lr
3:
  mvn r1, #0x80000000
  add r0, r1, r0, lsr#31 @ so -Inf → 0x80000000, +Inf → 0x7fffffff
  bx lr

float_section float2int
// (not a real thing - kept because we use wrapper in saving_func)
saving_func regular float2int
float2int_entry:
  lsls r1, r0, #1
  // r0 = abs(zero)                   => r1 = 0x00000000
  // r0 = abs(denornaml)              => r1 = 0x00xxxxxx
  // r0 = abs(1.0f)                   => r1 = 0x7f000000
  // r0 = abs(inf/nan)                => r1 = 0xffxxxxxx
  bls float2int_z_entry // input positive or zero or -zero are ok for int64_z
  lsrs r1, #24
  beq float2int_z_entry // input denormal is flushed to zero anyway
  subs r1, #0x7f
  bcc 1f // input < 1.0f means we need to subtract 1 after conversion
  // mask off all but fractional bits
  lsls r2, r0, r1
  lsls r2, #9
  beq float2int_z_entry // input is integer
1:
  WXFC r0, r0
  ADD0
  ADD1
  NTDC
  RDIC r0
  subs r0, #1
saving_func_return

#if 0 // not sure these are super useful; if they are we should give them names
float_wrapper_section __aeabi_f2i_r
// (not a real thing - kept because we use wrapper in saving_func)
saving_func wrapper __aeabi_f2i_r
@ with rounding
  dcp_float2int_r_m r0,r0
  saving_func_return

float_wrapper_section __aeabi_f2ui_r
// (not a real thing - kept because we use wrapper in saving_func)
saving_func wrapper __aeabi_f2ui_r
@ with rounding
  dcp_float2uint_r_m r0,r0
  saving_func_return
#endif

// ============== COMPARISON FUNCTIONS ===============

float_wrapper_section __aeabi_fcmpun
saving_func wrapper __aeabi_fcmpun
  dcp_fcmp_m r0,r0,r1
  // extract unordered bit
  ubfx r0, r0, #28, #1
  saving_func_return

float_wrapper_section __aeabi_fcmp
saving_func wrapper __aeabi_cfrcmple
  dcp_fcmp_m apsr_nzcv,r1,r0 // with arguments reversed
  bvs cmp_nan
  saving_func_return

// these next two can be the same function in the absence of exceptions
saving_func wrapper __aeabi_cfcmple
  dcp_fcmp_m apsr_nzcv,r0,r1
  bvs cmp_nan
  saving_func_return

// It is not clear from the ABI documentation whether cfcmpeq must set the C flag
// in the same way as cfcmple. If not, we could save the "bvs" below; but we
// err on the side of caution.
saving_func wrapper __aeabi_cfcmpeq
  dcp_fcmp_m apsr_nzcv,r0,r1
  bvs cmp_nan
  saving_func_return

// If the result of a flag-setting comparison is "unordered" then we need to set C and clear Z.
// We could conceivably just do lsrs r12,r14,#1, or even cmp r14,r14,lsr#1 as (a) r14 here is a
// return address and r14b0=1 for Thumb mode; (b) we are unlikely to be returning to address 0.
cmp_nan:
  movs r12, #3 // r12 does not need to be preserved by the flag-setting comparisons
  lsrs r12, #1 // set C, clear Z
  saving_func_return

float_wrapper_section __aeabi_fcmpeq
saving_func wrapper __aeabi_fcmpeq
  dcp_fcmp_m r0,r0,r1
  // extract Z
  ubfx r0, r0, #30, #1
  saving_func_return

float_wrapper_section __aeabi_fcmplt
saving_func wrapper __aeabi_fcmplt
  dcp_fcmp_m apsr_nzcv,r1,r0
  ite hi
  movhi r0,#1
  movls r0,#0
  saving_func_return

float_wrapper_section __aeabi_fcmple
saving_func wrapper __aeabi_fcmple
  dcp_fcmp_m apsr_nzcv,r1,r0
  ite hs
  movhs r0,#1
  movlo r0,#0
  saving_func_return

float_wrapper_section __aeabi_fcmpge
saving_func wrapper __aeabi_fcmpge
  dcp_fcmp_m apsr_nzcv,r0,r1
  ite hs
  movhs r0,#1
  movlo r0,#0
  saving_func_return

float_wrapper_section __aeabi_fcmpgt
saving_func wrapper __aeabi_fcmpgt
  dcp_fcmp_m apsr_nzcv,r0,r1
  ite hi
  movhi r0,#1
  movls r0,#0
  saving_func_return

#endif
