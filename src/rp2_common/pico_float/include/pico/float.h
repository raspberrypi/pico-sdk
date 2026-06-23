/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_FLOAT_H
#define _PICO_FLOAT_H

/** \file float.h
* \defgroup pico_float pico_float
*
* \brief Optimized single-precision floating point functions
*
* An application can take control of the floating point routines used in the application over and above what is provided by the compiler,
* by depending on the pico_float library. A user might want to do this
*
* 1. To use optimized software implementations provided by the RP2-series device's bootrom or the SDK
* 2. To use optimized combined software/hardware implementations utilizing custom RP2-series hardware for acceleration
* 3. To control the amount of C compiler/library code bloat
* 4. To make sure no floating point is called at all
*
* The pico_float library comes in three main flavors:
*
* 1. `pico_float_none` - all floating point operations cause a \ref panic - no single-precision floating point code is included
* 2. `pico_float_compiler` - no custom functions are provided; all single-precision floating point is handled by the C compiler/library
* 3. `pico_float_pico` - the smallest and fastest available for the platform, along with additional functionality (e.g. fixed-point conversions) which are detailed below
*
* The user can control which version they want (e.g. **pico_float_xxx** by either setting the CMake global variable
* `PICO_DEFAULT_FLOAT_IMPL=xxx`, or by using the CMake function `pico_set_float_implementation(<TARGET> xxx)`. Note that in the absence
* of either, pico_float_pico is used by default.
*
* \if rp2040_specific
* On RP2040, `pico_float_pico` uses optimized hand coded implementations from the bootrom and the SDK for both
* basic single-precision floating point operations and floating point math library functions. These implementations
* are generally faster and smaller than those provided by the C compiler/library, though they don't support all the features of a fully compliant
* floating point implementation; they are however usually fine for the majority of cases
* \endif
*
* \if rp2350_specific
* On Arm on RP2350, there are multiple options for `pico_float_pico`:
*
* 1. `pico_float_pico_vfp` - this library leaves basic C single-precision floating point operations to the compiler
* which can use inlined VFP (Arm FPU) code. Custom optimized versions of trigonometric and scientific functions are provided.
* No DCP (RP2350 Double co-processor) instructions are used.
* 2. `pico_float_pico_dcp` - this library prevents the compiler injecting inlined VFP code, and also implements
* all single-precision floating point operations in optimized DCP or M33 code. This option is not quite as fast
* as pico_float_pico_vfp, however it allows floating point operations without enabling the floating point co-processor
* on the CPU; this can be beneficial in certain circumstances, e.g. where leaving stack in tasks or interrupts
* for the floating point state is undesirable.
*
* Note: `pico_float_pico` is equivalent to `pico_float_pico_vfp` on RP2350, as this is the most sensible default
* \endif
*
* On Arm, (replacement) optimized implementations are provided for the following compiler built-ins
* and math library functions when using `_pico` variants of `pico_float`:
*
* - basic arithmetic: (except `pico_float_pico_vfp`)
*
*   __aeabi_fadd, __aeabi_fdiv, __aeabi_fmul, __aeabi_frsub, __aeabi_fsub
*
* - comparison: (except `pico_float_pico_vfp`)
*
*   __aeabi_cfcmpeq, __aeabi_cfrcmple, __aeabi_cfcmple, __aeabi_fcmpeq, __aeabi_fcmplt, __aeabi_fcmple, __aeabi_fcmpge, __aeabi_fcmpgt, __aeabi_fcmpun
*
* - (u)int32 <-> float: (except `pico_float_pico_vfp`)
*
*    __aeabi_i2f, __aeabi_ui2f, __aeabi_f2iz, __aeabi_f2uiz
*
* - (u)int64 <-> float: (except `pico_float_pico_vfp`)
*
*   __aeabi_l2f, __aeabi_ul2f, __aeabi_f2lz, __aeabi_f2ulz
*
* - float -> double: (except `pico_float_pico_vfp`)
*
*   __aeabi_f2d
*
* - basic trigonometric:
*
*   sqrtf, cosf, sinf, tanf, atan2f, expf, logf
*
* - trigonometric and scientific
*
*   ldexpf, copysignf, truncf, floorf, ceilf, roundf, asinf, acosf, atanf, sinhf, coshf, tanhf, asinhf, acoshf, atanhf, exp2f, log2f, exp10f, log10f, powf, hypotf, cbrtf, fmodf, dremf, remainderf, remquof, expm1f, log1pf, fmaf
*
* - GNU extensions:
*
*   sincosf
*
* On Arm, the following additional optimized functions are also provided (when using `_pico` variants of `pico_float`), all of which
* saturate to the nearest representable value for too large input when converting from floating point types:
*
* - Conversions to/from integer types:
*
*   - (u)int -> float (round to nearest):
*
*     int2float, uint2float, int642float, uint642float
*
*     note: on `pico_float_pico_vfp` the 32-bit functions are also provided as C macros since they map to inline VFP code
*
*   - (u)float -> int (round towards zero):
*
*     float2int_z, float2uint_z, float2int64_z, float2uint64_z
*
*     note: on `pico_float_pico_vfp` the 32-bit functions are also provided as C macros since they map to inline VFP code
*
*   - (u)float -> int (round towards -infinity):
*
*     float2int, float2uint, float2int64, float2uint64
*
* - Conversions to/from fixed-point integers:
*
*   - (u)fix -> float (round to nearest):
*
*       fix2float, ufix2float, fix642float, ufix642float
*
*   - float -> (u)fix (round towards zero):
*
*       float2fix_z, float2ufix_z, float2fix64_z, float2ufix64_z
*
*     note: on `pico_float_pico_vfp` the 32-bit functions are also provided as C macros since they can map to inline VFP code
*     when the number of fractional bits is a compile time constant between 1 and 32
*
*   - float -> (u)fix (round towards -infinity):
*
*       float2fix, float2ufix, float2fix64, float2ufix64
*
*     note: on `pico_float_pico_vfp` the 32-bit functions are also provided as C macros since they can map to inline VFP code
*     when the number of fractional bits is a compile time constant between 1 and 32
*
* - Scientific functions:
*
*    powintf
* \if rp2350_specific
*
* - Even faster versions of divide and square-root functions that do not round correctly: (`pico_float_pico_dcp` only)
*
*    fdiv_fast, sqrtf_fast
*
* On RISC-V, (replacement) optimized implementations are provided for the following compiler built-ins when using the `pico_float_pico`
* library (note that there are no variants of this library like there are on Arm):
*
* - Basic arithmetic:
*
*    __addsf3, __subsf3, __mulsf3
* \endif
*/

