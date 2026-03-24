/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_DOUBLE_H
#define _PICO_DOUBLE_H

/** \file double.h
* \defgroup pico_double pico_double
*
* \brief Optimized double-precision floating point functions
*
* An application can take control of the floating point routines used in the application over and above what is provided by the compiler,
* by depending on the pico_double library. A user might want to do this:
*
* 1. To use optimized software implementations provided by the RP2-series device's bootrom or the SDK
* 2. To use optimized combined software/hardware implementations utilizing custom RP2-series hardware for acceleration
* 3. To control the amount of C compiler/library code bloat
* 4. To make sure no floating point is called at all
*
* The pico_double library comes in three main flavors:
*
* 1. `pico_double_none` - all floating point operations cause a \ref panic - no double-precision floating point code is included
* 2. `pico_double_compiler` - no custom functions are provided; all double-precision floating point is handled by the C compiler/library
* 3. `pico_double_pico` - the smallest and fastest available for the platform, along with additional functionality (e.g. fixed point conversions) which are detailed below
*
* The user can control which version they want (e.g. **pico_double_xxx** by either setting the CMake global variable
* `PICO_DEFAULT_DOUBLE_IMPL=xxx`, or by using the CMake function `pico_set_double_implementation(<TARGET> xxx)`. Note that in the absence
* of either, pico_double_pico is used by default.
*
* \if rp2040_specific
* On RP2040, `pico_double_pico` uses optimized hand coded implementations from the bootrom and the SDK for both
* basic double-precision floating point operations and floating point math library functions. These implementations
* are generally faster and smaller than those provided by the C compiler/library, though they don't support all the features of a fully compliant
* floating point implementation; they are however usually fine for the majority of cases
* \endif
*
* \if rp2350_specific
* On RP2350, `pico_double_pico` uses RP2350 DCP instructions (double co-processor) to implement fast version of the basic
* arithmetic functions, and provides optimized M33 implementations of trignometric and scientific functions.
* These implementations are generally faster and smaller than those provided by the C compiler/library, though they don't support all the features of a fully compliant
* floating point implementation; they are however usually fine for the majority of cases
* \endif
*
* On Arm, (replacement) optimized implementations are provided for the following compiler built-ins
* and math library functions when using `pico_double_pico`:
*
* - basic arithmetic:
*
*   __aeabi_dadd, __aeabi_ddiv, __aeabi_dmul, __aeabi_drsub, __aeabi_dsub
*
* - comparison:
*
*   __aeabi_cfcmpeq, __aeabi_cfrcmple, __aeabi_cfcmple, __aeabi_dcmpeq, __aeabi_dcmplt, __aeabi_dcmple, __aeabi_dcmpge, __aeabi_dcmpgt, __aeabi_dcmpun
*
* - (u)int32 <-> double:
*
*    __aeabi_i2d, __aeabi_ui2d, __aeabi_d2iz, __aeabi_d2uiz
*
* - (u)int64 <-> double:
*
*   __aeabi_l2d, __aeabi_ul2d, __aeabi_d2lz, __aeabi_d2ulz
*
* - double -> float:
*
*   __aeabi_d2d
*
* - basic trigonometric:
*
*   sqrt, cos, sin, tan, atan2, exp, log
*
* - trigonometric and scientific
*
*   ldexp, copysign, trunc, floor, ceil, round, asin, acos, atan, sinh, cosh, tanh, asinh, acosh, atanh, exp2, log2, exp10, log10, pow, hypot, cbrt, fmod, drem, remainder, remquo, expm1, log1p, fma
*
* - GNU extensions:
*
*   sincos
*
* On Arm, the following additional optimized functions are also provided when using `pico_double_pico`, all of which
* saturate to the nearest representable value for too large input when converting from floating point types:
*
* - Conversions to/from integer types:
*
*   - (u)int -> double (round to nearest):
*
*     int2double, uint2double, int642double, uint642double
*
*   - (u)double -> int (round towards zero):
*
*     double2int_z, double2uint_z, double2int64_z, double2uint64_z
*
*   - (u)double -> int (round towards -infinity):
*
*     double2int, double2uint, double2int64, double2uint64
*
* - Conversions to/from fixed point integers:
*
*   - (u)fix -> double (round to nearest):
*
*       fix2double, ufix2double, fix642double, ufix642double
*
*   - double -> (u)fix (round towards zero):
*
*       double2fix_z, double2ufix_z, double2fix64_z, double2ufix64_z
*
*   - double -> (u)fix (round towards -infinity):
*
*       double2fix, double2ufix, double2fix64, double2ufix64
*
* - Scientific functions:
*
*    powint
* \if rp2350_specific
*
* - Even faster versions of divide and square-root functions that do not round correctly:
*
*   ddiv_fast, sqrt_fast
*
* - Faster un-fused multiply and accumulate:
*
*   mla/fma_fast
*
* On RISC-V there is no custom double-precision floating point support, so `pico_double_pico` is equivalent to `pico_double_compiler`
* \endif
*/

