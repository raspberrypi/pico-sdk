/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#if PICO_RP2040
#include "pico/asm_helper.S"
#include "pico/runtime_init.h"
#include "pico/double.h"
#include "pico/bootrom/sf_table.h"
#include "hardware/divider_helper.S"

pico_default_asm_setup

PICO_RUNTIME_INIT_FUNC_RUNTIME(__aeabi_double_init, PICO_RUNTIME_INIT_AEABI_DOUBLE)

.macro double_section name
#if PICO_DOUBLE_IN_RAM
.section RAM_SECTION_NAME(\name), "ax"
#else
.section SECTION_NAME(\name), "ax"
#endif
.endm

.macro _double_wrapper_func x
    wrapper_func \x
.endm

.macro wrapper_func_d1 x
   _double_wrapper_func \x
#if PICO_DOUBLE_PROPAGATE_NANS
    mov ip, lr
    bl __check_nan_d1
    mov lr, ip
#endif
.endm

// the ROM gives -Inf for a negative argument to log and sqrt; return NaN instead
// (-0 and negative denormals are left to the ROM)
.macro nan_if_negative_d
#if PICO_DOUBLE_PROPAGATE_NANS
    lsls r2, r1, #1
    bcc 3f
    lsrs r2, #21
    beq 3f
    movs r0, #0
    mvns r1, r0
    lsls r1, #19                // 0xfff80000
    bx lr
3:
#endif
.endm

.macro wrapper_func_d2 x
    _double_wrapper_func \x
#if PICO_DOUBLE_PROPAGATE_NANS
    mov ip, lr
    bl __check_nan_d2
    mov lr, ip
#endif
.endm

#if PICO_DOUBLE_PROPAGATE_NANS
double_section __check_nan_d1
regular_func __check_nan_d1
   movs r3, #1
   lsls r3, #21
   lsls r2, r1, #1
   adds r2, r3
   bhi 1f
   bx lr
1:
   bx ip

double_section __check_nan_d2
regular_func __check_nan_d2
   push {r0, r2}
   movs r2, #1
   lsls r2, #21
   lsls r0, r1, #1
   adds r0, r2
   bhi 1f
   lsls r0, r3, #1
   adds r0, r2
   bhi 2f
   pop {r0, r2}
   bx lr
2:
   pop {r0, r2}
   mov r0, r2
   mov r1, r3
   bx ip
1:
   pop {r0, r2}
   bx ip
#endif

.macro table_tail_call SF_TABLE_OFFSET
    push {r3, r4}
#if PICO_DOUBLE_SUPPORT_ROM_V1 && PICO_RP2040_B0_SUPPORTED
#ifndef NDEBUG
    movs r3, #0
    mov ip, r3