#include "pico.h"

// PICO_CONFIG: PICO_FLOAT_IN_RAM, Force placement of SDK provided single-precision floating point code into RAM, type=bool, default=0, group=pico_float
#ifndef PICO_FLOAT_IN_RAM
#define PICO_FLOAT_IN_RAM 0
#endif

#if !(LIB_PICO_FLOAT_COMPILER || defined(__riscv)) || PICO_DOCS
// private define to simplify this header only - it is undefined at the end
#define __PICO_FLOAT_ARM_OPTIMIZED 1
#endif

//! \addtogroup pico_float
//! \{

// we always define these for C code, but they are inline
// funcs except for __PICO_FLOAT_ARM_OPTIMIZED so wouldn't
// be callable for assembly
#if __PICO_FLOAT_ARM_OPTIMIZED || !defined(__ASSEMBLER__)
//! Set if \ref int2float and \ref uint2float are available
#define PICO_FLOAT_HAS_INT32_TO_FLOAT_CONVERSIONS 1
//! Set if \ref int642float and \ref uint642float are available
#define PICO_FLOAT_HAS_INT64_TO_FLOAT_CONVERSIONS 1
//! Set if \ref float2int_z and \ref float2uint_z are available (rounding towards zero)
#define PICO_FLOAT_HAS_FLOAT_TO_INT32_Z_CONVERSIONS 1
//! Set if \ref float2int64_z and \ref float2uint64_z are available (rounding towards zero)
#define PICO_FLOAT_HAS_FLOAT_TO_INT64_Z_CONVERSIONS 1
#endif