#include "pico.h"

// PICO_CONFIG: PICO_DOUBLE_IN_RAM, Force placement of SDK provided double-precision floating point code into RAM, type=bool, default=0, group=pico_double
#ifndef PICO_DOUBLE_IN_RAM
#define PICO_DOUBLE_IN_RAM 0
#endif

#if !(LIB_PICO_DOUBLE_COMPILER || defined(__riscv)) || PICO_DOCS
// private define to simplify this header only - it is undefined at the end
#define __PICO_DOUBLE_ARM_OPTIMIZED 1
#endif

//! \addtogroup pico_double
//! \{

// we always define these for C code, but they are inline
// funcs except for __PICO_DOUBLE_ARM_OPTIMIZED so wouldn't
// be callable for assembly
#if __PICO_DOUBLE_ARM_OPTIMIZED || !defined(__ASSEMBLER__)
//! Set if \ref int2double and \ref uint2double are available
#define PICO_DOUBLE_HAS_INT32_TO_DOUBLE_CONVERSIONS 1
//! Set if \ref int642double and \ref uint642double are available
#define PICO_DOUBLE_HAS_INT64_TO_DOUBLE_CONVERSIONS 1
//! Set if \ref double2int_z and \ref double2uint_z are available (rounding towards zero)
#define PICO_DOUBLE_HAS_DOUBLE_TO_INT32_Z_CONVERSIONS 1
//! Set if \ref double2int64_z and \ref double2uint64_z are available (rounding towards zero)
#define PICO_DOUBLE_HAS_DOUBLE_TO_INT64_Z_CONVERSIONS 1
#endif

#if __PICO_DOUBLE_ARM_OPTIMIZED
//! Set if \ref fix2double and \ref ufix2double are available
#define PICO_DOUBLE_HAS_FIX32_TO_DOUBLE_CONVERSIONS 1
//! Set if \ref fix642double and \ref ufix642double are available
#define PICO_DOUBLE_HAS_FIX64_TO_DOUBLE_CONVERSIONS 1
//! Set if \ref double2fix_z and \ref double2ufix_z are available (rounding towards zero)
#define PICO_DOUBLE_HAS_DOUBLE_TO_FIX32_Z_CONVERSIONS 1
//! Set if \ref double2fix64_z and \ref double2ufix64_z are available (rounding towards zero)
#define PICO_DOUBLE_HAS_DOUBLE_TO_FIX64_Z_CONVERSIONS 1

//! Set if \ref double2int and \ref double2uint are available (rounding towards -Infinity)
#define PICO_DOUBLE_HAS_DOUBLE_TO_INT32_M_CONVERSIONS 1
//! Set if \ref double2int64 and \ref double2uint64 are available (rounding towards -Infinity)
#define PICO_DOUBLE_HAS_DOUBLE_TO_INT64_M_CONVERSIONS 1

//! Set if \ref double2fix and \ref double2ufix are available (rounding towards -Infinity)
#define PICO_DOUBLE_HAS_DOUBLE_TO_FIX32_M_CONVERSIONS 1
//! Set if \ref double2fix64 and \ref double2ufix64 are available (rounding towards -Infinity)
#define PICO_DOUBLE_HAS_DOUBLE_TO_FIX64_M_CONVERSIONS 1
#endif

#if (PICO_RP2350 && LIB_PICO_DOUBLE_PICO_DCP) || PICO_DOCS
//! Set if \ref ddiv_fast is available
#define PICO_DOUBLE_HAS_DDIV_FAST 1
//! Set if \ref sqrt_fast is available
#define PICO_DOUBLE_HAS_SQRT_FAST 1
//! Set if \ref fma_fast is available
#define PICO_DOUBLE_HAS_FMA_FAST 1
#endif

#if __PICO_DOUBLE_ARM_OPTIMIZED || __builtin_powi || PICO_DOCS
//! Set if \ref powint is available
#define PICO_DOUBLE_HAS_POWINT 1
#endif
//! \}

#ifndef __ASSEMBLER__
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

//! \addtogroup pico_double
//! \{
#if PICO_DOUBLE_HAS_INT32_TO_DOUBLE_CONVERSIONS
#if !__PICO_DOUBLE_ARM_OPTIMIZED
    // for non Arm-optimized we may as well provide the function and let the compiler handle it
    static inline double int2double(int32_t i) { return (double)i; }
    static inline double uint2double(uint32_t i) { return (double)i; }
#else
    //! Convert a signed 32-bit integer to the nearest double
    double int2double(int32_t i);
    //! Convert an unsigned 32-bit integer to the nearest double
    double uint2double(uint32_t i);
