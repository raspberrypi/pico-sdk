/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_DOUBLE_H
#define _PICO_DOUBLE_H

#include <math.h>
#include "pico.h"
#include "pico/bootrom/sf_table.h"

#ifdef __cplusplus
extern "C" {
#endif

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
* - GNU exetnsions:
*
*   powint, sincos
*
* On Arm, the following additional optimized functions are also provided when using `pico_double_pico`:
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
* - Even faster versions of divide and square-root functions that do not round correctly:
*
*   ddiv_fast, sqrt_fast (these do not round correctly)
*
* - Faster unfused multiply and accumulate:
*
*   mla (fast fma)
*
* \if rp2350_specific
* On RISC-V there is no custom double-precision floating point support, so `pico_double_pico` is equivalent to `pico_double_compiler`
* \endif
*/
#if !defined(__riscv) || PICO_COMBINED_DOCS

#if PICO_COMBINED_DOCS || !LIB_PICO_DOUBLE_COMPILER
double int2double(int32_t i);
double uint2double(uint32_t i);
double int642double(int64_t i);
double uint642double(uint64_t i);
double fix2double(int32_t m, int e);
double ufix2double(uint32_t m, int e);
double fix642double(int64_t m, int e);
double ufix642double(uint64_t m, int e);

// These methods round towards 0, which IS the C way
int32_t double2int_z(double f);
int64_t double2int64_z(double f);
int32_t double2uint_z(double f);
int64_t double2uint64_z(double f);
int32_t double2fix_z(double f, int e);
uint32_t double2ufix_z(double f, int e);
int64_t double2fix64_z(double f, int e);
uint64_t double2ufix64_z(double f, int e);

// These methods round towards -Infinity - which IS NOT the C way for negative numbers;
// as such the naming is not ideal, however is kept for backwards compatibility
int32_t double2int(double f);
uint32_t double2uint(double f);
int64_t double2int64(double f);
uint64_t double2uint64(double f);
int32_t double2fix(double f, int e);
uint32_t double2ufix(double f, int e);
int64_t double2fix64(double f, int e);
uint64_t double2ufix64(double f, int e);

#endif

double exp10(double x);
void sincos(double x, double *sinx, double *cosx);
double powint(double x, int y);

#if !PICO_RP2040 || PICO_COMBINED_DOCS
double ddiv_fast(double n, double d);
double sqrt_fast(double f);
double fma_fast(double x, double y, double z); // this is not fused
double mla(double x, double y, double z); // another name for fma_fast
#endif

#endif

#if LIB_PICO_DOUBLE_COMPILER || defined(__riscv)
// when using the compiler; we provide as many functions as we trivially can, though in the double case they are not optimal
static inline double int2double(int32_t i) { return (double)i; }
static inline double uint2double(uint32_t i) { return (double)i; }
static inline double int642double(int64_t i) { return (double)i; }
static inline double uint642double(uint64_t i) { return (double)i; }

static inline int32_t double2int_z(double d) { return (int32_t)d; }
static inline int64_t double2int64_z(double d) { return (int64_t)d; }
static inline int32_t double2uint_z(double d) { return (uint32_t)d; }
static inline int64_t double2uint64_z(double d) { return (uint64_t)d; }
#endif

#ifdef __cplusplus
}
#endif

#endif