#if __PICO_FLOAT_ARM_OPTIMIZED
//! Set if \ref fix2float and \ref ufix2float are available
#define PICO_FLOAT_HAS_FIX32_TO_FLOAT_CONVERSIONS 1
//! Set if \ref fix642float and \ref ufix642float are available
#define PICO_FLOAT_HAS_FIX64_TO_FLOAT_CONVERSIONS 1
//! Set if \ref float2fix_z and \ref float2ufix_z are available (rounding towards zero)
#define PICO_FLOAT_HAS_FLOAT_TO_FIX32_Z_CONVERSIONS 1
//! Set if \ref float2fix64_z and \ref float2ufix64_z are available (rounding towards zero)
#define PICO_FLOAT_HAS_FLOAT_TO_FIX64_Z_CONVERSIONS 1

//! Set if \ref float2int and \ref float2uint are available (rounding towards -Infinity)
#define PICO_FLOAT_HAS_FLOAT_TO_INT32_M_CONVERSIONS 1
//! Set if \ref float2int64 and \ref float2uint64 are available (rounding towards -Infinity)
#define PICO_FLOAT_HAS_FLOAT_TO_INT64_M_CONVERSIONS 1

//! Set if \ref float2fix and \ref float2ufix are available (rounding towards -Infinity)
#define PICO_FLOAT_HAS_FLOAT_TO_FIX32_M_CONVERSIONS 1
//! Set if \ref float2fix64 and \ref float2ufix64 are available (rounding towards -Infinity)
#define PICO_FLOAT_HAS_FLOAT_TO_FIX64_M_CONVERSIONS 1
#endif

#if (PICO_RP2350 && LIB_PICO_FLOAT_PICO_DCP) || PICO_DOCS
//! Set if \ref fdiv_fast is available
#define PICO_FLOAT_HAS_FDIV_FAST 1
//! Set if \ref sqrtf_fast is available
#define PICO_FLOAT_HAS_SQRTF_FAST 1
#endif

#if __PICO_FLOAT_ARM_OPTIMIZED || __builtin_powif || PICO_DOCS
//! Set if \ref powintf is available
#define PICO_FLOAT_HAS_POWINTF 1
#endif
//! \}

#ifndef __ASSEMBLER__
#include <math.h>
#include <float.h>

