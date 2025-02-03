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

#if 1 && (LIB_PICO_FLOAT_COMPILER || defined(__riscv))
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

#if 1 && LIB_PICO_FLOAT_VFP
// prevet the compiler from eliding the calculations
#undef float2int_z
#undef float2uint_z
#undef int2float
#undef uint2float
#endif

int test() {
    int rc = 0;
#if LIB_PICO_FLOAT_PICO_DCP
    printf(">>> Using DCP\n");
#endif
#if LIB_PICO_FLOAT_PICO_VFP
    printf(">>> Using VFP\n");
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

    printf("uint2float\n");
    test_checkf(uint2float(0), 0.0f, "uint2float1");
    test_checkf(uint2float(1), 1.0f, "uint2float2");
    test_checkf(uint2float(INT32_MAX), 2147483647.0f, "uint2float3");
    // todo test correct rounding around maximum precision
    test_checkf(uint2float(UINT32_MAX), 4294967295.0f, "uint2float4");

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

    printf("ufix2float\n");
    // todo test correct rounding around maximum precision
    test_checkf(ufix2float(0xa0000000, 30), 2.5f, "ufix2float1");
    test_checkf(ufix2float(3, -4), 48.0f, "ufix2float2");

    printf("fix642float\n");
    // todo test correct rounding around maximum precision
    test_checkf(fix642float(-0xa000000000ll, 38), -2.5f, "fix6422float1");
    test_checkf(fix642float(-3, -34), -51539607552.0f, "fix642float2");

    printf("ufix642float\n");
    // todo test correct rounding around maximum precision
    test_checkf(ufix642float(0xa000000000ll, 38), 2.5f, "ufix642float1");
    test_checkf(ufix642float(3, -34), 51539607552.0f, "fix64float2");

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
    test_checki(float2fix(3.24999f, 2), 12, "float2fix6");
    test_checki(float2fix(3.25f, 2), 13, "float2fix7");
    test_checki(float2fix(-3.24999f, 2), -13, "float2fix8");
    test_checki(float2fix(-3.25f, 2), -13, "float2fix9");
    test_checki(float2fix(-0.75f, 1), -2, "float2fix10");
    test_checki(float2fix(-3.0f, -1), -2, "float2fix11"); // not very useful
    u32f.u = 0x7f012345;
    test_checki(float2fix(u32f.f, 1), INT32_MAX, "float2fix12");
    u32f.u = 0xff012345;
    test_checki(float2fix(u32f.f, 1), INT32_MIN, "float2fix13");

    printf("float2ufix\n");
    test_checku(float2ufix(3.5f, 8), 0x380, "float2ufix1");
    test_checku(float2ufix(-3.5f, 8), 0, "float2ufix2");
    test_checku(float2ufix(32768.0f, 16), 32768 << 16, "float2ufix3");
    test_checku(float2ufix(65536.0f, 16), UINT32_MAX, "float2ufix4");
    test_checku(float2ufix(INFINITY, 16), UINT32_MAX, "float2ufix5");
    test_checku(float2ufix(3.24999f, 2), 12, "float2ufix6");
    test_checku(float2ufix(3.25f, 2), 13, "float2ufix7");
    test_checku(float2ufix(3.0f, -1), 1, "float2ufix8"); // not very useful

    printf("float2fix64\n");
    test_checki64(float2fix64(3.5f, 8), 0x380, "float2fix641");
    test_checki64(float2fix64(-3.5f, 8), -0x380, "float2fix642");
    test_checki64(float2fix64(32768.0f, 16), 32768ll << 16, "float2fix643");
    test_checki64(float2fix64(65536.0f, 16), 65536ll << 16, "float2fix644");
    test_checki64(float2fix64(2147483648.0f, 16), 2147483648ll << 16, "float2ufix644b");
    test_checki64(float2fix64(65536.0f * 65536.0f * 32768.0f, 16), INT64_MAX, "float2fix644c");
    test_checki64(float2fix64(INFINITY, 16), INT64_MAX, "float2fix645");
    test_checki64(float2fix64(3.24999f, 2), 12, "float2fix646");
    test_checki64(float2fix64(3.25f, 2), 13, "float2fix647");
    test_checki64(float2fix64(-3.24999f, 2), -13, "float2fix648");
    test_checki64(float2fix64(-3.25f, 2), -13, "float2fix649");
    test_checki64(float2fix64(-3.0f, -1), -2, "float2fix6410"); // not very useful

    printf("float2ufix64\n");
    test_checku64(float2ufix64(3.5f, 8), 0x380, "float2ufix641");
    test_checku64(float2ufix64(-3.5f, 8), 0, "float2ufix642");
    test_checku64(float2ufix64(32768.0f, 16), 32768ull << 16, "float2ufix643");
    test_checku64(float2ufix64(65536.0f, 16), 65536ull << 16, "float2ufix644");
    test_checku64(float2ufix64(2147483648.0f, 16), 2147483648ull << 16, "float2ufix644b");
    test_checku64(float2ufix64(INFINITY, 16), UINT64_MAX, "float2ufix645");
    test_checku64(float2ufix64(3.24999f, 2), 12, "float2ufix646");
    test_checku64(float2ufix64(3.25f, 2), 13, "float2ufix647");
    test_checku64(float2ufix64(3.0f, -1), 1, "float2ufix648"); // not very useful

    printf("float2fix_z\n");
    test_checki(float2fix_z(3.5f, 8), 0x380, "float2fix_z1");
    test_checki(float2fix_z(-3.5f, 8), -0x380, "float2fix_z2");
    test_checki(float2fix_z(32768.0f, 16), INT32_MAX, "float2fix_z3");
    test_checki(float2fix_z(65536.0f, 16), INT32_MAX, "float2fix_z4");
    test_checki(float2fix_z(INFINITY, 16), INT32_MAX, "float2fix_z5");
    test_checki(float2fix_z(-INFINITY, 16), INT32_MIN, "float2fix_z5b");
    test_checki(float2fix_z(3.24999f, 2), 12, "float2fix_z6");
    test_checki(float2fix_z(3.25f, 2), 13, "float2fix_z7");
    test_checki(float2fix_z(-3.24999f, 2), -12, "float2fix_z8");
    test_checki(float2fix_z(-3.25f, 2), -13, "float2fix_z9");
    test_checki(float2fix_z(-0.75f, 1), -1, "float2fix_z10");
    test_checki(float2fix_z(-3.0f, -1), -1, "float2fix_z11"); // not very useful
    u32f.u = 0x7f012345;
    test_checki(float2fix_z(u32f.f, 1), INT32_MAX, "float2fix_z12");
    u32f.u = 0xff012345;
    test_checki(float2fix_z(u32f.f, 1), INT32_MIN, "float2fix_z13");

    printf("float2ufix_z\n");
    test_checku(float2ufix_z(3.5f, 8), 0x380, "float2ufix_z1");
    test_checku(float2ufix_z(-3.5f, 8), 0, "float2ufix_z2");
    test_checku(float2ufix_z(32768.0f, 16), 32768 << 16, "float2ufix_z3");
    test_checku(float2ufix_z(65536.0f, 16), UINT32_MAX, "float2ufix_z4");
    test_checku(float2ufix_z(INFINITY, 16), UINT32_MAX, "float2ufix_z5");
    test_checku(float2ufix_z(3.24999f, 2), 12, "float2ufix_z6");
    test_checku(float2ufix_z(3.25f, 2), 13, "float2ufix_z7");
    test_checku(float2ufix_z(3.0f, -1), 1, "float2ufix_z8"); // not very useful
    u32f.u = 0x7f012345;
    test_checku(float2ufix_z(u32f.f, 1), UINT32_MAX, "float2fix_z9");
    u32f.u = 0xff012345;
    test_checku(float2ufix_z(u32f.f, 1), 0, "float2fix_z10");

    printf("float2fix64_z\n");
    test_checki64(float2fix64_z(3.5f, 8), 0x380, "float2fix64_z1");
    test_checki64(float2fix64_z(-3.5f, 8), -0x380, "float2fix64_z2");
    test_checki64(float2fix64_z(32768.0f, 16), 32768ll << 16, "float2fix64_z3");
    test_checki64(float2fix64_z(65536.0f, 16), 65536ll << 16, "float2fix64_z4");
    test_checki64(float2fix64_z(65536.0f * 65536.0f * 32768.0f, 16), INT64_MAX, "float2fix64_z4b");
    test_checki64(float2fix64_z(INFINITY, 16), INT64_MAX, "float2fix64_z5");
    test_checki64(float2fix64_z(3.24999f, 2), 12, "float2fix64_z6");
    test_checki64(float2fix64_z(3.25f, 2), 13, "float2fix64_z7");
    test_checki64(float2fix64_z(-3.24999f, 2), -12, "float2fix64_z8");
    test_checki64(float2fix64_z(-3.25f, 2), -13, "float2fix64_z9");
    test_checki64(float2fix64_z(-3.0f, -1), -1, "float2fix64_z10"); // not very useful

    printf("float2ufix64_z\n");
    test_checku64(float2ufix64_z(3.5f, 8), 0x380, "float2ufix64_z1");
    test_checku64(float2ufix64_z(-3.5f, 8), 0, "float2ufix64_z2");
    test_checku64(float2ufix64_z(32768.0f, 16), 32768ll << 16, "float2ufix64_z3");
    test_checku64(float2ufix64_z(65536.0f, 16), 65536ll << 16, "float2ufix64_z4");
    test_checki64(float2ufix64_z(65536.0f * 65536.0f * 65536.0f, 16), UINT64_MAX, "float2fix64_z4b");
    test_checku64(float2ufix64_z(INFINITY, 16), UINT64_MAX, "float2ufix64_z5");
    test_checku64(float2ufix64_z(3.24999f, 2), 12, "float2ufix64_z6");
    test_checku64(float2ufix64_z(3.25f, 2), 13, "float2ufix64_z7");
    test_checki64(float2ufix64_z(3.0f, -1), 1, "float2fuix64_z8"); // not very useful

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
