#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/float.h"
#include "math.h"

#if 0
#define printf(...) ((void)0)
#endif
#if 0
#define stop() return -1
#else
#define stop() rc=1
#endif
#define test_assert(x) ({ if (!(x)) { printf("Assertion failed: ");puts(#x);printf("  at " __FILE__ ":%d\n", __LINE__); stop(); } })
#define test_checkf(x, expected, msg) ({ if ((x) != (expected)) { printf("  %s: %f != %f\n", msg, x, expected); stop(); } })
#define test_checki(x, expected, msg) ({ if ((x) != (expected)) { printf("  %s: %d != %d\n", msg, x, expected); stop(); } })
#define test_checku(x, expected, msg) ({ if ((uint32_t)(x) != (uint32_t)(expected)) { printf("  %s: %u != %u\n", msg, x, expected); stop(); } })
#define test_checki64(x, expected, msg) ({ if ((x) != (expected)) { printf("  %s: %lld != %lld\n", msg, (int64_t)(x), (int64_t)(expected)); stop(); } })
#define test_checku64(x, expected, msg) ({ if ((uint64_t)(x) != (uint64_t)(expected)) { printf("  %s: %llu != %llu\n", msg, (uint64_t)(x), (uint64_t)(expected)); stop(); } })

#if !(LIB_PICO_FLOAT_COMPILER || defined(__riscv))
static inline float fix2float_8(int32_t m) { return fix2float(m, 8); }
static inline float fix2float_12(int32_t m) { return fix2float(m, 12); }
static inline float fix2float_16(int32_t m) { return fix2float(m, 16); }
static inline float fix2float_24(int32_t m) { return fix2float(m, 24); }
static inline float fix2float_28(int32_t m) { return fix2float(m, 28); }
static inline float fix2float_32(int32_t m) { return fix2float(m, 32); }

static inline float ufix2float_12(int32_t m) { return ufix2float(m, 12); }

static inline float float2fix_12(int32_t m) { return float2fix(m, 12); }

static inline float float2ufix_12(int32_t m) { return float2ufix(m, 12); }
#endif

#if LIB_PICO_FLOAT_COMPILER || defined(__riscv)
#if __SOFTFP__ || defined(__riscv)
#define FREG "+r"
#else
#define FREG "+t"
#endif
// prevent the compiler from eliding the calculations
#define float2int_z(f) ({ float _f = f; pico_default_asm_volatile("" : FREG (_f)); float2 ## int_z(_f); })
#define float2uint_z(f) ({ float _f = f; pico_default_asm_volatile("" : FREG (_f)); float2 ## uint_z(_f); })
#define float2int64_z(f) ({ float _f = f; pico_default_asm_volatile("" : FREG (_f)); float2 ## int64_z(_f); })
#define float2uint64_z(f) ({ float _f = f; pico_default_asm_volatile("" : FREG (_f)); float2 ## uint64_z(_f); })
#define int2float(i) ({ int32_t _i = i; pico_default_asm_volatile("" : "+r" (_i)); int2 ## float(_i); })
#define uint2float(i) ({ uint32_t _i = i; pico_default_asm_volatile("" : "+r" (_i)); uint2 ## float(_i); })
#define int642float(i) ({ int64_t _i = i; pico_default_asm_volatile("" : "+r" (_i)); int642 ## float(_i); })
#define uint642float(i) ({ uint64_t _i = i; pico_default_asm_volatile("" : "+r" (_i)); uint642 ## float(_i); })
#endif

float make_positive_denormal_float(void) {
    union {
        float f;
        uint32_t u;
    } x;
    x.u = 0x00000001u;
    return x.f;
}

float make_negative_denormal_float(void) {
    union {
        float f;
        uint32_t u;
    } x;
    x.u = 0x80000001u;
    return x.f;
}

#if LIB_PICO_FLOAT_PICO_VFP
float __attribute__((naked)) call_int2float(int32_t i) {
    pico_default_asm_volatile("b int2float");
}

float __attribute__((naked)) call_uint2float(uint32_t i) {
    pico_default_asm_volatile("b uint2float");
}

int32_t __attribute__((naked)) call_float2int_z(float f) {
    pico_default_asm_volatile("b float2int_z");
}

uint32_t __attribute__((naked)) call_float2uint_z(float f) {
    pico_default_asm_volatile("b float2uint_z");
}

float __attribute__((naked)) call_fix2float(int32_t i, uint32_t n) {
    pico_default_asm_volatile("b fix2float");
}

float __attribute__((naked)) call_ufix2float(int32_t i, uint32_t n) {
    pico_default_asm_volatile("b ufix2float");
}

int32_t __attribute__((naked)) call_float2fix(float f, uint32_t n) {
    pico_default_asm_volatile("b float2fix");
}

uint32_t __attribute__((naked)) call_float2ufix(float f, uint32_t n) {
    pico_default_asm_volatile("b float2ufix");
}

int32_t __attribute__((naked)) call_float2fix_z(float f, uint32_t n) {
    pico_default_asm_volatile("b float2fix_z");
}

uint32_t __attribute__((naked)) call_float2ufix_z(float f, uint32_t n) {
    pico_default_asm_volatile("b float2ufix_z");
}
#endif