#ifdef __cplusplus
extern "C" {
#endif

//! \addtogroup pico_float
//! \{
#if PICO_FLOAT_HAS_INT32_TO_FLOAT_CONVERSIONS
#if LIB_PICO_FLOAT_PICO_VFP || !__PICO_FLOAT_ARM_OPTIMIZED
    // for VFP the C cast is an assembly instruction anyway, so we prefer that over a function call
    // for non Arm-optimized we may as well provide the function and let the compiler handle it
    static inline float int2float(int32_t i) { return (float)i; }
    static inline float uint2float(uint32_t u) { return (float)u; }
#else
    //! Convert a signed 32-bit integer to the nearest float
    float int2float(int32_t i);
    //! Convert an unsigned 32-bit integer to the nearest float
    float uint2float(uint32_t u);
#endif
#endif

#if PICO_FLOAT_HAS_INT64_TO_FLOAT_CONVERSIONS
#if !__PICO_FLOAT_ARM_OPTIMIZED
    // for non Arm-optimized we may as well provide the function and let the compiler handle it
    static inline float int642float(int64_t i) { return (float)i; }
    static inline float uint642float(uint64_t u) { return (float)u; }
#else
    //! Convert a signed 64-bit integer to the nearest float
    float int642float(int64_t i);
    //! Convert an unsigned 64-bit integer to the nearest float
    float uint642float(uint64_t u);
#endif
#endif

#if PICO_FLOAT_HAS_FLOAT_TO_INT32_Z_CONVERSIONS
#if !__PICO_FLOAT_ARM_OPTIMIZED
    // for non Arm-optimized we may as well provide the function and let the compiler handle it
    static inline int32_t float2int_z(float f) { return (int32_t)f; }
    static inline uint32_t float2uint_z(float f) { return (uint32_t)f; }
#else
    //! \brief Convert a float to a signed 32-bit integer, rounding towards zero.
    //! On Arm this conversion is saturating (to INT32_MAX/INT32_MIN) for out of range input except when using `pico_float_compiler`
    int32_t float2int_z(float f);
    //! \brief Convert a float to an unsigned 32-bit integer, rounding towards zero
    //! On Arm this conversion is saturating (to UINT32_MAX/UINT32_MIN) for out of range input except when using `pico_float_compiler`
    uint32_t float2uint_z(float f);
#endif
#endif

#if PICO_FLOAT_HAS_FLOAT_TO_INT64_Z_CONVERSIONS
#if !__PICO_FLOAT_ARM_OPTIMIZED
    // for non Arm-optimized we may as well provide the function and let the compiler handle it
    static inline int64_t float2int64_z(float f) { return (int64_t)f; }
    static inline uint64_t float2uint64_z(float f) { return (uint64_t)f; }
#else
    //! \brief Convert a float to a signed 64-bit integer, rounding towards zero.
    //! On Arm this conversion is saturating (to INT64_MAX/INT64_MIN) for out of range input except when using `pico_float_compiler`
    int64_t float2int64_z(float f);
    //! \brief Convert a float to an unsigned 64-bit integer, rounding towards zero.
    //! On Arm this conversion is saturating (to UINT64_MAX/UINT64_MIN) for out of range input except when using `pico_float_compiler`
    uint64_t float2uint64_z(float f);
#endif
#endif

#if PICO_FLOAT_HAS_FIX32_TO_FLOAT_CONVERSIONS
//! \brief Convert a signed 32-bit fixed-point integer with the given number of fractional bits to the nearest float
//! Out of range inputs will convert to +/- Infinity
float fix2float(int32_t m, int e);
//! \brief Convert an unsigned 32-bit fixed-point integer with the given number of fractional bits to the nearest float
//! Out of range inputs will convert to +Infinity
float ufix2float(uint32_t m, int e);
#endif

#if PICO_FLOAT_HAS_FIX64_TO_FLOAT_CONVERSIONS
//! \brief Convert a signed 64-bit fixed-point integer with the given number of fractional bits to the nearest float
//! Out of range inputs will convert to +/- Infinity
float fix642float(int64_t m, int e);
//! \brief Convert an unsigned 64-bit fixed-point integer with the given number of fractional bits to the nearest float
//! Out of range inputs will convert to +Infinity
float ufix642float(uint64_t m, int e);
#endif

#if PICO_FLOAT_HAS_FLOAT_TO_FIX32_Z_CONVERSIONS
//! \brief Convert a float to a signed 32-bit fixed-point integer with the given number of fractional bits, rounding towards zero.
//! On Arm this conversion is saturating (to INT32_MAX/INT32_MIN) for out of range input except when using `pico_float_compiler`
int32_t float2fix_z(float f, int e);
//! \brief Convert a float to an unsigned 32-bit fixed-point integer with the given number of fractional bits, rounding towards zero.
//! This conversion is saturating (to UINT32_MAX/UINT32_MIN) for out of range input
uint32_t float2ufix_z(float f, int e);
#endif

#if PICO_FLOAT_HAS_FLOAT_TO_FIX64_Z_CONVERSIONS
//! \brief Convert a float to a signed 64-bit fixed-point integer with the given number of fractional bits, rounding towards zero.
//! On Arm this conversion is saturating (to INT64_MAX/INT64_MIN) for out of range input except when using `pico_float_compiler`
int64_t float2fix64_z(float f, int e);
//! \brief Convert a float to an unsigned 64-bit fixed-point integer with the given number of fractional bits, rounding towards zero.
//! This conversion is saturating (to UINT64_MAX/UINT64_MIN) for out of range input
uint64_t float2ufix64_z(float f, int e);
#endif

// These methods round towards -Infinity - which IS NOT the C way for negative numbers;
// as such the naming is not ideal, however is kept for backwards compatibility
#if PICO_FLOAT_HAS_FLOAT_TO_INT32_M_CONVERSIONS
//! \brief Convert a float to a signed 32-bit integer, rounding towards -Infinity.
//! This conversion is saturating (to INT32_MAX/INT32_MIN) for out of range input
int32_t float2int(float f);
//! \brief Convert a float to an unsigned 32-bit integer, rounding towards -Infinity.
//! This conversion is saturating (to UINT32_MAX/UINT32_MIN) for out of range input
uint32_t float2uint(float f);
#endif

#if PICO_FLOAT_HAS_FLOAT_TO_INT64_M_CONVERSIONS
//! \brief Convert a float to a signed 64-bit integer, rounding towards -Infinity.
//! This conversion is saturating (to INT64_MAX/INT64_MIN) for out of range input
int64_t float2int64(float f);
//! \brief Convert a float to an unsigned 64-bit integer, rounding towards -Infinity.
//! This conversion is saturating (to UINT64_MAX/UINT64_MIN) for out of range input
uint64_t float2uint64(float f);
#endif

#if PICO_FLOAT_HAS_FLOAT_TO_FIX32_M_CONVERSIONS
//! \brief Convert a float to a signed 32-bit fixed-point integer with the given number of fractional bits, rounding towards -Infinity.
//! This conversion is saturating (to INT32_MAX/INT32_MIN) for out of range input
int32_t float2fix(float f, int e);
//! \brief Convert a float to an unsigned 32-bit fixed-point integer with the given number of fractional bits, rounding towards -Infinity.
//! This conversion is saturating (to UINT32_MAX/UINT32_MIN) for out of range input
uint32_t float2ufix(float f, int e);
#endif

#if PICO_FLOAT_HAS_FLOAT_TO_FIX64_M_CONVERSIONS
//! \brief Convert a float to a signed 64-bit fixed-point integer with the given number of fractional bits, rounding towards -Infinity.
//! This conversion is saturating (to INT64_MAX/INT64_MIN) for out of range input
int64_t float2fix64(float f, int e);
//! \brief Convert a float to an unsigned 64-bit fixed-point integer with the given number of fractional bits, rounding towards -Infinity.
//! This conversion is saturating (to UINT64_MAX/UINT64_MIN) for out of range input
uint64_t float2ufix64(float f, int e);
#endif

#if LIB_PICO_FLOAT_PICO_VFP
// special handling of fixed-point conversions for VFP - we want to inline calls with fixed exponents
// between 1 and 32, because they can use an assembly instruction. we leave the function in place
// with its original name, however make a #define which will either do the inline instruction
// or call the original function
#if PICO_FLOAT_HAS_FIX32_TO_FLOAT_CONVERSIONS
// a bit of a hack to inline VFP fixed-point conversion when exponent is constant and in range 1-32
#define fix2float(m, e) (__builtin_constant_p(e) && (e) >= 1 && (e) <= 32 ? _fix2float_inline(m, e) : fix2 ## float(m, e))
#define ufix2float(m, e) (__builtin_constant_p(e) && (e) >= 1 && (e) <= 32 ? _ufix2float_inline(m, e) : ufix2 ## float(m, e))

#define _fix2float_inline(m, e) ({ \
    int32_t _m = m; \
    float f; \
    pico_default_asm( \
        "vmov %0, %1\n" \
        "vcvt.f32.s32 %0, %0, %2\n" \
        : "=t" (f) \
        : "r" (_m), "i" (e) \
    ); \
    f; \
})
#define _ufix2float_inline(m, e) ({ \
    uint32_t _m = m; \
    float f; \
    pico_default_asm( \
        "vmov %0, %1\n" \
        "vcvt.f32.u32 %0, %0, %2\n" \
        : "=t" (f) \
        : "r" (_m), "i" (e) \
    ); \
    f; \
})