#endif
#endif

#if PICO_DOUBLE_HAS_INT64_TO_DOUBLE_CONVERSIONS
#if !__PICO_DOUBLE_ARM_OPTIMIZED
    // for non Arm-optimized we may as well provide the function and let the compiler handle it
    static inline double int642double(int64_t i) { return (double)i; }
    static inline double uint642double(uint64_t i) { return (double)i; }
#else
    //! Convert a signed 64-bit integer to the nearest double
    double int642double(int64_t i);
    //! Convert an unsigned 64-bit integer to the nearest double
    double uint642double(uint64_t i);
#endif
#endif

#if PICO_DOUBLE_HAS_DOUBLE_TO_INT32_Z_CONVERSIONS
#if !__PICO_DOUBLE_ARM_OPTIMIZED
    // for non Arm-optimized we may as well provide the function and let the compiler handle it
    static inline int32_t double2int_z(double f) { return (int32_t)f; }
    static inline int32_t double2uint_z(double f) { return (uint32_t)f; }
#else
    //! \brief Convert a double to a signed 32-bit integer, rounding towards zero.
    //! On Arm this conversion is saturating (to INT32_MAX/INT32_MIN) for out of range input except when using `pico_double_compiler`
    int32_t double2int_z(double f);
    //! \brief Convert a double to an unsigned 32-bit integer, rounding towards zero
    //! On Arm this conversion is saturating (to UINT32_MAX/UINT32_MIN) for out of range input except when using `pico_double_compiler`
    int32_t double2uint_z(double f);
#endif
#endif

#if PICO_DOUBLE_HAS_DOUBLE_TO_INT64_Z_CONVERSIONS
#if !__PICO_DOUBLE_ARM_OPTIMIZED
    // for non Arm-optimized we may as well provide the function and let the compiler handle it
    static inline int64_t double2int64_z(double f) { return (int64_t)f; }
    static inline int64_t double2uint64_z(double f) { return (uint64_t)f; }
#else
    //! \brief Convert a double to a signed 64-bit integer, rounding towards zero.
    //! On Arm this conversion is saturating (to INT64_MAX/INT64_MIN) for out of range input except when using `pico_double_compiler`
    int64_t double2int64_z(double f);
    //! \brief Convert a double to an unsigned 64-bit integer, rounding towards zero.
    //! On Arm this conversion is saturating (to UINT64_MAX/UINT64_MIN) for out of range input except when using `pico_double_compiler`
    int64_t double2uint64_z(double f);
#endif
#endif

#if PICO_DOUBLE_HAS_FIX32_TO_DOUBLE_CONVERSIONS
//! \brief Convert a signed 32-bit integer with the given number of fractional bits to the nearest double
//! Out of range inputs will convert to +/- Infinity
double fix2double(int32_t m, int e);
//! \brief Convert an unsigned 32-bit integer with the given number of fractional bits to the nearest double
//! Out of range inputs will convert to +Infinity
double ufix2double(uint32_t m, int e);
#endif

#if PICO_DOUBLE_HAS_FIX64_TO_DOUBLE_CONVERSIONS
//! \brief Convert a signed 64-bit integer with the given number of fractional bits to the nearest double
//! Out of range inputs will convert to +/- Infinity
double fix642double(int64_t m, int e);
//! \brief Convert an unsigned 64-bit integer with the given number of fractional bits to the nearest double
//! Out of range inputs will convert to +Infinity
double ufix642double(uint64_t m, int e);
#endif

#if PICO_DOUBLE_HAS_DOUBLE_TO_FIX32_Z_CONVERSIONS
//! \brief Convert a double to a signed 32-bit fixed-point integer with the given number of fractional bits, rounding towards zero.
//! On Arm this conversion is saturating (to INT32_MAX/INT32_MIN) for out of range input except when using `pico_double_compiler`
int32_t double2fix_z(double f, int e);
//! \brief Convert a double to an unsigned 32-bit fixed-point integer with the given number of fractional bits, rounding towards zero.
//! This conversion is saturating (to UINT32_MAX/UINT32_MIN) for out of range input
uint32_t double2ufix_z(double f, int e);
#endif

#if PICO_DOUBLE_HAS_DOUBLE_TO_FIX64_Z_CONVERSIONS
//! \brief Convert a double to a signed 64-bit fixed-point integer with the given number of fractional bits, rounding towards zero.
//! On Arm this conversion is saturating (to INT64_MAX/INT64_MIN) for out of range input except when using `pico_double_compiler`
int64_t double2fix64_z(double f, int e);
//! \brief Convert a double to an unsigned 64-bit fixed-point integer with the given number of fractional bits, rounding towards zero.
//! This conversion is saturating (to UINT64_MAX/UINT64_MIN) for out of range input
uint64_t double2ufix64_z(double f, int e);
#endif