#endif
#endif
    ldr r3, =sd_table
    ldr r3, [r3, #\SF_TABLE_OFFSET]
    str r3, [sp, #4]
    pop {r3, pc}
.endm

.macro shimmable_table_tail_call SF_TABLE_OFFSET shim
    push {r3, r4}
    ldr r3, =sd_table
    ldr r3, [r3, #\SF_TABLE_OFFSET]
#if PICO_DOUBLE_SUPPORT_ROM_V1 && PICO_RP2040_B0_SUPPORTED
    mov ip, pc
#endif
    str r3, [sp, #4]
    pop {r3, pc}
#if PICO_DOUBLE_SUPPORT_ROM_V1 && PICO_RP2040_B0_SUPPORTED
.byte \SF_TABLE_OFFSET, 0xdf
.word \shim
#endif
.endm

.macro double_wrapper_section func
double_section WRAPPER_FUNC_NAME(\func)
.endm

double_section push_r8_r11
regular_func push_r8_r11
 mov r4,r8
 mov r5,r9
 mov r6,r10
 mov r7,r11
 push {r4-r7}
 bx r14

double_section pop_r8_r11
regular_func pop_r8_r11
 pop {r4-r7}
 mov r8,r4
 mov r9,r5
 mov r10,r6
 mov r11,r7
 bx r14

// note generally each function is in a separate section unless there is fall thru or branching between them
// note fadd, fsub, fmul, fdiv are so tiny and just defer to rom so are lumped together so they can share constant pool

// note functions are word aligned except where they are an odd number of linear instructions

// double FUNC_NAME(__aeabi_dadd)(double, double)         double-precision addition
double_wrapper_section __aeabi_darithmetic
// double FUNC_NAME(__aeabi_drsub)(double x, double y)    double-precision reverse subtraction, y - x

// frsub first because it is the only one that needs alignment
.align 2
wrapper_func __aeabi_drsub
    eors r0, r1
    eors r1, r0
    eors r0, r1
    // fall thru

// double FUNC_NAME(__aeabi_dsub)(double x, double y)     double-precision subtraction, x - y
wrapper_func_d2 __aeabi_dsub
#if PICO_DOUBLE_PROPAGATE_NANS
    // we want to return nan for inf-inf or -inf - -inf, but without too much upfront cost
    mov ip, r0
    mov r0, r1
    eors r0, r3
    bmi 1f // different signs
    mov r0, ip
    push {r0-r3, lr}
    bl 2f
    b ddiv_dsub_nan_helper
1:
    mov r0, ip
2:
#endif
   shimmable_table_tail_call SF_TABLE_FSUB dsub_shim

wrapper_func_d2 __aeabi_dadd
#if PICO_DOUBLE_PROPAGATE_NANS
    // likewise for inf + -inf
    mov ip, r0
    mov r0, r1
    eors r0, r3
    bpl 1f // same signs
    mov r0, ip
    push {r0-r3, lr}
    bl 2f
    b ddiv_dsub_nan_helper
1:
    mov r0, ip
2:
#endif
   shimmable_table_tail_call SF_TABLE_FADD dadd_shim

// double FUNC_NAME(__aeabi_ddiv)(double n, double d)     double-precision division, n / d
wrapper_func_d2 __aeabi_ddiv
#if PICO_DOUBLE_PROPAGATE_NANS
    push {r0-r3, lr}
    bl 1f
    b ddiv_dsub_nan_helper
1:
#endif
#if !PICO_DIVIDER_DISABLE_INTERRUPTS
    // to support IRQ usage (or context switch) we must save/restore divider state around call if state is dirty
    mov ip, r2
    ldr r2, =(SIO_BASE)
    ldr r2, [r2, #SIO_DIV_CSR_OFFSET]
    lsrs r2, #SIO_DIV_CSR_DIRTY_SHIFT_FOR_CARRY
    bcs ddiv_save_state
    mov r2, ip
#else
    // to avoid worrying about IRQs (or context switches), simply disable interrupts around call
    push {r4, lr}
    mrs r4, PRIMASK
    cpsid i
    bl ddiv_shim_call
    msr PRIMASK, r4
    pop {r4, pc}
#endif
ddiv_shim_call:
    shimmable_table_tail_call SF_TABLE_FDIV ddiv_shim

#if !PICO_DIVIDER_DISABLE_INTERRUPTS
ddiv_save_state:
    ldr r2, =(SIO_BASE)
    save_div_state_and_lr
    mov r2, ip
    bl ddiv_shim_call
    ldr r2, =(SIO_BASE)
    restore_div_state_and_return
#endif

ddiv_dsub_nan_helper:
#if PICO_DOUBLE_PROPAGATE_NANS
    // check for infinite op infinite (or rather check for infinite result with both
    // operands being infinite)
    lsls r2, r1, #1
    asrs r2, r2, #21
    adds r2, #1
    beq 2f
    add sp, #16
    pop {pc}
2:
    ldr r2, [sp, #4]
    ldr r3, [sp, #12]
    lsls r2, #1
    asrs r2, r2, #21
    lsls r3, #1
    asrs r3, r3, #21
    ands r2, r3
    adds r2, #1
    bne 3f
    // infinite to nan
    movs r2, #1
    lsls r2, #19
    orrs r1, r2
3:
    add sp, #16
    pop {pc}
#endif

// double FUNC_NAME(__aeabi_dmul)(double, double)         double-precision multiplication
wrapper_func_d2 __aeabi_dmul
#if PICO_DOUBLE_PROPAGATE_NANS
    push {r0-r3, lr}
    bl 1f

    // check for multiplication of infinite by zero (or rather check for infinite result with either
    // operand 0)
    lsls r3, r1, #1
    asrs r3, r3, #21
    adds r3, #1
    beq 2f
    add sp, #16
    pop {pc}
2:
    ldr r2, [sp, #4]
    ldr r3, [sp, #12]
    ands r2, r3
    bne 3f
    // infinite to nan
    movs r2, #1
    lsls r2, #19
    orrs r1, r2
3:
    add sp, #16
    pop {pc}
1:
#endif
   shimmable_table_tail_call SF_TABLE_FMUL dmul_shim

// void FUNC_NAME(__aeabi_cdrcmple)(double, double)         reversed 3-way (<, =, ?>) compare [1], result in PSR ZC flags
double_wrapper_section __aeabi_cdcmple

wrapper_func __aeabi_cdrcmple
 push {r0-r7,r14}
    eors r0, r2
    eors r2, r0
    eors r0, r2
    eors r1, r3
    eors r3, r1
    eors r1, r3
    b __aeabi_dfcmple_guts

// NOTE these share an implementation as we have no excepting NaNs.
// void FUNC_NAME(__aeabi_cdcmple)(double, double)         3-way (<, =, ?>) compare [1], result in PSR ZC flags
// void FUNC_NAME(__aeabi_cdcmpeq)(double, double)         non-excepting equality comparison [1], result in PSR ZC flags
@ compare r0:r1 against r2:r3, returning -1/0/1 for <, =, >
@ also set flags accordingly
.align 2
wrapper_func __aeabi_cdcmple
wrapper_func __aeabi_cdcmpeq
 push {r0-r7,r14}
__aeabi_dfcmple_guts:
 ldr r7,=0x7ff                @ flush NaNs and denormals
 lsls r4,r1,#1
 lsrs r4,#21
 beq 1f
 cmp r4,r7
 bne 2f
 lsls r4, r1, #12
 bhi 7f
1:
 movs r0,#0
 lsrs r1,#20
 lsls r1,#20
2:
 lsls r4,r3,#1
 lsrs r4,#21
 beq 1f
 cmp r4,r7
 bne 2f
 lsls r4, r3, #12
 bhi 7f
1:
 movs r2,#0
 lsrs r3,#20
 lsls r3,#20
2:
 movs r6,#1
 eors r3,r1
 bmi 4f                        @ opposite signs? then can proceed on basis of sign of x
 eors r3,r1                    @ restore r3
 bpl 2f
 cmp r3,r1
 bne 7f
1:
 cmp r2,r0
7:
 pop {r0-r7,r15}
2:
 cmp r1,r3
 bne 7b
1:
 cmp r0,r2
 pop {r0-r7,r15}
4:
 orrs r3,r1                    @ make -0==+0
 adds r3,r3
 orrs r3,r0
 orrs r3,r2
 beq 7b
 mvns r1, r1     @ carry inverse of r1 sign
 adds r1, r1
 pop {r0-r7,r15}


// int FUNC_NAME(__aeabi_dcmpeq)(double, double)         result (1, 0) denotes (=, ?<>) [2], use for C == and !=
double_wrapper_section __aeabi_dcmpeq
.align 2
wrapper_func __aeabi_dcmpeq
    push {lr}
    bl __aeabi_cdcmpeq
    beq 1f
    movs r0, #0
    pop {pc}
1:
    movs r0, #1
    pop {pc}

// int FUNC_NAME(__aeabi_dcmplt)(double, double)         result (1, 0) denotes (<, ?>=) [2], use for C <
double_wrapper_section __aeabi_dcmplt
.align 2
wrapper_func __aeabi_dcmplt
    push {lr}
    bl __aeabi_cdcmple
    sbcs r0, r0
    pop {pc}

// int FUNC_NAME(__aeabi_dcmple)(double, double)         result (1, 0) denotes (<=, ?>) [2], use for C <=
double_wrapper_section __aeabi_dcmple
.align 2
wrapper_func __aeabi_dcmple
    push {lr}
    bl __aeabi_cdcmple
    bls 1f
    movs r0, #0
    pop {pc}
1:
    movs r0, #1
    pop {pc}

// int FUNC_NAME(__aeabi_dcmpge)(double, double)         result (1, 0) denotes (>=, ?<) [2], use for C >=
double_wrapper_section __aeabi_dcmpge
.align 2
wrapper_func __aeabi_dcmpge
    push {lr}
    // because of NaNs it is better to reverse the args than the result
    bl __aeabi_cdrcmple
    bls 1f
    movs r0, #0
    pop {pc}
1:
    movs r0, #1
    pop {pc}

// int FUNC_NAME(__aeabi_dcmpgt)(double, double)         result (1, 0) denotes (>, ?<=) [2], use for C >
double_wrapper_section __aeabi_dcmpgt
wrapper_func __aeabi_dcmpgt
    push {lr}
    // because of NaNs it is better to reverse the args than the result
    bl __aeabi_cdrcmple
    sbcs r0, r0
    pop {pc}

// int FUNC_NAME(__aeabi_dcmpun)(double, double)         result (1, 0) denotes (?, <=>) [2], use for C99 isunordered()
double_wrapper_section __aeabi_dcmpun
wrapper_func __aeabi_dcmpun
   movs r0, #1
   lsls r0, #21
   lsls r2, r1, #1
   adds r2, r0
   bhi 1f
   lsls r2, r3, #1
   adds r2, r0
   bhi 1f
   movs r0, #0
   bx lr
1:
   movs r0, #1
   bx lr

// double FUNC_NAME(__aeabi_ui2d)(unsigned)             unsigned to double (double precision) conversion
double_wrapper_section __aeabi_ui2d
    shimmable_table_tail_call SF_TABLE_UINT2FLOAT uint2double_shim

double_wrapper_section __aeabi_i2d

wrapper_func __aeabi_ui2d
regular_func uint2double
    movs r1, #0
    cmp r0, #0
    bne 2f
1:
    bx lr
// double FUNC_NAME(__aeabi_i2d)(int)                     integer to double (double precision) conversion
wrapper_func __aeabi_i2d
regular_func int2double
    asrs r1, r0, #31
    eors r0, r1
    subs r0, r1
    beq 1b
    lsls r1, #31
2:
    push {r0, r1, r4, lr}
    ldr r3, =sf_clz_func
    ldr r3, [r3]
    blx r3
    pop {r2, r3}
    adds r4, r0, #1
    lsls r2, r4
    lsls r0, r2, #20
    lsrs r2, #12
    ldr r1,=1055
    subs r1, r4
    lsls r1, #20
    orrs r1, r3
    orrs r1, r2
    pop {r4, pc}

// int FUNC_NAME(__aeabi_d2iz)(double)                     double (double precision) to integer C-style conversion [3]
double_wrapper_section __aeabi_d2iz
wrapper_func __aeabi_d2iz
regular_func double2int_z
    lsls r2, r1, #1
    lsrs r2, #21
    movs r3, #0x80
    adds r2, r3
    lsls r3, #3
    subs r2, r3                 // exponent rebased to float bias
    lsls r3, #21                // 0x80000000
    cmp r2, #126
    ble 1f                      // |x| < 1
    subs r2, #158
    bge 2f                      // |x| >= 2^31, Inf or NaN
    negs r2, r2
    lsrs r0, #21
    orrs r0, r3
    lsls r3, r1, #11
    orrs r0, r3                 // top 32 bits of the mantissa, with the implied 1
    lsrs r0, r2
    asrs r1, #31
    eors r0, r1
    subs r0, r1                 // apply the sign
    bx lr
1:
    movs r0, #0
    bx lr
2:
    // overflow
    lsrs r0, r1, #31
    adds r0, r3
    subs r0, #1
    bx lr

double_section double2int
regular_func double2int
    shimmable_table_tail_call SF_TABLE_FLOAT2INT double2int_shim

// unsigned FUNC_NAME(__aeabi_d2uiz)(double)             double (double precision) to unsigned C-style conversion [3]
double_wrapper_section __aeabi_d2uiz
wrapper_func __aeabi_d2uiz
regular_func double2uint_z
regular_func double2uint
    shimmable_table_tail_call SF_TABLE_FLOAT2UINT double2uint_shim

double_section fix2double
regular_func fix2double
    shimmable_table_tail_call SF_TABLE_FIX2FLOAT fix2double_shim

double_section ufix2double
regular_func ufix2double
    shimmable_table_tail_call SF_TABLE_UFIX2FLOAT ufix2double_shim

double_section fix642double
regular_func fix642double
    shimmable_table_tail_call SF_TABLE_FIX642FLOAT fix642double_shim

double_section ufix2double
regular_func ufix642double
    shimmable_table_tail_call SF_TABLE_UFIX642FLOAT ufix642double_shim

// double FUNC_NAME(__aeabi_l2d)(long long)             long long to double (double precision) conversion
double_wrapper_section __aeabi_l2d
wrapper_func __aeabi_l2d
regular_func int642double
    shimmable_table_tail_call SF_TABLE_INT642FLOAT int642double_shim

// double FUNC_NAME(__aeabi_l2f)(long long)             long long to double (double precision) conversion
double_wrapper_section __aeabi_ul2d
wrapper_func __aeabi_ul2d
regular_func uint642double
    shimmable_table_tail_call SF_TABLE_UINT642FLOAT uint642double_shim

// long long FUNC_NAME(__aeabi_d2lz)(double)             double (double precision) to long long C-style conversion [3]
double_wrapper_section __aeabi_d2lz
wrapper_func __aeabi_d2lz
regular_func double2int64_z
    cmn r1, r1
    bcc double2int64
    push {lr}
    lsls r1, #1
    lsrs r1, #1
    movs r2, #0
    bl double2ufix64
    cmp r1, #0
    bmi 1f
    movs r2, #0
    negs r0, r0
    sbcs r2, r1
    mov r1, r2
    pop {pc}
1:
    movs r1, #128
    lsls r1, #24
    movs r0, #0
    pop {pc}

double_section double2int64
regular_func double2int64
    shimmable_table_tail_call SF_TABLE_FLOAT2INT64 double2int64_shim

// unsigned long long FUNC_NAME(__aeabi_d2ulz)(double)     double to unsigned long long C-style conversion [3]
double_wrapper_section __aeabi_d2ulz
wrapper_func __aeabi_d2ulz
regular_func double2uint64
regular_func double2uint64_z
    shimmable_table_tail_call SF_TABLE_FLOAT2UINT64 double2uint64_shim

double_section double2fix64_z
regular_func double2fix64_z
  lsls r3, r1, #1
  bcc double2fix64 // input positive is ok for fix64
  mov ip, r2
  asrs r2, r3, #21
  beq 3f           // input zero or denormal, so just return zero
  adds r2, #1
  beq double2fix64 // input infinite/nan is ok for fix64

  lsrs r3, #21
  add r3, ip
  movs r2, #1
  negs r2, r2
  lsrs r2, #22
  subs r3, r2 // r3 = modified e - 0x3ff

  bcc 3f // modified input < 1.0 means result is zero
  cmp r3, #52
  bge 2f // modified input must be an integer or infinite

  adds r3, #12
  mov r2, r1
  lsls r2, r2, r3    // r2 has remaining fractional mantissa bits of r1
  bne 1f             // not integer as non zero fractional bits remain
  subs r3, #32
  asrs r2, r3, #31
  bics r3, r3, r2
  movs r2, r0
  lsls r2, r2, r3
  bne 1f             // remaining fractional bits are non-zero, so argument was not an integer
2:
  // integer
  mov r2, ip
  b double2fix64
3: // result is zero
  movs r0, #0
  movs r1, #0
  bx lr
1:
  push {lr}
  mov r2, ip
  bl double2fix64
  movs r2, #0
  adds r0, #1
  adcs r1, r2
  pop {pc}

double_section double2fix64
regular_func double2fix64
    shimmable_table_tail_call SF_TABLE_FLOAT2FIX64 double2fix64_shim

double_section double2ufix64
regular_func double2ufix64
regular_func double2ufix64_z
    shimmable_table_tail_call SF_TABLE_FLOAT2UFIX64 double2ufix64_shim

double_section double2fix
regular_func double2fix
    shimmable_table_tail_call SF_TABLE_FLOAT2FIX double2fix_shim

double_section double2fix_z
regular_func double2fix_z
  lsls r3, r1, #1
  asrs r3, #21
  beq 2f // input is zero or denormal
  adds r3, #1
  beq 3f // input is infinite or nan

  // extract exponent again
  lsls r3, r1, #1
  lsrs r3, #21
  // adjust
  adds r3, r2
  ble 2f // adjusted input is zero or denormal or < 1
  lsrs r3, r3, #11
  bne 3f // adjusted input is > infinite

  lsls r2, r2, #20 // align exponent adjustment offset
  adds r1, r1, r2  // we know adjustment is safe
  b double2int_z
2:
  // result is zero
  movs r0, #0
  bx lr
3:
  movs r0, #0
  subs r0, #1
  lsrs r0, #1
  asrs r1, #31
  eors r0, r1
  bx lr

double_section double2ufix
regular_func double2ufix
regular_func double2ufix_z
    shimmable_table_tail_call SF_TABLE_FLOAT2UFIX double2ufix_shim

double_wrapper_section __aeabi_d2f
1:
#if PICO_DOUBLE_PROPAGATE_NANS
    // copy sign bit and 23 NAN id bits into sign bit and significant id bits, also set high id bit

    lsrs r0, #30
    lsls r2, r1, #12
    lsrs r2, #9
    asrs r1, #22
    lsls r1, #22
    orrs r0, r1
    orrs r0, r2
    bx lr
#endif
wrapper_func __aeabi_d2f
#if PICO_DOUBLE_PROPAGATE_NANS
    movs r3, #1
    lsls r3, #21
    lsls r2, r1, #1
    adds r2, r3
    bhi 1b
#endif
    // note double->float in double table at same index as float->double in double table
    shimmable_table_tail_call SF_TABLE_FLOAT2DOUBLE double2float_shim

double_wrapper_section srqt
wrapper_func_d1 sqrt
    nan_if_negative_d
    shimmable_table_tail_call SF_TABLE_FSQRT dsqrt_shim

double_section sincostan_remainder
// For finite |x| >= 1024, returns x reduced to [-pi, pi] to within about half an ulp, for the ROM sin/cos/tan which
// need |x| < 1024. |x| = m * 2^(e - 1075), so frac(|x| / 2pi) only needs the bits of 1/(2*pi) from 2^(1074 - e) down;
// a 224 bit window of them, from the word holding that bit, is multiplied by m into p on the stack.
// stack: p[-1] = 0, p[0..6] at sp+4, then &window low word, the window shift, hi(x), m_hi
9:
    // nan
    movs r2, #1
    lsls r2, #19
    orrs r1, r2
    bx lr
regular_func sincostan_remainder
    lsls r2, r1, #1
    asrs r2, #21
    adds r2, #1
    beq 9b
    push {r4-r7, lr}
    sub sp, #48
    movs r2, #0
    movs r3, #0
    movs r4, #0
    movs r5, #0
    mov r7, sp
    stmia r7!, {r2-r5}
    stmia r7!, {r2-r5}
    lsls r2, r1, #1
    lsrs r2, #21
    ldr r3, =1011
    subs r2, r3
    lsrs r4, r2, #5
    lsls r4, #2
    ldr r3, =inv_2pi_words + 24
    adds r4, r3
    str r4, [sp, #32]
    lsls r2, #27
    lsrs r2, #27
    str r2, [sp, #36]
    str r1, [sp, #40]
    lsls r3, r1, #12
    lsrs r3, #12
    movs r2, #1
    lsls r2, #20
    orrs r3, r2
    str r3, [sp, #44]
    // p += m_lo * window, then p[1..] += m_hi * window, mod 2^224
    movs r5, r0
    add r7, sp, #4
1:
    movs r6, #0
2:
    ldr r0, [r4]
    subs r4, #4
    movs r1, r5
    bl umul32
    ldr r2, [r7]
    adds r0, r2
    movs r2, #0
    adcs r1, r2
    adds r0, r6
    adcs r1, r2
    stmia r7!, {r0}
    movs r6, r1
    add r0, sp, #32
    cmp r7, r0
    bne 2b
    ldr r5, [sp, #44]
    cmp r5, #0
    beq 3f
    movs r0, #0
    str r0, [sp, #44]
    ldr r4, [sp, #32]
    add r7, sp, #8
    b 1b
3:
    // shift out the s bits above the window start to give frac(|x| / 2pi) as 0.224; a fraction of 0.5 or more
    // is negative, so is inverted (the missing +1 is below the precision of the window)
    ldr r2, [sp, #36]
    movs r3, #32
    subs r3, r2
    ldr r0, [sp, #28]
    ldr r1, [sp, #24]
    lsls r0, r2
    lsrs r1, r3
    orrs r0, r1
    asrs r6, r0, #31
    add r7, sp, #28
4:
    ldr r0, [r7]
    subs r7, #4
    ldr r1, [r7]
    lsls r0, r2
    lsrs r1, r3
    orrs r0, r1
    eors r0, r6
    str r0, [r7, #4]
    mov r0, sp
    cmp r7, r0
    bne 4b
    ldr r0, [sp, #40]
    eors r0, r6
    lsrs r0, #31
    lsls r0, #31
    str r0, [sp, #40]
    // normalize, counting the shift in r6: skip zero words, then load the top 96 bits as r5:r4:r3
    movs r6, #0
    add r7, sp, #28
    add r3, sp, #12
5:
    ldr r0, [r7]
    cmp r0, #0
    bne 6f
    cmp r7, r3
    beq 6f
    subs r7, #4
    adds r6, #32
    b 5b
6:
    subs r7, #8
    ldmia r7!, {r3, r4, r5}
    movs r0, r3
    orrs r0, r4
    orrs r0, r5
    beq 8f
    b 7f
6:
    adds r6, #1
    adds r3, r3
    adcs r4, r4
    adcs r5, r5
7:
    cmp r5, #0
    bge 6b
    // r1:r0 = top 64 bits of r5:r4 * 2*pi * 2^61
    movs r0, r5
    ldr r1, =0x2168c235
    bl umul32
    movs r7, r1
    movs r0, r4
    ldr r1, =0xc90fdaa2
    bl umul32
    movs r4, r1
    movs r0, r5
    ldr r1, =0xc90fdaa2
    bl umul32
    movs r2, #0
    adds r0, r7
    adcs r1, r2
    adds r0, r4
    adcs r1, r2
    bmi 1f
    adds r0, r0
    adcs r1, r1
    adds r6, #1
1:
    // round to 53 bits; a carry out of the mantissa goes into the exponent
    movs r2, #1
    lsls r2, #10
    adds r0, r2
    movs r2, #0
    adcs r1, r2
    lsls r2, r1, #21
    lsrs r0, #11
    orrs r0, r2
    lsrs r1, #11
    movs r2, #1
    lsls r2, #10
    subs r2, r6
    lsls r2, #20
    adds r1, r2
8:
    ldr r2, [sp, #40]
    orrs r1, r2
    add sp, #48
    pop {r4-r7, pc}

// r1:r0 = r0 * r1, using r2, r3 and ip
.thumb_func
umul32:
    lsrs r2, r0, #16
    uxth r0, r0
    lsrs r3, r1, #16
    uxth r1, r1
    mov ip, r2
    muls r2, r1
    muls r1, r0
    muls r0, r3
    adds r0, r2
    mov r2, ip
    muls r3, r2
    movs r2, #0
    adcs r2, r2
    lsls r2, #16
    adds r3, r2
    lsls r2, r0, #16
    lsrs r0, #16
    adds r1, r2
    adcs r3, r0
    movs r0, r1
    movs r1, r3
    bx lr

.ltorg
.align 2
// the bits of 1/(2*pi) after two zero words, which are read for |x| < 2^52
inv_2pi_words:
.word 0, 0
.word 0x28be60db, 0x9391054a, 0x7f09d5f4, 0x7d4d3770, 0x36d8a566, 0x4f10e410
.word 0x7f9458ea, 0xf7aef158, 0x6dc91b8e, 0x909374b8, 0x01924bba, 0x82746487
.word 0x3f877ac7, 0x2c4a69cf, 0xba208d7d, 0x4baed121, 0x3a671c09, 0xad17df90
.word 0x4e64758e, 0x60d4ce7d, 0x272117e2, 0xef7e4a0e, 0xc7fe25ff, 0xf7816603
.word 0xfbcbc462, 0xd6829b47, 0xdb4d9fb3, 0xc9f2c26d, 0xd3d18fd9, 0xa797fa8b
.word 0x5d49eeb1, 0xfaf97c5e, 0xcf41ce7d, 0xe294a4ba, 0x9afed7ec, 0x47e35742
.word 0x1580cc11

double_wrapper_section cos
#don't use _d1 as we're doing a range check anyway and infinites/nans are bigger than 1024
wrapper_func cos
    // rom version only works for -1024 < angle < 1024
    lsls r2, r1, #2
    bcc 1f
    lsrs r2, #22
    cmp r2, #9
    bge 2f
1:
    shimmable_table_tail_call SF_TABLE_FCOS dcos_shim
2:
#if PICO_DOUBLE_PROPAGATE_NANS
    lsls r2, r1, #1
    asrs r2, #21
    adds r2, #1
    bne 3f
    // infinite to nan
    movs r2, #1
    lsls r2, #19
    orrs r1, r2
    bx lr
3:
#endif
    push {lr}
    bl sincostan_remainder
    pop {r2}
    mov lr, r2
    b 1b

double_wrapper_section sin
#don't use _d1 as we're doing a range check anyway and infinites/nans are bigger than 1024
wrapper_func sin
    // rom version only works for -1024 < angle < 1024
    lsls r2, r1, #2
    bcc 1f
    lsrs r2, #22
    cmp r2, #9
    bge 2f
1:
    shimmable_table_tail_call SF_TABLE_FSIN dsin_shim
2:
#if PICO_DOUBLE_PROPAGATE_NANS
    lsls r2, r1, #1
    asrs r2, #21
    adds r2, #1
    bne 3f
    // infinite to nan
    movs r2, #1
    lsls r2, #19
    orrs r1, r2
    bx lr
3:
#endif
    push {lr}
    bl sincostan_remainder
    pop {r2}
    mov lr, r2
    b 1b

double_wrapper_section sincos
    // out of line remainder code for abs(angle)>=1024
2:
#if PICO_DOUBLE_PROPAGATE_NANS
    lsls r2, r1, #1
    asrs r2, #21
    adds r2, #1
    bne 3f
    // infinite to nan
    movs r2, #1
    lsls r2, #19
    orrs r1, r2
    pop {r4-r5}
    stmia r4!, {r0, r1}
    stmia r5!, {r0, r1}
    pop {r4, r5, pc}
3:
#endif
    push {lr}
    bl sincostan_remainder
    pop {r2}
    mov lr, r2
    b 1f // continue with sincos

wrapper_func sincos
    push {r2-r5, lr}
    // rom version only works for -1024 < angle < 1024
    lsls r2, r1, #2
    bcc 1f
    lsrs r2, #22
    cmp r2, #9
    bge 2b
1:
    bl 2f // call the shim
    pop {r4-r5}
    stmia r4!, {r0, r1}
    stmia r5!, {r2, r3}
    pop {r4, r5, pc}

2:
    shimmable_table_tail_call SF_TABLE_V3_FSINCOS sincos_shim_bootstrap

.thumb_func
sincos_shim_bootstrap:
    push {r2, r3, r4}
    movs r3, #0x13
    ldrb r3, [r3]
#if PICO_DOUBLE_SUPPORT_ROM_V1 && PICO_RP2040_B0_SUPPORTED
    cmp r3, #1
    bne 1f
    ldr r3, =dsincos_shim
    b 2f
#endif
1:
    ldr r3, =dsincos_shim_v2
2:
    ldr r2, =sd_table
    str r3, [r2, #SF_TABLE_V3_FSINCOS]
    str r3, [sp, #8]
    pop {r2, r3, pc}
.thumb_func
dsincos_shim_v2:
     push {r4-r7,r14}
     bl push_r8_r11
     bl v2_rom_dsincos_internal
     mov r12,r0                    @ save ε
     bl v2_rom_dcos_finish
     push {r0,r1}
     mov r0,r12
     bl v2_rom_dsin_finish
     pop {r2,r3}
     bl pop_r8_r11
     pop {r4-r7,r15}
.thumb_func
v2_rom_dsincos_internal:
    push {r0, lr}
    ldr r0, =0x3855
    str r0, [sp, #4]
    pop {r0, pc}
.thumb_func
v2_rom_dcos_finish:
    push {r0, r1}
    ldr r0, =0x389d
    str r0, [sp, #4]
    pop {r0, pc}
.thumb_func
v2_rom_dsin_finish:
    push {r0, r1}
    ldr r0, =0x38d9
    str r0, [sp, #4]
    pop {r0, pc}

double_wrapper_section tan
#don't use _d1 as we're doing a range check anyway and infinites/nans are bigger than 1024
wrapper_func tan
    // rom version only works for -1024 < angle < 1024
    lsls r2, r1, #2
    bcc dtan_in_range
    lsrs r2, #22
    cmp r2, #9
    bge dtan_angle_out_of_range
dtan_in_range:
#if !PICO_DIVIDER_DISABLE_INTERRUPTS
    // to support IRQ usage (or context switch) we must save/restore divider state around call if state is dirty
    mov ip, r2
    ldr r2, =(SIO_BASE)
    ldr r2, [r2, #SIO_DIV_CSR_OFFSET]
    lsrs r2, #SIO_DIV_CSR_DIRTY_SHIFT_FOR_CARRY
    bcs dtan_save_state
    mov r2, ip
#else
    // to avoid worrying about IRQs (or context switches), simply disable interrupts around call
    push {r4, lr}
    mrs r4, PRIMASK
    cpsid i
    bl dtan_shim_call
    msr PRIMASK, r4
    pop {r4, pc}
#endif
dtan_shim_call:
    shimmable_table_tail_call SF_TABLE_FTAN dtan_shim
#if !PICO_DIVIDER_DISABLE_INTERRUPTS
dtan_save_state:
    ldr r2, =(SIO_BASE)
    save_div_state_and_lr
    mov r2, ip
    bl dtan_shim_call
    ldr r2, =(SIO_BASE)
    restore_div_state_and_return
#endif
dtan_angle_out_of_range:
#if PICO_DOUBLE_PROPAGATE_NANS
    lsls r2, r1, #1
    asrs r2, #21
    adds r2, #1
    bne 3f
    // infinite to nan
    movs r2, #1
    lsls r2, #19
    orrs r1, r2
    bx lr
3:
#endif
    push {lr}
    bl sincostan_remainder
    pop {r2}
    mov lr, r2
    b dtan_in_range

double_wrapper_section atan2
wrapper_func_d2 atan2
    shimmable_table_tail_call SF_TABLE_FATAN2 datan2_shim

double_wrapper_section exp
wrapper_func_d1 exp
    shimmable_table_tail_call SF_TABLE_FEXP dexp_shim

double_wrapper_section log
wrapper_func_d1 log
    nan_if_negative_d
    shimmable_table_tail_call SF_TABLE_FLN dln_shim

#endif