#endif
#if PICO_FLOAT_HAS_FLOAT_TO_FIX32_Z_CONVERSIONS
#define float2fix_z(f, e) (__builtin_constant_p(e) && (e) >= 1 && (e) <= 32 ? _float2fix_z_inline(f, e) : float2 ## fix_z(f, e))
#define float2ufix_z(f, e) (__builtin_constant_p(e) && (e) >= 1 && (e) <= 32 ? _float2ufix_z_inline(f, e) : float2 ## ufix_z(f, e))

#define _float2fix_z_inline(f, e) ({ \
    int32_t _m; \
    float _f = (f); \
    pico_default_asm( \
        "vcvt.s32.f32 %0, %0, %2\n" \
        "vmov %1, %0\n" \
        : "+t" (_f), "=r" (_m) \
        : "i" (e) \
    ); \
    _m; \
})
#define _float2ufix_z_inline(f, e) ({ \
    uint32_t _m; \
    float _f = (f); \
    pico_default_asm( \
        "vcvt.u32.f32 %0, %0, %2\n" \
        "vmov %1, %0\n" \
        : "+t" (_f), "=r" (_m) \
        : "i" (e) \
    ); \
    _m; \
})

#endif
#if PICO_FLOAT_HAS_FLOAT_TO_FIX32_M_CONVERSIONS
#define float2fix(f, e) (__builtin_constant_p(e) && (e) >= 1 && (e) <= 32 ? _float2fix_inline(f, e) : float2 ## fix(f, e))
#define float2ufix(f, e) (__builtin_constant_p(e) && (e) >= 1 && (e) <= 32 ? _float2ufix_inline(f, e) : float2 ## ufix(f, e))