// These methods round towards -Infinity - which IS NOT the C way for negative numbers;
// as such the naming is not ideal, however is kept for backwards compatibility
#if PICO_DOUBLE_HAS_DOUBLE_TO_INT32_M_CONVERSIONS
//! \brief Convert a double to a signed 32-bit integer, rounding towards -Infinity.
//! This conversion is saturating (to INT32_MAX/INT32_MIN) for out of range input
int32_t double2int(double f);
//! \brief Convert a double to an unsigned 32-bit integer, rounding towards -Infinity.
//! This conversion is saturating (to UINT32_MAX/UINT32_MIN) for out of range input
uint32_t double2uint(double f);
#endif

#if PICO_DOUBLE_HAS_DOUBLE_TO_INT64_M_CONVERSIONS
//! \brief Convert a double to a signed 64-bit integer, rounding towards -Infinity.
//! This conversion is saturating (to INT64_MAX/INT64_MIN) for out of range input
int64_t double2int64(double f);
//! \brief Convert a double to an usigned 64-bit integer, rounding towards -Infinity.
//! This conversion is saturating (to UINT64_MAX/UINT64_MIN) for out of range input
uint64_t double2uint64(double f);
#endif

#if PICO_DOUBLE_HAS_DOUBLE_TO_FIX32_M_CONVERSIONS
//! \brief Convert a double to a signed 32-bit fixed-point integer with the given number of fractional bits, rounding towards -Infinity.
//! This conversion is saturating (to INT32_MAX/INT32_MIN) for out of range input
int32_t double2fix(double f, int e);
//! \brief Convert a double to an unsigned 32-bit fixed-point integer with the given number of fractional bits, rounding towards -Infinity.
//! This conversion is saturating (to UINT32_MAX/UINT32_MIN) for out of range input
uint32_t double2ufix(double f, int e);
#endif

#if PICO_DOUBLE_HAS_DOUBLE_TO_FIX64_M_CONVERSIONS
//! \brief Convert a double to a signed 64-bit fixed-point integer with the given number of fractional bits, rounding towards -Infinity.
//! This conversion is saturating (to INT64_MAX/INT64_MIN) for out of range input
int64_t double2fix64(double f, int e);
//! \brief Convert a double to an unsigned 64-bit fixed-point integer with the given number of fractional bits, rounding towards -Infinity.
//! This conversion is saturating (to UINT64_MAX/UINT64_MIN) for out of range input
uint64_t double2ufix64(double f, int e);
#endif

    // exp10 doesn't always appear in math.h but is present on all our platforms even for LIB_PICO_DOUBLE_COMPILER
    // so we declare it here always

    //! Evaluate 10.0 to the power of the given value
    double exp10(double x);

    // sincos doesn't always appear in math.h but is present on all our platforms even for LIB_PICO_DOUBLE_COMPILER
    // so we declare it here always
#if __PICO_DOUBLE_ARM_OPTIMIZED && PICO_C_COMPILER_IS_CLANG
    // clang unhelpfully splits sincos into explict calls to sin & cos
    extern void WRAPPER_FUNC(sincos)(double x, double *sinx, double *cosx);
    #define sincos(x, sinx, cosx) WRAPPER_FUNC(sincos)(x, sinx, cosx)
#else
    //! Return both the sine and cosine of an angle efficiently
    void sincos(double x, double *sinx, double *cosx);
#endif

#if PICO_DOUBLE_HAS_POWINT
#if !__PICO_DOUBLE_ARM_OPTIMIZED && __has_builtin(__builtin_powi)
    static __force_inline double powint(double f, int32_t p) {
        return __builtin_powi(f, p);
    }
#else
    //! Raise a floating point number to an integer power
    double powint(double x, int32_t y);
#endif
#endif

#if PICO_DOUBLE_HAS_DDIV_FAST
//! Perform a fast floating point divide with reduced accuracy
double ddiv_fast(double n, double d);
#endif

#if PICO_DOUBLE_HAS_SQRT_FAST
//! Perform a fast floating point square-root with reduced accuracy
double sqrt_fast(double f);
#endif

#if PICO_DOUBLE_HAS_FMA_FAST
//! Perform a fast (non-fused) multiply-add (x * y + z) with reduced accuracy
double fma_fast(double x, double y, double z);
//! Perform a fast multiply-add (x * y + z) with reduced accuracy (not fused multiply-add). This is another name for \ref fma_fast
double mla(double x, double y, double z); // another name for fma_fast
#endif
//! \}

#undef __PICO_DOUBLE_ARM_OPTIMIZED

#ifdef __cplusplus
}
#endif

#endif

#endif