int test() {
    int rc = 0;
    printf("=== custom_float_funcs_test ");
#if PICO_C_COMPILER_IS_CLANG
    printf("(LLVM)");
#elif PICO_C_COMPILER_IS_GNU
    printf("(GCC)");
#endif
#if defined(__ARM_PCS_VFP)
    printf(" (hard)");
#endif
    printf("\n");
#if LIB_PICO_FLOAT_PICO_DCP
    printf("--- using DCP\n");
#elif LIB_PICO_FLOAT_PICO_VFP
    printf("--- using VFP\n");
#elif LIB_PICO_FLOAT_COMPILER
    printf("--- using compiler\n");
#endif
    printf("int2float\n");
    test_checkf(int2float(0), 0.0f, "int2float1");
    test_checkf(int2float(-1), -1.0f, "int2float2");
    test_checkf(int2float(1), 1.0f, "int2float3");
    test_checkf(int2float(INT32_MAX), 2147483647.0f, "int2float4");
    test_checkf(int2float(INT32_MIN), -2147483648.0f, "int2float5");
    // check rounding
    test_checkf(int2float(2147483391), 2147483392.0f, "int2float6");
    test_checkf(int2float(2147483456), 2147483392.0f, "int2float7");
    test_checkf(int2float(2147483457), 2147483520.0f, "int2float8");
    test_checkf(int2float(2147483483), 2147483520.0f, "int2float9");
    test_checkf(int2float(2147483584), 2147483648.0f, "int2float10");

#if LIB_PICO_FLOAT_PICO_VFP
    printf("call_int2float\n");
    test_checkf(call_int2float(0), 0.0f, "call_int2float1");
    test_checkf(call_int2float(-1), -1.0f, "call_int2float2");
    test_checkf(call_int2float(1), 1.0f, "call_int2float3");
    test_checkf(call_int2float(INT32_MAX), 2147483647.0f, "call_int2float4");
    test_checkf(call_int2float(INT32_MIN), -2147483648.0f, "call_int2float5");
    // check rounding
    test_checkf(call_int2float(2147483391), 2147483392.0f, "call_int2float6");
    test_checkf(call_int2float(2147483456), 2147483392.0f, "call_int2float7");
    test_checkf(call_int2float(2147483457), 2147483520.0f, "call_int2float8");
    test_checkf(call_int2float(2147483483), 2147483520.0f, "call_int2float9");
    test_checkf(call_int2float(2147483584), 2147483648.0f, "call_int2float10");
#endif

    printf("uint2float\n");
    test_checkf(uint2float(0), 0.0f, "uint2float1");
    test_checkf(uint2float(1), 1.0f, "uint2float2");
    test_checkf(uint2float(INT32_MAX), 2147483647.0f, "uint2float3");
    // todo test correct rounding around maximum precision
    test_checkf(uint2float(UINT32_MAX), 4294967295.0f, "uint2float4");

#if LIB_PICO_FLOAT_PICO_VFP
    printf("call_uint2float\n");
    test_checkf(call_uint2float(0), 0.0f, "call_uint2float1");
    test_checkf(call_uint2float(1), 1.0f, "call_uint2float2");
    test_checkf(call_uint2float(INT32_MAX), 2147483647.0f, "call_uint2float3");
    // todo test correct rounding around maximum precision
    test_checkf(call_uint2float(UINT32_MAX), 4294967295.0f, "call_uint2float4");
#endif

    printf("int642float\n");
    test_checkf(int642float(0), 0.0f, "int642float1");
    test_checkf(int642float(-1), -1.0f, "int642float2");
    test_checkf(int642float(1), 1.0f, "int642float3");
    test_checkf(int642float(INT32_MAX-1), 2147483646.0f, "int642float4"); // note equality is within 1ulp
    test_checkf(int642float(INT32_MAX), 2147483647.0f, "int642float5"); // note equality is within 1ulp
    test_checkf(int642float(INT32_MAX+1ll), 2147483648.0f, "int642float6");
    test_checkf(int642float(INT32_MIN-1ll), -2147483649.0f, "int642float7"); // note equality is within 1ulp
    test_checkf(int642float(INT32_MIN), -2147483648.0f, "int642float8");
    test_checkf(int642float(INT32_MIN+1ll), -2147483647.0f, "int642float9"); // note equality is within 1ulp
    // todo test correct rounding around maximum precision
    test_checkf(int642float(INT64_MAX), 9223372036854775807.0f, "int642float10");
    test_checkf(int642float(INT64_MIN), -9223372036854775808.0f, "int642float11");

    printf("uint642float\n");
    test_checkf(uint642float(0), 0.0f, "uint642float1");
    test_checkf(uint642float(1), 1.0f, "uint642float2");
    test_checkf(uint642float(INT32_MAX-1), 2147483646.0f, "uint642float3"); // note equality is within 1ulp
    test_checkf(uint642float(INT32_MAX), 2147483647.0f, "uint642float4"); // note equality is within 1ulp
    test_checkf(uint642float(INT32_MAX+1ll), 2147483648.0f, "uint642float5");
    test_checkf(uint642float(INT64_MAX), 9223372036854775807.0f, "uint642float6");
    // todo test correct rounding around maximum precision
    test_checkf(uint642float(UINT64_MAX), 18446744073709551615.0f, "uint642float7");

    union {
        uint32_t u;
        float f;
    } u32f;

#if !(LIB_PICO_FLOAT_COMPILER || defined(__riscv))
    printf("fix2float\n");
    // todo test correct rounding around maximum precision
    test_checkf(fix2float(-3, 1), -1.5f, "fix2float1");
    test_checkf(fix2float(-3, 1), -1.5f, "fix2float2");
    test_checkf(fix2float(-3, -4), -48.0f, "fix2float3");

#if LIB_PICO_FLOAT_PICO_VFP
#ifndef fix2float
#error fix2float not actually overridden
#endif
    printf("call_fix2float\n");
    // todo test correct rounding around maximum precision
    test_checkf(call_fix2float(-3, 1), -1.5f, "call_fix2float1");
    test_checkf(call_fix2float(-3, 1), -1.5f, "call_fix2float2");
    test_checkf(call_fix2float(-3, -4), -48.0f, "call_fix2float3");
#endif

    printf("ufix2float\n");
    // todo test correct rounding around maximum precision
    test_checkf(ufix2float(0xa0000000, 30), 2.5f, "ufix2float1");
    test_checkf(ufix2float(3, -4), 48.0f, "ufix2float2");

#if LIB_PICO_FLOAT_PICO_VFP
#ifndef ufix2float
#error ufix2float not actually overridden
#endif

    printf("call_ufix2float\n");
    // todo test correct rounding around maximum precision
    test_checkf(call_ufix2float(0xa0000000, 30), 2.5f, "call_ufix2float1");
    test_checkf(call_ufix2float(3, -4), 48.0f, "call_ufix2float2");
#endif

    printf("fix642float\n");
    // todo test correct rounding around maximum precision
    test_checkf(fix642float(-0xa000000000ll, 38), -2.5f, "fix642float1");
    test_checkf(fix642float(-3, -34), -51539607552.0f, "fix642float2");

#ifdef fix642float
#error fix642float overridden, so original needs testing
#endif

    printf("ufix642float\n");
    // todo test correct rounding around maximum precision
    test_checkf(ufix642float(0xa000000000ll, 38), 2.5f, "ufix642float1");
    test_checkf(ufix642float(3, -34), 51539607552.0f, "ufix642float2");

#ifdef ufix642float
#error ufix642float overridden, so original needs testing
#endif

    printf("fix2float_N\n");
    test_checkf(fix2float_8(128), 0.5f, "fix2float_8_1");
    test_checkf(fix2float_8(-128), -0.5f, "fix2float_8_2");
    test_checkf(fix2float_16(8192), 0.125f, "fix2float_8_3");
    test_checkf(fix2float_16(-8192), -0.125f, "fix2float_8_4");
    test_checkf(fix2float_24(3<<23), 1.5f, "fix2float_8_5");
    test_checkf(fix2float_24(-(3<<23)), -1.5f, "fix2float_8_6");

    printf("float2fix\n");
    test_checki(float2fix(-0.5f, 8), -0x80, "float2fix0");
    test_checki(float2fix(3.5f, 8), 0x380, "float2fix1");
    test_checki(float2fix(-3.5f, 8), -0x380, "float2fix2");
    test_checki(float2fix(32768.0f, 16), INT32_MAX, "float2fix3");
    test_checki(float2fix(65536.0f, 16), INT32_MAX, "float2fix4");
    test_checki(float2fix(-65536.0f, 16), INT32_MIN, "float2fix4b");
    test_checki(float2fix(INFINITY, 16), INT32_MAX, "float2fix5");
    test_checki(float2fix(-INFINITY, 16), INT32_MIN, "float2fix5b");
    test_checki(float2fix(INFINITY, -16), INT32_MAX, "float2fix5c");
    test_checki(float2fix(-INFINITY, -16), INT32_MIN, "float2fix5d");
    test_checki(float2fix(INFINITY, 0), INT32_MAX, "float2fix5e");
    test_checki(float2fix(-INFINITY, 0), INT32_MIN, "float2fix5f");
    test_checki(float2fix(3.24999f, 2), 12, "float2fix6");
    test_checki(float2fix(3.25f, 2), 13, "float2fix7");
    test_checki(float2fix(-3.24999f, 2), -13, "float2fix8");
    test_checki(float2fix(-3.25f, 2), -13, "float2fix9");
    test_checki(float2fix(-0.75f, 1), -2, "float2fix10");
    test_checki(float2fix(-3.0f, -1), -2, "float2fix11"); // not very useful
    u32f.u = 0x7f012345;
    test_checki(float2fix(u32f.f, 0), INT32_MAX, "float2fix12a");
    test_checki(float2fix(u32f.f, 1), INT32_MAX, "float2fix12b");
    test_checki(float2fix(u32f.f, 2), INT32_MAX, "float2fix12c");
    u32f.u = 0xff012345;
    test_checki(float2fix(u32f.f, 0), INT32_MIN, "float2fix13a");
    test_checki(float2fix(u32f.f, 1), INT32_MIN, "float2fix13b");
    test_checki(float2fix(u32f.f, 2), INT32_MIN, "float2fix13c");

#if LIB_PICO_FLOAT_PICO_VFP
#ifndef float2fix
#error float2fix not actually overridden
#endif
    printf("call_float2fix\n");
    test_checki(call_float2fix(-0.5f, 8), -0x80, "call_float2fix0");
    test_checki(call_float2fix(3.5f, 8), 0x380, "call_float2fix1");
    test_checki(call_float2fix(-3.5f, 8), -0x380, "call_float2fix2");
    test_checki(call_float2fix(32768.0f, 16), INT32_MAX, "call_float2fix3");
    test_checki(call_float2fix(65536.0f, 16), INT32_MAX, "call_float2fix4");
    test_checki(call_float2fix(-65536.0f, 16), INT32_MIN, "call_float2fix4b");
    test_checki(call_float2fix(INFINITY, 16), INT32_MAX, "call_float2fix5");
    test_checki(call_float2fix(-INFINITY, 16), INT32_MIN, "call_float2fix5b");
    test_checki(call_float2fix(INFINITY, -16), INT32_MAX, "call_float2fix5c");
    test_checki(call_float2fix(-INFINITY, -16), INT32_MIN, "call_float2fix5d");
    test_checki(call_float2fix(INFINITY, 0), INT32_MAX, "call_float2fix5e");
    test_checki(call_float2fix(-INFINITY, 0), INT32_MIN, "call_float2fix5f");
    test_checki(call_float2fix(3.24999f, 2), 12, "call_float2fix6");
    test_checki(call_float2fix(3.25f, 2), 13, "call_float2fix7");
    test_checki(call_float2fix(-3.24999f, 2), -13, "call_float2fix8");
    test_checki(call_float2fix(-3.25f, 2), -13, "call_float2fix9");
    test_checki(call_float2fix(-0.75f, 1), -2, "call_float2fix10");
    test_checki(call_float2fix(-3.0f, -1), -2, "call_float2fix11"); // not very useful
    u32f.u = 0x7f012345;
    test_checki(call_float2fix(u32f.f, 0), INT32_MAX, "call_float2fix12a");
    test_checki(call_float2fix(u32f.f, 1), INT32_MAX, "call_float2fix12b");
    test_checki(call_float2fix(u32f.f, 2), INT32_MAX, "call_float2fix12c");
    u32f.u = 0xff012345;
    test_checki(call_float2fix(u32f.f, 0), INT32_MIN, "call_float2fix13a");
    test_checki(call_float2fix(u32f.f, 1), INT32_MIN, "call_float2fix13b");
    test_checki(call_float2fix(u32f.f, 2), INT32_MIN, "call_float2fix13c");
#endif

    printf("float2ufix\n");
    test_checku(float2ufix(3.5f, 8), 0x380, "float2ufix1");
    test_checku(float2ufix(-3.5f, 8), 0, "float2ufix2");
    test_checku(float2ufix(32768.0f, 16), 32768 << 16, "float2ufix3");
    test_checku(float2ufix(65536.0f, 16), UINT32_MAX, "float2ufix4");
    test_checku(float2ufix(INFINITY, 16), UINT32_MAX, "float2ufix5");
    test_checku(float2ufix(-INFINITY, 16), 0, "float2ufix5b");
    test_checku(float2ufix(INFINITY, -16), UINT32_MAX, "float2ufix5c");
    test_checku(float2ufix(-INFINITY, -16), 0, "float2ufix5d");
    test_checku(float2ufix(INFINITY, 0), UINT32_MAX, "float2ufix5e");
    test_checku(float2ufix(-INFINITY, 0), 0, "float2ufix5f");
    test_checku(float2ufix(3.24999f, 2), 12, "float2ufix6");
    test_checku(float2ufix(3.25f, 2), 13, "float2ufix7");
    test_checku(float2ufix(3.0f, -1), 1, "float2ufix8"); // not very useful
    u32f.u = 0x7f012345;
    test_checku(float2ufix(u32f.f, 0), UINT32_MAX, "float2ufix9a");
    test_checku(float2ufix(u32f.f, 1), UINT32_MAX, "float2ufix9b");
    test_checku(float2ufix(u32f.f, 2), UINT32_MAX, "float2ufix9c");
    u32f.u = 0xff012345;
    test_checku(float2ufix(u32f.f, 0), 0, "float2ufix10a");
    test_checku(float2ufix(u32f.f, 1), 0, "float2ufix10b");
    test_checku(float2ufix(u32f.f, 2), 0, "float2ufix10c");

#if LIB_PICO_FLOAT_PICO_VFP
#ifndef float2ufix
#error float2ufix not actually overridden
#endif
    printf("call_float2ufix\n");
    test_checku(call_float2ufix(3.5f, 8), 0x380, "call_float2ufix1");
    test_checku(call_float2ufix(-3.5f, 8), 0, "call_float2ufix2");
    test_checku(call_float2ufix(32768.0f, 16), 32768 << 16, "call_float2ufix3");
    test_checku(call_float2ufix(65536.0f, 16), UINT32_MAX, "call_float2ufix4");
    test_checku(call_float2ufix(INFINITY, 16), UINT32_MAX, "call_float2ufix5");
    test_checku(call_float2ufix(-INFINITY, 16), 0, "call_float2ufix5b");
    test_checku(call_float2ufix(INFINITY, -16), UINT32_MAX, "call_float2ufix5c");
    test_checku(call_float2ufix(-INFINITY, -16), 0, "call_float2ufix5d");
    test_checku(call_float2ufix(INFINITY, 0), UINT32_MAX, "call_float2ufix5e");
    test_checku(call_float2ufix(-INFINITY, 0), 0, "call_float2ufix5f");
    test_checku(call_float2ufix(3.24999f, 2), 12, "call_float2ufix6");
    test_checku(call_float2ufix(3.25f, 2), 13, "call_float2ufix7");
    test_checku(call_float2ufix(3.0f, -1), 1, "call_float2ufix8"); // not very useful
    u32f.u = 0x7f012345;
    test_checku(call_float2ufix(u32f.f, 0), UINT32_MAX, "call_float2ufix9a");
    test_checku(call_float2ufix(u32f.f, 1), UINT32_MAX, "call_float2ufix9b");
    test_checku(call_float2ufix(u32f.f, 2), UINT32_MAX, "call_float2ufix9c");
    u32f.u = 0xff012345;
    test_checku(call_float2ufix(u32f.f, 0), 0, "call_float2ufix10a");
    test_checku(call_float2ufix(u32f.f, 1), 0, "call_float2ufix10b");
    test_checku(call_float2ufix(u32f.f, 2), 0, "call_float2ufix10c");
#endif

    printf("float2fix64\n");
    test_checki64(float2fix64(3.5f, 8), 0x380, "float2fix641");
    test_checki64(float2fix64(-3.5f, 8), -0x380, "float2fix642");
    test_checki64(float2fix64(32768.0f, 16), 32768ll << 16, "float2fix643");
    test_checki64(float2fix64(65536.0f, 16), 65536ll << 16, "float2fix644");
    test_checki64(float2fix64(2147483648.0f, 16), 2147483648ll << 16, "float2fix644b");
    test_checki64(float2fix64(65536.0f * 65536.0f * 32768.0f, 16), INT64_MAX, "float2fix644c");
    test_checki64(float2fix64(INFINITY, 16), INT64_MAX, "float2fix645");
    test_checki64(float2fix64(-INFINITY, 16), INT64_MIN, "float2fix645b");
    test_checki64(float2fix64(INFINITY, -16), INT64_MAX, "float2fix645c");
    test_checki64(float2fix64(-INFINITY, -16), INT64_MIN, "float2fix645d");
    test_checki64(float2fix64(INFINITY, 0), INT64_MAX, "float2fix645e");
    test_checki64(float2fix64(-INFINITY, 0), INT64_MIN, "float2fix645f");
    test_checki64(float2fix64(3.24999f, 2), 12, "float2fix646");
    test_checki64(float2fix64(3.25f, 2), 13, "float2fix647");
    test_checki64(float2fix64(-3.24999f, 2), -13, "float2fix648");
    test_checki64(float2fix64(-3.25f, 2), -13, "float2fix649");
    test_checki64(float2fix64(-3.0f, -1), -2, "float2fix6410"); // not very useful
    u32f.u = 0x7f012345;
    test_checki64(float2fix64(u32f.f, 0), INT64_MAX, "float2fix6411a");
    test_checki64(float2fix64(u32f.f, 1), INT64_MAX, "float2fix6411b");
    test_checki64(float2fix64(u32f.f, 2), INT64_MAX, "float2fix6411c");
    u32f.u = 0xff012345;
    test_checki64(float2fix64(u32f.f, 0), INT64_MIN, "float2fix6412a");
    test_checki64(float2fix64(u32f.f, 1), INT64_MIN, "float2fix6412b");
    test_checki64(float2fix64(u32f.f, 2), INT64_MIN, "float2fix6412c");

#ifdef float2fix64
#error float2fix64 overridden, so original needs testing
#endif

    printf("float2ufix64\n");
    test_checku64(float2ufix64(3.5f, 8), 0x380, "float2ufix641");
    test_checku64(float2ufix64(-3.5f, 8), 0, "float2ufix642");
    test_checku64(float2ufix64(32768.0f, 16), 32768ull << 16, "float2ufix643");
    test_checku64(float2ufix64(65536.0f, 16), 65536ull << 16, "float2ufix644");
    test_checku64(float2ufix64(2147483648.0f, 16), 2147483648ull << 16, "float2ufix644b");
    test_checku64(float2ufix64(INFINITY, 16), UINT64_MAX, "float2ufix645");
    test_checku64(float2ufix64(-INFINITY, 16), 0, "float2ufix645b");
    test_checku64(float2ufix64(INFINITY, -16), UINT64_MAX, "float2ufix645c");
    test_checku64(float2ufix64(-INFINITY, -16), 0, "float2ufix645d");
    test_checku64(float2ufix64(INFINITY, 0), UINT64_MAX, "float2ufix645e");
    test_checku64(float2ufix64(-INFINITY, 0), 0, "float2ufix645f");
    test_checku64(float2ufix64(INFINITY, 16), UINT64_MAX, "float2ufix645");
    test_checku64(float2ufix64(-INFINITY, 16), 0, "float2ufix645b");
    test_checku64(float2ufix64(3.24999f, 2), 12, "float2ufix646");
    test_checku64(float2ufix64(3.25f, 2), 13, "float2ufix647");
    test_checku64(float2ufix64(3.0f, -1), 1, "float2ufix648"); // not very useful
    u32f.u = 0x7f012345;
    test_checku64(float2ufix64(u32f.f, 0), UINT64_MAX, "float2ufix649a");
    test_checku64(float2ufix64(u32f.f, 1), UINT64_MAX, "float2ufix649b");
    test_checku64(float2ufix64(u32f.f, 2), UINT64_MAX, "float2ufix649c");
    u32f.u = 0xff012345;
    test_checku64(float2ufix64(u32f.f, 0), 0, "float2ufix6410a");
    test_checku64(float2ufix64(u32f.f, 1), 0, "float2ufix6410b");
    test_checku64(float2ufix64(u32f.f, 2), 0, "float2ufix6410c");

#ifdef float2ufix64
#error float2ufix64 overridden, so original needs testing
#endif

    printf("float2fix_z\n");
    test_checki(float2fix_z(3.5f, 8), 0x380, "float2fix_z1");
    test_checki(float2fix_z(-3.5f, 8), -0x380, "float2fix_z2");
    test_checki(float2fix_z(32768.0f, 16), INT32_MAX, "float2fix_z3");
    test_checki(float2fix_z(65536.0f, 16), INT32_MAX, "float2fix_z4");
    test_checki(float2fix_z(INFINITY, 16), INT32_MAX, "float2fix_z5");
    test_checki(float2fix_z(-INFINITY, 16), INT32_MIN, "float2fix_z5b");
    test_checki(float2fix_z(INFINITY, -16), INT32_MAX, "float2fix_z5c");
    test_checki(float2fix_z(-INFINITY, -16), INT32_MIN, "float2fix_z5d");
    test_checki(float2fix_z(INFINITY, 0), INT32_MAX, "float2fix_z5e");
    test_checki(float2fix_z(-INFINITY, 0), INT32_MIN, "float2fix_z5f");
    test_checki(float2fix_z(3.24999f, 2), 12, "float2fix_z6");
    test_checki(float2fix_z(3.25f, 2), 13, "float2fix_z7");
    test_checki(float2fix_z(-3.24999f, 2), -12, "float2fix_z8");
    test_checki(float2fix_z(-3.25f, 2), -13, "float2fix_z9");
    test_checki(float2fix_z(-0.75f, 1), -1, "float2fix_z10");
    test_checki(float2fix_z(-3.0f, -1), -1, "float2fix_z11"); // not very useful
    u32f.u = 0x7f012345;
    test_checki(float2fix_z(u32f.f, 0), INT32_MAX, "float2fix_z12a");
    test_checki(float2fix_z(u32f.f, 1), INT32_MAX, "float2fix_z12b");
    test_checki(float2fix_z(u32f.f, 2), INT32_MAX, "float2fix_z12c");
    u32f.u = 0xff012345;
    test_checki(float2fix_z(u32f.f, 0), INT32_MIN, "float2fix_z13a");
    test_checki(float2fix_z(u32f.f, 1), INT32_MIN, "float2fix_z13b");
    test_checki(float2fix_z(u32f.f, 2), INT32_MIN, "float2fix_z13c");

#if LIB_PICO_FLOAT_PICO_VFP
    printf("call_float2fix_z\n");
    test_checki(call_float2fix_z(3.5f, 8), 0x380, "call_float2fix_z1");
    test_checki(call_float2fix_z(-3.5f, 8), -0x380, "call_float2fix_z2");
    test_checki(call_float2fix_z(32768.0f, 16), INT32_MAX, "call_float2fix_z3");
    test_checki(call_float2fix_z(65536.0f, 16), INT32_MAX, "call_float2fix_z4");
    test_checki(call_float2fix_z(INFINITY, 16), INT32_MAX, "call_float2fix_z5");
    test_checki(call_float2fix_z(-INFINITY, 16), INT32_MIN, "call_float2fix_z5b");
    test_checki(call_float2fix_z(INFINITY, -16), INT32_MAX, "call_float2fix_z5c");
    test_checki(call_float2fix_z(-INFINITY, -16), INT32_MIN, "call_float2fix_z5d");
    test_checki(call_float2fix_z(INFINITY, 0), INT32_MAX, "call_float2fix_z5e");
    test_checki(call_float2fix_z(-INFINITY, 0), INT32_MIN, "call_float2fix_z5f");
    test_checki(call_float2fix_z(3.24999f, 2), 12, "call_float2fix_z6");
    test_checki(call_float2fix_z(3.25f, 2), 13, "call_float2fix_z7");
    test_checki(call_float2fix_z(-3.24999f, 2), -12, "call_float2fix_z8");
    test_checki(call_float2fix_z(-3.25f, 2), -13, "call_float2fix_z9");
    test_checki(call_float2fix_z(-0.75f, 1), -1, "call_float2fix_z10");
    test_checki(call_float2fix_z(-3.0f, -1), -1, "call_float2fix_z11"); // not very useful
    u32f.u = 0x7f012345;
    test_checki(call_float2fix_z(u32f.f, 0), INT32_MAX, "call_float2fix_z12a");
    test_checki(call_float2fix_z(u32f.f, 1), INT32_MAX, "call_float2fix_z12b");
    test_checki(call_float2fix_z(u32f.f, 2), INT32_MAX, "call_float2fix_z12c");
    u32f.u = 0xff012345;
    test_checki(call_float2fix_z(u32f.f, 0), INT32_MIN, "call_float2fix_z13a");
    test_checki(call_float2fix_z(u32f.f, 1), INT32_MIN, "call_float2fix_z13b");
    test_checki(call_float2fix_z(u32f.f, 2), INT32_MIN, "call_float2fix_z13c");
#endif

    printf("float2ufix_z\n");
    test_checku(float2ufix_z(3.5f, 8), 0x380, "float2ufix_z1");
    test_checku(float2ufix_z(-3.5f, 8), 0, "float2ufix_z2");
    test_checku(float2ufix_z(32768.0f, 16), 32768 << 16, "float2ufix_z3");
    test_checku(float2ufix_z(65536.0f, 16), UINT32_MAX, "float2ufix_z4");
    test_checku(float2ufix_z(INFINITY, 16), UINT32_MAX, "float2ufix_z5");
    test_checku(float2ufix_z(-INFINITY, 16), 0, "float2ufix_z5b");
    test_checku(float2ufix_z(INFINITY, -16), UINT32_MAX, "float2ufix_z5c");
    test_checku(float2ufix_z(-INFINITY, -16), 0, "float2ufix_z5d");
    test_checku(float2ufix_z(INFINITY, 0), UINT32_MAX, "float2ufix_z5e");
    test_checku(float2ufix_z(-INFINITY, 0), 0, "float2ufix_z5f");
    test_checku(float2ufix_z(3.24999f, 2), 12, "float2ufix_z6");
    test_checku(float2ufix_z(3.25f, 2), 13, "float2ufix_z7");
    test_checku(float2ufix_z(3.0f, -1), 1, "float2ufix_z8"); // not very useful
    u32f.u = 0x7f012345;
    test_checku(float2ufix_z(u32f.f, 0), UINT32_MAX, "float2ufix_z9a");
    test_checku(float2ufix_z(u32f.f, 1), UINT32_MAX, "float2ufix_z9b");
    test_checku(float2ufix_z(u32f.f, 2), UINT32_MAX, "float2ufix_z9c");
    u32f.u = 0xff012345;
    test_checku(float2ufix_z(u32f.f, 0), 0, "float2ufix_z10a");
    test_checku(float2ufix_z(u32f.f, 1), 0, "float2ufix_z10b");
    test_checku(float2ufix_z(u32f.f, 2), 0, "float2ufix_z10c");

#if LIB_PICO_FLOAT_PICO_VFP
    printf("call_float2ufix_z\n");
    test_checku(call_float2ufix_z(3.5f, 8), 0x380, "call_float2ufix_z1");
    test_checku(call_float2ufix_z(-3.5f, 8), 0, "call_float2ufix_z2");
    test_checku(call_float2ufix_z(32768.0f, 16), 32768 << 16, "call_float2ufix_z3");
    test_checku(call_float2ufix_z(65536.0f, 16), UINT32_MAX, "call_float2ufix_z4");
    test_checku(call_float2ufix_z(INFINITY, 16), UINT32_MAX, "call_float2ufix_z5");
    test_checku(call_float2ufix_z(-INFINITY, 16), 0, "call_float2ufix_z5b");
    test_checku(call_float2ufix_z(INFINITY, -16), UINT32_MAX, "call_float2ufix_z5c");
    test_checku(call_float2ufix_z(-INFINITY, -16), 0, "call_float2ufix_z5d");
    test_checku(call_float2ufix_z(INFINITY, 0), UINT32_MAX, "call_float2ufix_z5e");
    test_checku(call_float2ufix_z(-INFINITY, 0), 0, "call_float2ufix_z5f");
    test_checku(call_float2ufix_z(3.24999f, 2), 12, "call_float2ufix_z6");
    test_checku(call_float2ufix_z(3.25f, 2), 13, "call_float2ufix_z7");
    test_checku(call_float2ufix_z(3.0f, -1), 1, "call_float2ufix_z8"); // not very useful
    u32f.u = 0x7f012345;
    test_checku(call_float2ufix_z(u32f.f, 0), UINT32_MAX, "call_float2ufix_z9a");
    test_checku(call_float2ufix_z(u32f.f, 1), UINT32_MAX, "call_float2ufix_z9b");
    test_checku(call_float2ufix_z(u32f.f, 2), UINT32_MAX, "call_float2ufix_z9c");
    u32f.u = 0xff012345;
    test_checku(call_float2ufix_z(u32f.f, 0), 0, "call_float2ufix_z10a");
    test_checku(call_float2ufix_z(u32f.f, 1), 0, "call_float2ufix_z10b");
    test_checku(call_float2ufix_z(u32f.f, 2), 0, "call_float2ufix_z10c");
#endif

    printf("float2fix64_z\n");
    test_checki64(float2fix64_z(3.5f, 8), 0x380, "float2fix64_z1");
    test_checki64(float2fix64_z(-3.5f, 8), -0x380, "float2fix64_z2");
    test_checki64(float2fix64_z(32768.0f, 16), 32768ll << 16, "float2fix64_z3");
    test_checki64(float2fix64_z(65536.0f, 16), 65536ll << 16, "float2fix64_z4");
    test_checki64(float2fix64_z(65536.0f * 65536.0f * 32768.0f, 16), INT64_MAX, "float2fix64_z4b");
    test_checki64(float2fix64_z(INFINITY, 16), INT64_MAX, "float2fix64_z5");
    test_checki64(float2fix64_z(-INFINITY, 16), INT64_MIN, "float2fix64_z5b");
    test_checki64(float2fix64_z(INFINITY, -16), INT64_MAX, "float2fix64_z5c");
    test_checki64(float2fix64_z(-INFINITY, -16), INT64_MIN, "float2fix64_z5d");
    test_checki64(float2fix64_z(INFINITY, 0), INT64_MAX, "float2fix64_z5e");
    test_checki64(float2fix64_z(-INFINITY, 0), INT64_MIN, "float2fix64_z5f");
    test_checki64(float2fix64_z(3.24999f, 2), 12, "float2fix64_z6");
    test_checki64(float2fix64_z(3.25f, 2), 13, "float2fix64_z7");
    test_checki64(float2fix64_z(-3.24999f, 2), -12, "float2fix64_z8");
    test_checki64(float2fix64_z(-3.25f, 2), -13, "float2fix64_z9");
    test_checki64(float2fix64_z(-3.0f, -1), -1, "float2fix64_z10"); // not very useful
    u32f.u = 0x7f012345;
    test_checki64(float2fix64_z(u32f.f, 0), INT64_MAX, "float2fix64_z11a");
    test_checki64(float2fix64_z(u32f.f, 1), INT64_MAX, "float2fix64_z11b");
    test_checki64(float2fix64_z(u32f.f, 2), INT64_MAX, "float2fix64_z11c");
    u32f.u = 0xff012345;
    test_checki64(float2fix64_z(u32f.f, 0), INT64_MIN, "float2fix64_z12a");
    test_checki64(float2fix64_z(u32f.f, 1), INT64_MIN, "float2fix64_z12b");
    test_checki64(float2fix64_z(u32f.f, 2), INT64_MIN, "float2fix64_z12c");
#ifdef float2fix64_z
#error float2fix64_z overridden, so original needs testing
#endif

    printf("float2ufix64_z\n");
    test_checku64(float2ufix64_z(3.5f, 8), 0x380, "float2ufix64_z1");
    test_checku64(float2ufix64_z(-3.5f, 8), 0, "float2ufix64_z2");
    test_checku64(float2ufix64_z(32768.0f, 16), 32768ll << 16, "float2ufix64_z3");
    test_checku64(float2ufix64_z(65536.0f, 16), 65536ll << 16, "float2ufix64_z4");
    test_checku64(float2ufix64_z(65536.0f * 65536.0f * 65536.0f, 16), UINT64_MAX, "float2ufix64_z4b");
    test_checku64(float2ufix64_z(INFINITY, 16), UINT64_MAX, "float2ufix64_z5");
    test_checku64(float2ufix64_z(-INFINITY, 16), 0, "float2ufix64_z5b");
    test_checku64(float2ufix64_z(INFINITY, -16), UINT64_MAX, "float2ufix64_z5c");
    test_checku64(float2ufix64_z(-INFINITY, -16), 0, "float2ufix64_z5d");
    test_checku64(float2ufix64_z(INFINITY, 0), UINT64_MAX, "float2ufix64_z5e");
    test_checku64(float2ufix64_z(-INFINITY, 0), 0, "float2ufix64_z5f");
    test_checku64(float2ufix64_z(3.24999f, 2), 12, "float2ufix64_z6");
    test_checku64(float2ufix64_z(3.25f, 2), 13, "float2ufix64_z7");
    test_checku64(float2ufix64_z(3.0f, -1), 1, "float2ufix64_z8"); // not very useful
    u32f.u = 0x7f012345;
    test_checku64(float2ufix64_z(u32f.f, 0), UINT64_MAX, "float2ufix64_z9a");
    test_checku64(float2ufix64_z(u32f.f, 1), UINT64_MAX, "float2ufix64_z9b");
    test_checku64(float2ufix64_z(u32f.f, 2), UINT64_MAX, "float2ufix64_z9c");
    u32f.u = 0xff012345;
    test_checku64(float2ufix64_z(u32f.f, 0), 0, "float2ufix64_z10a");
    test_checku64(float2ufix64_z(u32f.f, 1), 0, "float2ufix64_z10b");
    test_checku64(float2ufix64_z(u32f.f, 2), 0, "float2ufix64_z10c");
#ifdef float2ufix64_z
#error float2ufix64_z overridden, so original needs testing
#endif

    printf("float2int\n");
    test_checki(float2int(0.0f), 0, "float2int1");
    test_checki(float2int(0.25f), 0, "float2int1b");
    test_checki(float2int(0.5f), 0, "float2int2");
    test_checki(float2int(0.75f), 0, "float2int2b");
    test_checki(float2int(1.0f), 1, "float2int3");
    test_checki(float2int(-10.0f), -10, "float2int3a");
    test_checki(float2int(-0.0f), 0, "float2int3b");
    test_checki(float2int(-0.25f), -1, "float2int4");
    test_checki(float2int(-0.5f), -1, "float2int4b");
    test_checki(float2int(-0.75f), -1, "float2int5");
    test_checki(float2int(-1.0f), -1, "float2int5b");
    // todo test correct rounding around maximum precision
    test_checki(float2int(2147483647.0f), INT32_MAX, "float2int6");
    test_checki(float2int(21474836470.0f), INT32_MAX, "float2int7");
    test_checki(float2int(-2147483648.0f), INT32_MIN, "float2int8");
    test_checki(float2int(-21474836480.0f), INT32_MIN, "float2int9");
    test_checki(float2int(-2.5f), -3, "float2int10");
    test_checki(float2int(-2.4f), -3, "float2int11");
#ifdef float2int
#error float2int overridden, so original needs testing
#endif

    printf("float2uint\n");
    test_checku(float2uint(0.0f), 0, "float2uint1");
    test_checku(float2uint(0.25f), 0, "float2uint2");
    test_checku(float2uint(0.5f), 0, "float2uint3");
    test_checku(float2uint(0.75f), 0, "float2uint4");
    test_checku(float2uint(1.0f), 1, "float2uint5");
    test_checku(float2uint(2147483647.0f), INT32_MAX+1u, "float2uint6"); // note loss of precision
    test_checku(float2uint(2147483648.0f), INT32_MAX+1u, "float2uint7");
    test_checku(float2uint(4294967294.5f), UINT32_MAX, "float2uint8"); // note loss of precision
    test_checku(float2uint(4294967295.0f), UINT32_MAX, "float2uint9");
    test_checku(float2uint(42949672950.0f), UINT32_MAX, "float2uint10");
#ifdef float2uint
#error float2uint overridden, so original needs testing
#endif

    printf("float2int64\n");
    test_checki64(float2int64(0.0f), 0, "float2int641");
    test_checki64(float2int64(0.25f), 0, "float2int641b");
    test_checki64(float2int64(0.5f), 0, "float2int642");
    test_checki64(float2int64(0.75f), 0, "float2int642b");
    test_checki64(float2int64(1.0f), 1, "float2int643");
    test_checki64(float2int64(-10.0f), -10, "float2int643a");
    test_checki64(float2int64(-0.0f), 0, "float2int643b");
    test_checki64(float2int64(-0.25f), -1, "float2int644");
    test_checki64(float2int64(-0.5f), -1, "float2int644b");
    test_checki64(float2int64(-0.75f), -1, "float2int645");
    test_checki64(float2int64(-1.0f), -1, "float2int645b");
    // todo test correct rounding around maximum precision
    test_checki64(float2int64(2147483647.0f), INT32_MAX+1ll, "float2int646");
    test_checki64(float2int64(21474836470.0f), 21474836480ll, "float2int647"); // note loss of precision
    test_checki64(float2int64(-2147483648.0f), INT32_MIN, "float2int648");
    test_checki64(float2int64(-21474836480.0f), -21474836480ll, "float2int649");
    test_checki64(float2int64(-2.5f), -3, "float2int6410");
    test_checki64(float2int64(-2.4f), -3, "float2int6411");
#ifdef float2uint64
#error float2uint64 overridden, so original needs testing
#endif


    printf("float2uint64\n");
    test_checku64(float2uint64(0.0f), 0, "float2uint641");
    test_checku64(float2uint64(0.25f), 0, "float2uint642");
    test_checku64(float2uint64(0.5f), 0, "float2uint643");
    test_checku64(float2uint64(0.75f), 0, "float2uint644");
    test_checku64(float2uint64(1.0f), 1, "float2uint645");
    test_checku64(float2uint64(2147483647.0f), INT32_MAX+1u, "float2uint646"); // note loss of precision
    test_checku64(float2uint64(2147483648.0f), INT32_MAX+1u, "float2uint647");
    test_checku64(float2uint64(4294967294.5f), 4294967296ull, "float2uint648"); // note loss of precision
    test_checku64(float2uint64(4294967295.0f), 4294967296ull, "float2uint649"); // note loss of precision
    test_checku64(float2uint64(42949672950.0f), 42949672960ull, "float2uint6410"); // note loss of precision
#endif

    // // These methods round towards 0.

    printf("float2int_z\n");
    test_checki(float2int_z(0.0f), 0, "float2int_z1");
    test_checki(float2int_z(0.25f), 0, "float2int_z1b");
    test_checki(float2int_z(0.5f), 0, "float2int_z2");
    test_checki(float2int_z(0.75f), 0, "float2int_z2b");
    test_checki(float2int_z(1.0f), 1, "float2int_z3");
    test_checki(float2int_z(-10.0f), -10, "float2int_z3a");
    test_checki(float2int_z(-0.0f), 0, "float2int_z3b");
    test_checki(float2int_z(-0.25f), 0, "float2int_z4");
    test_checki(float2int_z(-0.5f), 0, "float2int_z4b");
    test_checki(float2int_z(-0.75f), 0, "float2int_z5");
    test_checki(float2int_z(-1.0f), -1, "float2int_z5b");
    // todo test correct rounding around maximum precision
    test_checki(float2int_z(2147483647.0f), INT32_MAX, "float2int_z6");
    test_checki(float2int_z(21474836470.0f), INT32_MAX, "float2int_z7");
    test_checki(float2int_z(-2147483648.0f), INT32_MIN, "float2int_z8");
    test_checki(float2int_z(-21474836480.0f), INT32_MIN, "float2int_z9");
    test_checki(float2int_z(-2.5f), -2, "float2int_z10");
    test_checki(float2int_z(-2.4f), -2, "float2int_z11");
    test_checki(float2int_z(make_positive_denormal_float()), 0, "float2int_z12");
    test_checki(float2int_z(make_negative_denormal_float()), 0, "float2int_z13");

#if LIB_PICO_FLOAT_PICO_VFP
    // the override is actually an inline func
// #ifndef float2int_z
// #error float2int_z not actually overridden
// #endif
    printf("call_float2int_z\n");
    test_checki(call_float2int_z(0.0f), 0, "call_float2int_z1");
    test_checki(call_float2int_z(0.25f), 0, "call_float2int_z1b");
    test_checki(call_float2int_z(0.5f), 0, "call_float2int_z2");
    test_checki(call_float2int_z(0.75f), 0, "call_float2int_z2b");
    test_checki(call_float2int_z(1.0f), 1, "call_float2int_z3");
    test_checki(call_float2int_z(-10.0f), -10, "call_float2int_z3a");
    test_checki(call_float2int_z(-0.0f), 0, "call_float2int_z3b");
    test_checki(call_float2int_z(-0.25f), 0, "call_float2int_z4");
    test_checki(call_float2int_z(-0.5f), 0, "call_float2int_z4b");
    test_checki(call_float2int_z(-0.75f), 0, "call_float2int_z5");
    test_checki(call_float2int_z(-1.0f), -1, "call_float2int_z5b");
    // todo test correct rounding around maximum precision
    test_checki(call_float2int_z(2147483647.0f), INT32_MAX, "call_float2int_z6");
    test_checki(call_float2int_z(21474836470.0f), INT32_MAX, "call_float2int_z7");
    test_checki(call_float2int_z(-2147483648.0f), INT32_MIN, "call_float2int_z8");
    test_checki(call_float2int_z(-21474836480.0f), INT32_MIN, "call_float2int_z9");
    test_checki(call_float2int_z(-2.5f), -2, "call_float2int_z10");
    test_checki(call_float2int_z(-2.4f), -2, "call_float2int_z11");
    test_checki(call_float2int_z(make_positive_denormal_float()), 0, "call_float2int_z12");
    test_checki(call_float2int_z(make_negative_denormal_float()), 0, "call_float2int_z13");
#endif

    printf("float2int64_z\n");
    test_checki64(float2int64_z(0.0f), 0, "float2int64_z1");
    test_checki64(float2int64_z(0.25f), 0, "float2int64_z1b");
    test_checki64(float2int64_z(0.5f), 0, "float2int64_z2");
    test_checki64(float2int64_z(0.75f), 0, "float2int64_z2b");
    test_checki64(float2int64_z(1.0f), 1, "float2int64_z3");
    test_checki64(float2int64_z(-10.0f), -10, "float2int64_z3a");
    test_checki64(float2int64_z(-0.0f), 0, "float2int64_z3b");
    test_checki64(float2int64_z(-0.25f), 0, "float2int64_z4");
    test_checki64(float2int64_z(-0.5f), 0, "float2int64_z4b");
    test_checki64(float2int64_z(-0.75f), 0, "float2int64_z5");
    test_checki64(float2int64_z(-1.0f), -1, "float2int64_z5b");
    test_checki64(float2int64_z(2147483647.0f), 2147483648ll, "float2int64_z6"); // note loss of precision
    test_checki64(float2int64_z(21474836470.0f), 21474836480ll, "float2int64_z7"); // note loss of precision
    test_checki64(float2int64_z(-2147483648.0f), INT32_MIN, "float2int64_z8");
    test_checki64(float2int64_z(-21474836480.0f), -21474836480ll, "float2int64_z9");
    test_checki64(float2int64_z(-2.5f), -2, "float2int64_z10");
    test_checki64(float2int64_z(-2.4f), -2, "float2int64_z11");
    test_checki64(float2int64_z(make_positive_denormal_float()), 0, "float2int64_z12");
    test_checki64(float2int64_z(make_negative_denormal_float()), 0, "float2int64_z13");

    printf("float2uint_z\n");
    test_checku(float2uint_z(0.0f), 0, "float2uint_z1");
    test_checku(float2uint_z(0.25f), 0, "float2uint_z2");
    test_checku(float2uint_z(0.5f), 0, "float2uint_z3");
    test_checku(float2uint_z(0.75f), 0, "float2uint_z4");
    test_checku(float2uint_z(1.0f), 1, "float2uint_z5");
    test_checku(float2uint_z(2147483647.0f), INT32_MAX+1u, "float2uint_z6"); // note loss of precision
    test_checku(float2uint_z(2147483648.0f), INT32_MAX+1u, "float2uint_z7");
    // todo test correct rounding around maximum precision
    test_checku(float2uint_z(4294967294.5f), UINT32_MAX, "float2uint_z8"); // note loss of precision
    test_checku(float2uint_z(4294967295.0f), UINT32_MAX, "float2uint_z9");
    test_checku(float2uint_z(42949672950.0f), UINT32_MAX, "float2uint_z10");
    test_checku(float2uint_z(make_positive_denormal_float()), 0, "float2uint_z11");
    test_checku(float2uint_z(make_negative_denormal_float()), 0, "float2uint_z12");

#if LIB_PICO_FLOAT_PICO_VFP
    // the override is actually an inline func
// #ifndef float2uint_z
// #error float2uint_z not actually overridden
// #endif
    printf("call_float2uint_z\n");
    test_checku(call_float2uint_z(0.0f), 0, "call_float2uint_z1");
    test_checku(call_float2uint_z(0.25f), 0, "call_float2uint_z2");
    test_checku(call_float2uint_z(0.5f), 0, "call_float2uint_z3");
    test_checku(call_float2uint_z(0.75f), 0, "call_float2uint_z4");
    test_checku(call_float2uint_z(1.0f), 1, "call_float2uint_z5");
    test_checku(call_float2uint_z(2147483647.0f), INT32_MAX+1u, "call_float2uint_z6"); // note loss of precision
    test_checku(call_float2uint_z(2147483648.0f), INT32_MAX+1u, "call_float2uint_z7");
    // todo test correct rounding around maximum precision
    test_checku(call_float2uint_z(4294967294.5f), UINT32_MAX, "call_float2uint_z8"); // note loss of precision
    test_checku(call_float2uint_z(4294967295.0f), UINT32_MAX, "call_float2uint_z9");
    test_checku(call_float2uint_z(42949672950.0f), UINT32_MAX, "call_float2uint_z10");
    test_checku(call_float2uint_z(make_positive_denormal_float()), 0, "call_float2uint_z11");
    test_checku(call_float2uint_z(make_negative_denormal_float()), 0, "call_float2uint_z12");
#endif

    printf("float2uint64_z\n");
    test_checku64(float2uint64_z(0.0f), 0, "float2uint64_z1");
    test_checku64(float2uint64_z(0.25f), 0, "float2uint64_z2");
    test_checku64(float2uint64_z(0.5f), 0, "float2uint64_z3");
    test_checku64(float2uint64_z(0.75f), 0, "float2uint64_z4");
    test_checku64(float2uint64_z(1.0f), 1, "float2uint64_z5");
    test_checku64(float2uint64_z(2147483647.0f), INT32_MAX+1u, "float2uint64_z6"); // note loss of precision
    test_checku64(float2uint64_z(2147483648.0f), INT32_MAX+1u, "float2uint64_z7");
    test_checku64(float2uint64_z(4294967294.5f), 4294967296ull, "float2uint64_z8"); // note loss of precision
    test_checku64(float2uint64_z(4294967295.0f), 4294967296ull, "float2uint64_z9"); // note loss of precision
    test_checku64(float2uint64_z(42949672950.0f), 42949672960ull, "float2uint64_z10"); // note loss of precision
    test_checku64(float2uint64_z(make_positive_denormal_float()), 0, "float2uint64_z11");
    test_checku64(float2uint64_z(make_negative_denormal_float()), 0, "float2uint64_z12");

    // float exp10f(float x);
    // void sincosf(float x, float *sinx, float *cosx);
    // float powintf(float x, int y);
    return rc;
}

int main() {
    stdio_init_all();
    int rc = test();
    if (rc) {
        printf("FAILED\n");
    } else {
        printf("PASSED\n");
    }
}