#define _float2fix_inline(f, e) ({ \
    union { float _f; int32_t _i; } _u; \
    _u._f = (f); \
    uint rc, tmp; \
    pico_default_asm( \
        "vcvt.s32.f32 %0, %0, %4\n" \
        "vmov %2, %0\n" \
        "lsls %1, #1\n" \
        "bls 2f\n" /* positive or zero or -zero are ok with the result we have */ \
        "lsrs %3, %1, #24\n" \
        "subs %3, #0x7f - %c4\n" \
        "bcc 1f\n" /* 0 < abs(f) < 1 ^ e, so need to round down */ \
        /* mask off all but fractional bits */ \
        "lsls %1, %3\n" \
        "lsls %1, #8\n" \
        "beq 2f\n" /* integers can round towards zero */ \
        "1:\n" \
        /* need to subtract 1 from the result to round towards -infinity... */ \
        /* this will never cause an overflow, because to get here we must have had a non integer/infinite value which */ \
        /* therefore cannot have been equal to INT64_MIN when rounded towards zero */ \
        "subs %2, #1\n" \
        "2:\n" \
        : "+t" (_u._f), "+r" (_u._i), "=r" (rc), "=r" (tmp) \
        : "i" (e) \
    ); \
    rc; \
})
#define _float2ufix_inline(f, e) _float2ufix_z_inline((f), (e))
#endif
#endif

    // exp10f doesn't always appear in math.h but is present on all our platforms even for LIB_PICO_FLOAT_COMPILER
    // so we declare it here always

    //! Evaluate 10.0f to the power of the given value
    float exp10f(float x);

    // sincosf doesn't always appear in math.h but is present on all our platforms even for LIB_PICO_FLOAT_COMPILER
    // so we declare it here always
#if __PICO_FLOAT_ARM_OPTIMIZED && PICO_C_COMPILER_IS_CLANG
    // clang unhelpfully splits sincosf into explict calls to sin & cos
    extern void WRAPPER_FUNC(sincosf)(float x, float *sinx, float *cosx);
    #define sincosf(x, sinx, cosx) WRAPPER_FUNC(sincosf)(x, sinx, cosx)
#else
    //! Return both the sine and cosine of an angle efficiently
    void sincosf(float x, float *sinx, float *cosx);
#endif

#if PICO_FLOAT_HAS_POWINTF
#if !__PICO_FLOAT_ARM_OPTIMIZED && __has_builtin(__builtin_powif)
    static __force_inline float powintf(float f, int32_t p) {
        return __builtin_powif(f, p);
    }
#else
    //! Raise a floating point number to an integer power
    float powintf(float x, int32_t y);
#endif
#endif

#if PICO_FLOAT_HAS_FDIV_FAST
//! Perform a fast floating point divide with reduced accuracy
float fdiv_fast(float n, float d);
#endif

#if PICO_FLOAT_HAS_SQRTF_FAST
//! Perform a fast floating point square-root with reduced accuracy
float sqrtf_fast(float f);
#endif
//! \}

#undef __PICO_FLOAT_ARM_OPTIMIZED

#ifdef __cplusplus
}
#endif

#endif

#endif
