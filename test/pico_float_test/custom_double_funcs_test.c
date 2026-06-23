#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/double.h"
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
#define test_checkd(x, expected, msg) ({ if ((x) != (expected)) { printf("  %s: %f != %f\n", msg, x, expected); stop(); } })
#define test_checki(x, expected, msg) ({ if ((x) != (expected)) { printf("  %s: %d != %d\n", msg, x, expected); stop(); } })
#define test_checku(x, expected, msg) ({ if ((uint32_t)(x) != (uint32_t)(expected)) { printf("  %s: %u != %u\n", msg, x, expected); stop(); } })
#define test_checki64(x, expected, msg) ({ if ((x) != (expected)) { printf("  %s: %lld != %lld\n", msg, (int64_t)(x), (int64_t)(expected)); stop(); } })
#define test_checku64(x, expected, msg) ({ if ((uint64_t)(x) != (uint64_t)(expected)) { printf("  %s: %llu != %llu\n", msg, (uint64_t)(x), (uint64_t)(expected)); stop(); } })

// we only want these when we provided macros
#if PICO_DOUBLE_HAS_FIX32_TO_DOUBLE_CONVERSIONS
static inline double fix2double_8(int32_t m) { return fix2double(m, 8); }
static inline double fix2double_16(int32_t m) { return fix2double(m, 16); }
static inline double fix2double_24(int32_t m) { return fix2double(m, 24); }
static inline double fix2double_32(int32_t m) { return fix2double(m, 32); }

static inline double ufix2double_8(uint32_t m) { return ufix2double(m, 8); }
static inline double ufix2double_16(uint32_t m) { return ufix2double(m, 16); }
static inline double ufix2double_24(uint32_t m) { return ufix2double(m, 24); }
static inline double ufix2double_32(uint32_t m) { return ufix2double(m, 32); }

static inline int32_t double2fix_12(double d) { return double2fix(d, 12); }
static inline uint32_t double2ufix_12(double d) { return double2ufix(d, 12); }
#endif

#if LIB_PICO_DOUBLE_COMPILER || defined(__riscv)
#define double2int_z(d) ({ double _d = d; pico_default_asm_volatile("" : "+r" (_d)); double2 ## int_z(_d); })
#define double2uint_z(d) ({ double _d = d; pico_default_asm_volatile("" : "+r" (_d)); double2 ## uint_z(_d); })
#define double2int64_z(d) ({ double _d = d; pico_default_asm_volatile("" : "+r" (_d)); double2 ## int64_z(_d); })
#define double2uint64_z(d) ({ double _d = d; pico_default_asm_volatile("" : "+r" (_d)); double2 ## uint64_z(_d); })
#define int2double(i) ({ int32_t _i = i; pico_default_asm_volatile("" : "+r" (_i)); int2 ## double(_i); })
#define uint2double(u) ({ uint32_t _u = u; pico_default_asm_volatile("" : "+r" (_u)); uint2 ## double(_u); })
#define int642double(i) ({ int64_t _i = i; pico_default_asm_volatile("" : "+r" (_i)); int642 ## double(_i); })
#define uint642double(u) ({ uint64_t _u = u; pico_default_asm_volatile("" : "+r" (_u)); uint642 ## double(_u); })
#endif

double make_positive_denormal_double(void) {
    union {
        double d;
        uint64_t u;
    } x;
    x.u = 0x0000000000000001ull;
    return x.d;
}

double make_negative_denormal_double(void) {
    union {
        double d;
        uint64_t u;
    } x;
    x.u = 0x8000000000000001ull;
    return x.d;
}


int test() {
    int rc = 0;
    printf("=== custom_double_funcs_test ");
#if PICO_C_COMPILER_IS_CLANG
    printf("(LLVM)");
#elif PICO_C_COMPILER_IS_GNU
    printf("(GCC)");
#endif
#if defined(__ARM_PCS_VFP)
    printf(" (hard)");
#endif
    printf("\n");
#if LIB_PICO_DOUBLE_PICO_DCP
    printf("--- using DCP\n");
#elif LIB_PICO_FLOAT_COMPILER
    printf("--- using compiler\n");
#endif

#if PICO_DOUBLE_HAS_INT32_TO_DOUBLE_CONVERSIONS
    printf("int2double\n");
    test_checkd(int2double(0), 0.0, "int2double1");
    test_checkd(int2double(-1), -1.0, "int2double2");
    test_checkd(int2double(1), 1.0, "int2double3");
    test_checkd(int2double(INT32_MAX), 2147483647.0, "int2double4");
    test_checkd(int2double(INT32_MIN), -2147483648.0, "int2double5");
    // these have rounding behavior on float but not double
    test_checkd(int2double(2147483391), 2147483391.0, "int2double6");
    test_checkd(int2double(2147483391), 2147483391.0, "int2double7");
    test_checkd(int2double(2147483457), 2147483457.0, "int2double8");
    test_checkd(int2double(2147483483), 2147483483.0, "int2double9");
    test_checkd(int2double(2147483584), 2147483584.0, "int2double10");

    printf("uint2double\n");
    test_checkd(uint2double(0), 0.0, "uint2double1");
    test_checkd(uint2double(1), 1.0, "uint2double2");
    test_checkd(uint2double(INT32_MAX), 2147483647.0, "uint2double3");
    // todo test correct rounding around maximum precision
    test_checkd(uint2double(UINT32_MAX), 4294967295.0, "uint2double4");
#endif

#if PICO_DOUBLE_HAS_INT64_TO_DOUBLE_CONVERSIONS
    printf("int642double\n");
    test_checkd(int642double(0), 0.0, "int642double1");
    test_checkd(int642double(-1), -1.0, "int642double2");
    test_checkd(int642double(1), 1.0, "int642double3");
    test_checkd(int642double(INT32_MAX-1), 2147483646.0, "int642double4");
    test_checkd(int642double(INT32_MAX), 2147483647.0, "int642double5");
    test_checkd(int642double(INT32_MAX+1ll), 2147483648.0, "int642double6");
    test_checkd(int642double(INT32_MIN-1ll), -2147483649.0, "int642double7");
    test_checkd(int642double(INT32_MIN), -2147483648.0, "int642double8");
    test_checkd(int642double(INT32_MIN+1ll), -2147483647.0, "int642double9");
    // todo test correct rounding around maximum precision
    test_checkd(int642double(INT64_MAX), 9223372036854775807.0, "int642double10");
    test_checkd(int642double(INT64_MIN), -9223372036854775808.0, "int642double11");

    printf("uint642double\n");
    test_checkd(uint642double(0), 0.0, "uint642double1");
    test_checkd(uint642double(1), 1.0, "uint642double2");
    test_checkd(uint642double(INT32_MAX-1), 2147483646.0, "uint642double3");
    test_checkd(uint642double(INT32_MAX), 2147483647.0, "uint642double4");
    test_checkd(uint642double(INT32_MAX+1ll), 2147483648.0, "uint642double5");
    test_checkd(uint642double(INT64_MAX), 9223372036854775807.0, "uint642double6");
    // todo test correct rounding around maximum precision
    test_checkd(uint642double(UINT64_MAX), 18446744073709551615.0, "uint642double7");
#endif

    union {
        uint64_t u;
        double d;
    } u64d;

#if PICO_DOUBLE_HAS_FIX32_TO_DOUBLE_CONVERSIONS
    printf("fix2double\n");
    // todo test correct rounding around maximum precision
    test_checkd(fix2double(-3, 1), -1.5, "fix2double1");
    test_checkd(fix2double(-3, 1), -1.5, "fix2double2");
    test_checkd(fix2double(-3, -4), -48.0, "fix2double3");

    printf("ufix2double\n");
    // todo test correct rounding around maximum precision
    test_checkd(ufix2double(0xa0000000, 30), 2.5, "ufix2double1");
    test_checkd(ufix2double(3, -4), 48.0, "ufix2double2");

    printf("fix2double_N\n");
    test_checkd(fix2double_8(128), 0.5, "fix2double_8_1");
    test_checkd(fix2double_8(-128), -0.5, "fix2double_8_2");
    test_checkd(fix2double_16(8192), 0.125, "fix2double_16_1");
    test_checkd(fix2double_16(-8192), -0.125, "fix2double_16_2");
    test_checkd(fix2double_24(3<<23), 1.5, "fix2double_24_1");
    test_checkd(fix2double_24(-(3<<23)), -1.5, "fix2double_24_2");

    printf("ufix2double_N\n");
    test_checkd(ufix2double_8(128), 0.5, "ufix2double_8_1");
    test_checkd(ufix2double_8(-128), 16777215.5, "ufix2double_8_2");
    test_checkd(ufix2double_16(8192), 0.125, "ufix2double_16_1");
    test_checkd(ufix2double_16(-8192), 65535.875, "ufix2double_16_2");
    test_checkd(ufix2double_24(3<<23), 1.5, "ufix2double_24_1");
    test_checkd(ufix2double_24(-(3<<23)), 254.5, "ufix2double_24_2");
#endif

#if PICO_DOUBLE_HAS_FIX64_TO_DOUBLE_CONVERSIONS
    printf("fix642double\n");
    // todo test correct rounding around maximum precision
    test_checkd(fix642double(-0xa000000000ll, 38), -2.5, "fix642double1");
    test_checkd(fix642double(-3, -34), -51539607552.0, "fix642double2");

    printf("ufix642double\n");
    // todo test correct rounding around maximum precision
    test_checkd(ufix642double(0xa000000000ll, 38), 2.5, "ufix642double1");
    test_checkd(ufix642double(3, -34), 51539607552.0, "ufix642double2");
#endif

#if PICO_DOUBLE_HAS_DOUBLE_TO_FIX32_M_CONVERSIONS
    printf("double2fix\n");
    test_checki(double2fix(-0.5, 8), -0x80, "double2fix0");
    test_checki(double2fix(3.5, 8), 0x380, "double2fix1");
    test_checki(double2fix(-3.5, 8), -0x380, "double2fix2");
    test_checki(double2fix(32768.0, 16), INT32_MAX, "double2fix3");
    test_checki(double2fix(65536.0, 16), INT32_MAX, "double2fix4");
    test_checki(double2fix(-65536.0, 16), INT32_MIN, "double2fix4b");
    test_checki(double2fix(INFINITY, 16), INT32_MAX, "double2fix5");
    test_checki(double2fix(-INFINITY, 16), INT32_MIN, "double2fix5b");
    test_checki(double2fix(INFINITY, -16), INT32_MAX, "double2fix5c");
    test_checki(double2fix(-INFINITY, -16), INT32_MIN, "double2fix5d");
    test_checki(double2fix(INFINITY, 0), INT32_MAX, "double2fix5e");
    test_checki(double2fix(-INFINITY, 0), INT32_MIN, "double2fix5f");
    test_checki(double2fix(3.24999, 2), 12, "double2fix6");
    test_checki(double2fix(3.25, 2), 13, "double2fix7");
    test_checki(double2fix(-3.24999, 2), -13, "double2fix8");
    test_checki(double2fix(-3.25, 2), -13, "double2fix9");
    test_checki(double2fix(-0.75, 1), -2, "double2fix10");
    test_checki(double2fix(-3.0, -1), -2, "double2fix11"); // not very useful
    test_checki(double2fix(0.0, 16), 0, "double2fix12");
    test_checki(double2fix(-0.0, 16), 0, "double2fix13");
    test_checki(double2fix(0.0, -16), 0, "double2fix14");
    test_checki(double2fix(-0.0, -16), 0, "double2fix15");
    u64d.u = 0x7fe0000000012345ull;
    test_checki(double2fix(u64d.d, 0), INT32_MAX, "double2fix16a");
    test_checki(double2fix(u64d.d, 1), INT32_MAX, "double2fix16b");
    test_checki(double2fix(u64d.d, 2), INT32_MAX, "double2fix16c");
    u64d.u = 0xffe0000000012345ull;
    test_checki(double2fix(u64d.d, 0), INT32_MIN, "double2fix17a");
    test_checki(double2fix(u64d.d, 1), INT32_MIN, "double2fix17b");
    test_checki(double2fix(u64d.d, 2), INT32_MIN, "double2fix17c");


    printf("double2ufix\n");
    test_checku(double2ufix(3.5, 8), 0x380, "double2ufix1");
    test_checku(double2ufix(-3.5, 8), 0, "double2ufix2");
    test_checku(double2ufix(32768.0, 16), 32768 << 16, "double2ufix3");
    test_checku(double2ufix(65536.0, 16), UINT32_MAX, "double2ufix4");
    test_checku(double2ufix(INFINITY, 16), UINT32_MAX, "double2ufix5");
    test_checku(double2ufix(-INFINITY, 16), 0, "double2ufix5b");
    test_checku(double2ufix(INFINITY, -16), UINT32_MAX, "double2ufix5c");
    test_checku(double2ufix(-INFINITY, -16), 0, "double2ufix5d");
    test_checku(double2ufix(INFINITY, 0), UINT32_MAX, "double2ufix5e");
    test_checku(double2ufix(-INFINITY, 0), 0, "double2ufix5f");
    test_checku(double2ufix(3.24999, 2), 12, "double2ufix6");
    test_checku(double2ufix(3.25, 2), 13, "double2ufix7");
    test_checku(double2ufix(3.0, -1), 1, "double2ufix8"); // not very useful
    test_checku(double2ufix(0.0, 16), 0, "double2ufix9");
    test_checku(double2ufix(-0.0, 16), 0, "double2ufix10");
    test_checku(double2ufix(0.0, -16), 0, "double2ufix11");
    test_checku(double2ufix(-0.0, -16), 0, "double2ufix12");
    u64d.u = 0x7fe0000000012345ull;
    test_checku(double2ufix(u64d.d, 0), UINT32_MAX, "double2ufix13a");
    test_checku(double2ufix(u64d.d, 1), UINT32_MAX, "double2ufix13b");
    test_checku(double2ufix(u64d.d, 2), UINT32_MAX, "double2ufix13c");
    u64d.u = 0xffe0000000012345ull;
    test_checku(double2ufix(u64d.d, 0), 0, "double2ufix14a");
    test_checku(double2ufix(u64d.d, 1), 0, "double2ufix14b");
    test_checku(double2ufix(u64d.d, 2), 0, "double2ufix14c");
#endif

#if PICO_DOUBLE_HAS_DOUBLE_TO_FIX64_M_CONVERSIONS
    printf("double2fix64\n");
    test_checki64(double2fix64(3.5, 8), 0x380, "double2fix64_1");
    test_checki64(double2fix64(-3.5, 8), -0x380, "double2fix64_2");
    test_checki64(double2fix64(32768.0, 16), 32768ll << 16, "double2fix64_3");
    test_checki64(double2fix64(65536.0, 16), 65536ll << 16, "double2fix64_4");
    test_checki64(double2fix64(2147483648.0, 16), 2147483648ll << 16, "double2fix64_4b");
    test_checki64(double2fix64(65536.0 * 65536.0 * 32768.0, 16), INT64_MAX, "double2fix64_4c");
    test_checki64(double2fix64(INFINITY, 16), INT64_MAX, "double2fix64_5");
    test_checki64(double2fix64(-INFINITY, 16), INT64_MIN, "double2fix64_5b");
    test_checki64(double2fix64(INFINITY, -16), INT64_MAX, "double2fix64_5c");
    test_checki64(double2fix64(-INFINITY, -16), INT64_MIN, "double2fix64_5d");
    test_checki64(double2fix64(INFINITY, 0), INT64_MAX, "double2fix64_5e");
    test_checki64(double2fix64(-INFINITY, 0), INT64_MIN, "double2fix64_5f");
    test_checki64(double2fix64(3.24999, 2), 12, "double2fix64_6");
    test_checki64(double2fix64(3.25, 2), 13, "double2fix64_7");
    test_checki64(double2fix64(-3.24999, 2), -13, "double2fix64_8");
    test_checki64(double2fix64(-3.25, 2), -13, "double2fix64_9");
    test_checki64(double2fix64(-3.0, -1), -2, "double2fix64_10"); // not very useful
    test_checki64(double2fix64(2147483648.0 * 2147483648.0, 16), INT64_MAX, "double2fix64_11");
    test_checki64(double2fix64(0.0, 16), 0, "double2fix64_12");
    test_checki64(double2fix64(0.0, -16), 0, "double2fix64_12b");
    test_checki64(double2fix64(-0.0, 16), 0, "double2fix64_13");
    test_checki64(double2fix64(-0.0, -16), 0, "double2fix64_13b");
    test_checki64(double2fix64(-3.25, 40), -13ll * (1ll << 38), "double2fix64_14");
    u64d.u = 0xc00a000000000001;
    test_checki64(double2fix64(u64d.d, 40), -13ll * (1ll << 38) - 1ll, "double2fix64_14b");

    u64d.u = 0xc00a000080000001;
    test_checki64(double2fix64(u64d.d, 20), -13ll * (1ll << 18) - 2ll, "double2fix64_14c");
    u64d.u = 0xc00a000080000000;
    test_checki64(double2fix64(u64d.d, 20), -13ll * (1ll << 18) - 1ll, "double2fix64_14d");
    u64d.u = 0xc00a000000000001;
    test_checki64(double2fix64(u64d.d, 20), -13ll * (1ll << 18) - 1ll, "double2fix64_14e");
    u64d.u = 0xc00a000000000000;
    test_checki64(double2fix64(u64d.d, 20), -13ll * (1ll << 18), "double2fix64_14f");

    u64d.u = 0xc00a000080000001;
    test_checki64(double2fix64(u64d.d, 19), -13ll * (1ll << 17) - 1ll, "double2fix64_14g");
    u64d.u = 0xc00a000080000000;
    test_checki64(double2fix64(u64d.d, 19), -13ll * (1ll << 17) - 1ll, "double2fix64_14h");
    u64d.u = 0xc00a000000000001;
    test_checki64(double2fix64(u64d.d, 19), -13ll * (1ll << 17) - 1ll, "double2fix64_14i");
    u64d.u = 0xc00a000000000000;
    test_checki64(double2fix64(u64d.d, 19), -13ll * (1ll << 17), "double2fix64_14j");
    u64d.u = 0x7fe0000000012345ull;
    test_checki64(double2fix64(u64d.d, 0), INT64_MAX, "double2fix64_15a");
    test_checki64(double2fix64(u64d.d, 1), INT64_MAX, "double2fix64_15b");
    test_checki64(double2fix64(u64d.d, 2), INT64_MAX, "double2fix64_15c");
    u64d.u = 0xffe0000000012345ull;
    test_checki64(double2fix64(u64d.d, 0), INT64_MIN, "double2fix64_16a");
    test_checki64(double2fix64(u64d.d, 1), INT64_MIN, "double2fix64_16b");
    test_checki64(double2fix64(u64d.d, 2), INT64_MIN, "double2fix64_16c");

    printf("double2ufix64\n");
    test_checku64(double2ufix64(3.5, 8), 0x380, "double2ufix64_1");
    test_checku64(double2ufix64(-3.5, 8), 0, "double2ufix64_2");
    test_checku64(double2ufix64(32768.0, 16), 32768ull << 16, "double2ufix64_3");
    test_checku64(double2ufix64(65536.0, 16), 65536ull << 16, "double2ufix64_4");
    test_checku64(double2ufix64(2147483648.0, 16), 2147483648ull << 16, "double2ufix64_4b");
    test_checku64(double2ufix64(INFINITY, 16), UINT64_MAX, "double2ufix64_5");
    test_checku64(double2ufix64(-INFINITY, 16), 0, "double2ufix64_5b");
    test_checku64(double2ufix64(INFINITY, -16), UINT64_MAX, "double2ufix64_5c");
    test_checku64(double2ufix64(-INFINITY, -16), 0, "double2ufix64_5d");
    test_checku64(double2ufix64(INFINITY, 0), UINT64_MAX, "double2ufix64_5e");
    test_checku64(double2ufix64(-INFINITY, 0), 0, "double2ufix64_5f");
    test_checku64(double2ufix64(3.24999, 2), 12, "double2ufix64_6");
    test_checku64(double2ufix64(3.25, 2), 13, "double2ufix64_7");
    test_checku64(double2ufix64(3.0, -1), 1, "double2ufix64_8"); // not very useful
    test_checku64(double2ufix64(0.0, 16), 0, "double2ufix64_9");
    test_checku64(double2ufix64(-0.0, 16), 0, "double2ufix64_10");
    u64d.u = 0x7fe0000000012345ull;
    test_checku64(double2ufix64(u64d.d, 0), UINT64_MAX, "double2ufix64_11a");
    test_checku64(double2ufix64(u64d.d, 1), UINT64_MAX, "double2ufix64_11b");
    test_checku64(double2ufix64(u64d.d, 2), UINT64_MAX, "double2ufix64_11c");
    u64d.u = 0xffe0000000012345ull;
    test_checku64(double2ufix64(u64d.d, 0), 0, "double2ufix64_12a");
    test_checku64(double2ufix64(u64d.d, 1), 0, "double2ufix64_12b");
    test_checku64(double2ufix64(u64d.d, 2), 0, "double2ufix64_12c");
#endif

#if PICO_DOUBLE_HAS_DOUBLE_TO_FIX32_Z_CONVERSIONS
    printf("double2fix_z\n");
    test_checki(double2fix_z(3.5, 8), 0x380, "double2fix_z1");
    test_checki(double2fix_z(-3.5, 8), -0x380, "double2fix_z2");
    test_checki(double2fix_z(32768.0, 16), INT32_MAX, "double2fix_z3");
    test_checki(double2fix_z(65536.0, 16), INT32_MAX, "double2fix_z4");
    test_checki(double2fix_z(INFINITY, 16), INT32_MAX, "double2fix_z5");
    test_checki(double2fix_z(-INFINITY, 16), INT32_MIN, "double2fix_z5b");
    test_checki(double2fix_z(INFINITY, -50), INT32_MAX, "double2fix_z5c");
    test_checki(double2fix_z(-INFINITY, -50), INT32_MIN, "double2fix_z5d");
    test_checki(double2fix_z(INFINITY, 0), INT32_MAX, "double2fix_z5e");
    test_checki(double2fix_z(-INFINITY, 0), INT32_MIN, "double2fix_z5f");
    test_checki(double2fix_z(3.24999, 2), 12, "double2fix_z6");
    test_checki(double2fix_z(3.25, 2), 13, "double2fix_z7");
    test_checki(double2fix_z(-3.24999, 2), -12, "double2fix_z8");
    test_checki(double2fix_z(-3.25, 2), -13, "double2fix_z9");
    test_checki(double2fix_z(-0.75, 1), -1, "double2fix_z10");
    test_checki(double2fix_z(-3.0, -1), -1, "double2fix_z11"); // not very useful
    test_checki(double2fix_z(0.0, 16), 0, "double2fix_z12");
    test_checki(double2fix_z(0.0, -16), 0, "double2fix_z12b");
    test_checki(double2fix_z(-0.0, 16), 0, "double2fix_z13");
    test_checki(double2fix_z(-0.0, -16), 0, "double2fix_z13b");
    u64d.u = 0x7fe0000000012345ull;
    test_checki(double2fix_z(u64d.d, 0), INT32_MAX, "double2fix_z14a");
    test_checki(double2fix_z(u64d.d, 1), INT32_MAX, "double2fix_z14b");
    test_checki(double2fix_z(u64d.d, 2), INT32_MAX, "double2fix_z14c");
    u64d.u = 0xffe0000000012345ull;
    test_checki(double2fix_z(u64d.d, 0), INT32_MIN, "double2fix_z15a");
    test_checki(double2fix_z(u64d.d, 1), INT32_MIN, "double2fix_z15b");
    test_checki(double2fix_z(u64d.d, 2), INT32_MIN, "double2fix_z15c");

    printf("double2ufix_z\n");
    test_checku(double2ufix_z(3.5, 8), 0x380, "double2ufix_z1");
    test_checku(double2ufix_z(-3.5, 8), 0, "double2ufix_z2");
    test_checku(double2ufix_z(32768.0, 16), 32768 << 16, "double2ufix_z3");
    test_checku(double2ufix_z(65536.0, 16), UINT32_MAX, "double2ufix_z4");
    test_checku(double2ufix_z(INFINITY, 16), UINT32_MAX, "double2ufix_z5");
    test_checku(double2ufix_z(-INFINITY, 16), 0, "double2ufix_z5b");
    test_checku(double2ufix_z(INFINITY, -16), UINT32_MAX, "double2ufix_z5c");
    test_checku(double2ufix_z(-INFINITY, -16), 0, "double2ufix_z5d");
    test_checku(double2ufix_z(INFINITY, 0), UINT32_MAX, "double2ufix_z5e");
    test_checku(double2ufix_z(-INFINITY, 0), 0, "double2ufix_z5f");
    test_checku(double2ufix_z(3.24999, 2), 12, "double2ufix_z6");
    test_checku(double2ufix_z(3.25, 2), 13, "double2ufix_z7");
    test_checku(double2ufix_z(3.0, -1), 1, "double2ufix_z8"); // not very useful
    test_checku(double2ufix_z(0.0, 16), 0, "double2ufix_z9");
    test_checku(double2ufix_z(-0.0, 16), 0, "double2ufix_z10");
    test_checku(double2ufix_z(0.0, -16), 0, "double2ufix_z11");
    test_checku(double2ufix_z(-0.0, -16), 0, "double2ufix_z12");
    u64d.u = 0x7fe0000000012345ull;
    test_checku(double2ufix_z(u64d.d, 0), UINT32_MAX, "double2ufix_z13a");
    test_checku(double2ufix_z(u64d.d, 1), UINT32_MAX, "double2ufix_z13b");
    test_checku(double2ufix_z(u64d.d, 2), UINT32_MAX, "double2ufix_z13c");
    u64d.u = 0xffe0000000012345ull;
    test_checku(double2ufix_z(u64d.d, 0), 0, "double2ufix_z14a");
    test_checku(double2ufix_z(u64d.d, 1), 0, "double2ufix_z14b");
    test_checku(double2ufix_z(u64d.d, 2), 0, "double2ufix_z14c");
#endif

#if PICO_DOUBLE_HAS_DOUBLE_TO_FIX64_Z_CONVERSIONS
    printf("double2fix64_z\n");
    test_checki64(double2fix64_z(3.5, 8), 0x380, "double2fix64_z1");
    test_checki64(double2fix64_z(-3.5, 8), -0x380, "double2fix64_z2");
    test_checki64(double2fix64_z(32768.0, 16), 32768ll << 16, "double2fix64_z3");
    test_checki64(double2fix64_z(65536.0, 16), 65536ll << 16, "double2fix64_z4");
    test_checki64(double2fix64_z(65536.0 * 65536.0 * 32768.0, 16), INT64_MAX, "double2fix64_z4b");
    test_checki64(double2fix64_z(INFINITY, 16), INT64_MAX, "double2fix64_z5");
    test_checki64(double2fix64_z(-INFINITY, 16), INT64_MIN, "double2fix64_z5b");
    test_checki64(double2fix64_z(INFINITY, -16), INT64_MAX, "double2fix64_z5c");
    test_checki64(double2fix64_z(-INFINITY, -16), INT64_MIN, "double2fix64_z5d");
    test_checki64(double2fix64_z(INFINITY, 0), INT64_MAX, "double2fix64_z5e");
    test_checki64(double2fix64_z(-INFINITY, 0), INT64_MIN, "double2fix64_z5f");
    test_checki64(double2fix64_z(3.24999, 2), 12, "double2fix64_z6");
    test_checki64(double2fix64_z(3.25, 2), 13, "double2fix64_z7");
    test_checki64(double2fix64_z(-3.24999, 2), -12, "double2fix64_z8");
    test_checki64(double2fix64_z(-3.25, 2), -13, "double2fix64_z9");
    test_checki64(double2fix64_z(-3.0, -1), -1, "double2fix64_z10"); // not very useful
    test_checki64(double2fix64_z(0.0, 16), 0, "double2fix64_z11");
    test_checki64(double2fix64_z(-0.0, 16), 0, "double2fix64_z12");
    test_checki64(double2fix64_z(0.0, -16), 0, "double2fix64_z13");
    test_checki64(double2fix64_z(-0.0, -16), 0, "double2fix64_z14");
    test_checki64(double2fix64_z(-3.25, 40), -13ll * (1ll << 38), "double2fix64_z15");
    u64d.u = 0xc00a000000000001;
    test_checki64(double2fix64_z(u64d.d, 40), -13ll * (1ll << 38), "double2fix64_z15b");

    u64d.u = 0xc00a000080000001;
    test_checki64(double2fix64_z(u64d.d, 20), -13ll * (1ll << 18) - 1ll, "double2fix64_z15c");
    u64d.u = 0xc00a000080000000;
    test_checki64(double2fix64_z(u64d.d, 20), -13ll * (1ll << 18) - 1ll, "double2fix64_z15d");
    u64d.u = 0xc00a000000000001;
    test_checki64(double2fix64_z(u64d.d, 20), -13ll * (1ll << 18), "double2fix64_z15e");
    u64d.u = 0xc00a000000000000;
    test_checki64(double2fix64_z(u64d.d, 20), -13ll * (1ll << 18), "double2fix64_z15f");

    u64d.u = 0xc00a000080000001;
    test_checki64(double2fix64_z(u64d.d, 19), -13ll * (1ll << 17), "double2fix64_z15g");
    u64d.u = 0xc00a000080000000;
    test_checki64(double2fix64_z(u64d.d, 19), -13ll * (1ll << 17), "double2fix64_z15h");
    u64d.u = 0xc00a000000000001;
    test_checki64(double2fix64_z(u64d.d, 19), -13ll * (1ll << 17), "double2fix64_z15i");
    u64d.u = 0xc00a000000000000;
    test_checki64(double2fix64_z(u64d.d, 19), -13ll * (1ll << 17), "double2fix64_z15j");
    u64d.u = 0x7fe0000000012345ull;
    test_checki64(double2fix64_z(u64d.d, 0), INT64_MAX, "double2fix64_z16a");
    test_checki64(double2fix64_z(u64d.d, 1), INT64_MAX, "double2fix64_z16b");
    test_checki64(double2fix64_z(u64d.d, 2), INT64_MAX, "double2fix64_z16c");
    u64d.u = 0xffe0000000012345ull;
    test_checki64(double2fix64_z(u64d.d, 0), INT64_MIN, "double2fix64_z17a");
    test_checki64(double2fix64_z(u64d.d, 1), INT64_MIN, "double2fix64_z17b");
    test_checki64(double2fix64_z(u64d.d, 2), INT64_MIN, "double2fix64_z17c");

    printf("double2ufix64_z\n");
    test_checku64(double2ufix64_z(3.5, 8), 0x380, "double2ufix64_z1");
    test_checku64(double2ufix64_z(-3.5, 8), 0, "double2ufix64_z2");
    test_checku64(double2ufix64_z(32768.0, 16), 32768ll << 16, "double2ufix64_z3");
    test_checku64(double2ufix64_z(65536.0, 16), 65536ll << 16, "double2ufix64_z4");
    test_checku64(double2ufix64_z(65536.0 * 65536.0 * 65536.0, 16), UINT64_MAX, "double2ufix64_z4b");
    test_checku64(double2ufix64_z(INFINITY, 16), UINT64_MAX, "double2ufix64_z5");
    test_checku64(double2ufix64_z(-INFINITY, 16), 0, "double2ufix64_z5b");
    test_checku64(double2ufix64_z(INFINITY, -16), UINT64_MAX, "double2ufix64_z5c");
    test_checku64(double2ufix64_z(-INFINITY, -16), 0, "double2ufix64_z5d");
    test_checku64(double2ufix64_z(INFINITY, 0), UINT64_MAX, "double2ufix64_z5e");
    test_checku64(double2ufix64_z(-INFINITY, 0), 0, "double2ufix64_z5f");
    test_checku64(double2ufix64_z(3.24999, 2), 12, "double2ufix64_z6");
    test_checku64(double2ufix64_z(3.25, 2), 13, "double2ufix64_z7");
    test_checku64(double2ufix64_z(3.0, -1), 1, "double2ufix64_z8"); // not very useful
    test_checku64(double2ufix64_z(0.0, 16), 0, "double2ufix64_z9");
    test_checku64(double2ufix64_z(-0.0, 16), 0, "double2ufix64_z10");
    test_checku64(double2ufix64_z(0.0, -16), 0, "double2ufix64_z11");
    test_checku64(double2ufix64_z(-0.0, -16), 0, "double2ufix64_z12");
    u64d.u = 0x7fe0000000012345ull;
    test_checku64(double2ufix64_z(u64d.d, 0), UINT64_MAX, "double2ufix64_z13a");
    test_checku64(double2ufix64_z(u64d.d, 1), UINT64_MAX, "double2ufix64_z13b");
    test_checku64(double2ufix64_z(u64d.d, 2), UINT64_MAX, "double2ufix64_z13c");
    u64d.u = 0xffe0000000012345ull;
    test_checku64(double2ufix64_z(u64d.d, 0), 0, "double2ufix64_z14a");
    test_checku64(double2ufix64_z(u64d.d, 1), 0, "double2ufix64_z14b");
    test_checku64(double2ufix64_z(u64d.d, 2), 0, "double2ufix64_z14c");
#endif

#if PICO_DOUBLE_HAS_DOUBLE_TO_INT32_M_CONVERSIONS
    printf("double2int\n");
    test_checki(double2int(0.0), 0, "double2int1");
    test_checki(double2int(0.25), 0, "double2int1b");
    test_checki(double2int(0.5), 0, "double2int2");
    test_checki(double2int(0.75), 0, "double2int2b");
    test_checki(double2int(1.0), 1, "double2int3");
    test_checki(double2int(-10.0), -10, "double2int3b");
    test_checki(double2int(-0.0), 0, "double2int3c");
    test_checki(double2int(-0.25), -1, "double2int4");
    test_checki(double2int(-0.5), -1, "double2int4b");
    test_checki(double2int(-0.75), -1, "double2int5");
    test_checki(double2int(-1.0), -1, "double2int5b");
    // todo test correct rounding around maximum precision
    test_checki(double2int(2147483646.0), INT32_MAX-1, "double2int6");
    test_checki(double2int(2147483647.0), INT32_MAX, "double2int6b");
    test_checki(double2int(21474836470.0), INT32_MAX, "double2int7");
    test_checki(double2int(-2147483648.0), INT32_MIN, "double2int8");
    test_checki(double2int(-21474836480.0), INT32_MIN, "double2int9");
    test_checki(double2int(-2.5), -3, "double2int10");
    test_checki(double2int(-2.4), -3, "double2int11");
    u64d.u = 0xc000000000000000ull;
    test_checki(double2int(u64d.d), -2, "double2int12");
    u64d.u = 0xc008000000000000ull;
    test_checki(double2int(u64d.d), -3, "double2int12b");
    u64d.u = 0xc000000000000001ull;
    test_checki(double2int(u64d.d), -3, "double2int12c");
    u64d.u = 0xc000000080000000ull;
    test_checki(double2int(u64d.d), -3, "double2int12d");
    u64d.u = 0xc000000100000000ull;
    test_checki(double2int(u64d.d), -3, "double2int12e");
    u64d.u = 0xc000000100000001ull;
    test_checki(double2int(u64d.d), -3, "double2int12f");
    test_checki(double2int(-2147483647.0), INT32_MIN+1, "double2int13");
    test_checki(double2int(-2147483647.1), INT32_MIN, "double2int14");
    test_checki(double2int(-2147483647.9), INT32_MIN, "double2int15");
    test_checki(double2int(-2147483648.0), INT32_MIN, "double2int16");
    test_checki(double2int(-2147483648.1), INT32_MIN, "double2int17");
    test_checki(double2int(-21474836480.1), INT32_MIN, "double2int18");
    test_checki(double2int(make_positive_denormal_double()), 0, "double2int19");
    int double2int20 = double2int(make_negative_denormal_double());
    if (double2int20 == -1) double2int20 = 0; // -1 is a valid answer depending on flush to zero
    test_checki(double2int20, 0, "double2int20");

    printf("double2uint\n");
    test_checku(double2uint(0.0), 0, "double2uint1");
    test_checku(double2uint(0.25), 0, "double2uint2");
    test_checku(double2uint(0.5), 0, "double2uint3");
    test_checku(double2uint(0.75), 0, "double2uint4");
    test_checku(double2uint(1.0), 1, "double2uint5");
    test_checku(double2uint(2147483647.0), INT32_MAX, "double2uint6");
    test_checku(double2uint(2147483648.0), INT32_MAX+1u, "double2uint7");
    test_checku(double2uint(4294967294.5), UINT32_MAX-1, "double2uint8");
    test_checku(double2uint(4294967295.0), UINT32_MAX, "double2uint9");
    test_checku(double2uint(42949672950.0), UINT32_MAX, "double2uint10");
#endif

#if PICO_DOUBLE_HAS_DOUBLE_TO_INT64_M_CONVERSIONS
    printf("double2int64\n");
    test_checki64(double2int64(0.0), 0, "double2int64_1");
    test_checki64(double2int64(0.25), 0, "double2int64_1b");
    test_checki64(double2int64(0.5), 0, "double2int64_2");
    test_checki64(double2int64(0.75), 0, "double2int64_2b");
    test_checki64(double2int64(1.0), 1, "double2int64_3");
    test_checki64(double2int64(-10.0), -10, "double2int64_3b");
    test_checki64(double2int64(-0.0), 0, "double2int64_3c");
    test_checki64(double2int64(-0.25), -1, "double2int64_4");
    test_checki64(double2int64(-0.5), -1, "double2int64_4b");
    test_checki64(double2int64(-0.75), -1, "double2int64_5");
    test_checki64(double2int64(-1.0), -1, "double2int64_5b");
    // todo test correct rounding around maximum precision
    test_checki64(double2int64(2147483647.0), INT32_MAX, "double2int64_6");
    test_checki64(double2int64(21474836470.0), 21474836470ll, "double2int64_7");
    test_checki64(double2int64(-2147483648.0), INT32_MIN, "double2int64_8");
    test_checki64(double2int64(-21474836480.0), -21474836480ll, "double2int64_9");
    test_checki64(double2int64(-2.5), -3, "double2int64_10");
    test_checki64(double2int64(-2.4), -3, "double2int64_11");
    u64d.u = 0xc000000000000000ull;
    test_checki64(double2int64(u64d.d), -2, "double2int64_12");
    u64d.u = 0xc008000000000000ull;
    test_checki64(double2int64(u64d.d), -3, "double2int64_12b");
    u64d.u = 0xc000000000000001ull;
    test_checki64(double2int64(u64d.d), -3, "double2int64_12c");
    u64d.u = 0xc000000080000000ull;
    test_checki64(double2int64(u64d.d), -3, "double2int64_12d");
    u64d.u = 0xc000000100000000ull;
    test_checki64(double2int64(u64d.d), -3, "double2int64_12e");
    u64d.u = 0xc000000100000001ull;
    test_checki64(double2int64(u64d.d), -3, "double2int64_12f");
    test_checki64(double2int64(make_positive_denormal_double()), 0, "double2int64_13");
    test_checki64(double2int64(make_negative_denormal_double()), 0, "double2int64_14");

    printf("double2uint64\n");
    test_checku64(double2uint64(0.0), 0, "double2uint64_1");
    test_checku64(double2uint64(0.25), 0, "double2uint64_2");
    test_checku64(double2uint64(0.5), 0, "double2uint64_3");
    test_checku64(double2uint64(0.75), 0, "double2uint64_4");
    test_checku64(double2uint64(1.0), 1, "double2uint64_5");
    test_checku64(double2uint64(2147483647.0), INT32_MAX, "double2uint64_6");
    test_checku64(double2uint64(2147483648.0), INT32_MAX+1u, "double2uint64_7");
    // todo test correct rounding around maximum precision
    test_checku64(double2uint64(4294967294.5), 4294967294ull, "double2uint64_8");
    test_checku64(double2uint64(4294967295.0), 4294967295ull, "double2uint64_9");
    test_checku64(double2uint64(42949672950.0), 42949672950, "double2uint64_10");
    test_checku64(double2uint64(make_positive_denormal_double()), 0, "double2uint64_11");
    test_checku64(double2uint64(make_negative_denormal_double()), 0, "double2uint64_12");
#endif

#if PICO_DOUBLE_HAS_DOUBLE_TO_INT32_Z_CONVERSIONS
    // // These methods round towards 0.
    printf("double2int_z\n");
    test_checki(double2int_z(0.0), 0, "double2int_z1");
    test_checki(double2int_z(0.25), 0, "double2int_z1b");
    test_checki(double2int_z(0.5), 0, "double2int_z2");
    test_checki(double2int_z(0.75), 0, "double2int_z2b");
    test_checki(double2int_z(1.0), 1, "double2int_z3");
    test_checki(double2int_z(-10.0), -10, "double2int_z3b");
    test_checki(double2int_z(-0.0), 0, "double2int_z3c");
    test_checki(double2int_z(-0.25), 0, "double2int_z4");
    test_checki(double2int_z(-0.5), 0, "double2int_z4b");
    test_checki(double2int_z(-0.75), 0, "double2int_z5");
    test_checki(double2int_z(-1.0), -1, "double2int_z5b");
    // todo test correct rounding around maximum precision
    test_checki(double2int_z(2147483647.0), INT32_MAX, "double2int_z6");
    test_checki(double2int_z(21474836470.0), INT32_MAX, "double2int_z7");
    test_checki(double2int_z(-2147483648.0), INT32_MIN, "double2int_z8");
    test_checki(double2int_z(-21474836480.0), INT32_MIN, "double2int_z9");
    test_checki(double2int_z(-2.5), -2, "double2int_z10");
    test_checki(double2int_z(-2.4), -2, "double2int_z11");
    u64d.u = 0xc000000000000000ull;
    test_checki(double2int_z(u64d.d), -2, "double2int_z12");
    u64d.u = 0xc008000000000000ull;
    test_checki(double2int_z(u64d.d), -3, "double2int_z12b");
    u64d.u = 0xc000000000000001ull;
    test_checki(double2int_z(u64d.d), -2, "double2int_z12c");
    u64d.u = 0xc000000080000000ull;
    test_checki(double2int_z(u64d.d), -2, "double2int_z12d");
    u64d.u = 0xc000000100000000ull;
    test_checki(double2int_z(u64d.d), -2, "double2int_z12e");
    u64d.u = 0xc000000100000001ull;
    test_checki(double2int_z(u64d.d), -2, "double2int_z12f");
    test_checki(double2int_z(make_positive_denormal_double()), 0, "double2int_z13");
    test_checki(double2int_z(make_negative_denormal_double()), 0, "double2int_z14");

    printf("double2uint_z\n");
    test_checku(double2uint_z(0.0), 0, "double2uint_z1");
    test_checku(double2uint_z(0.25), 0, "double2uint_z2");
    test_checku(double2uint_z(0.5), 0, "double2uint_z3");
    test_checku(double2uint_z(0.75), 0, "double2uint_z4");
    test_checku(double2uint_z(1.0), 1, "double2uint_z5");
    test_checku(double2uint_z(2147483647.0), INT32_MAX, "double2uint_z6");
    test_checku(double2uint_z(2147483648.0), INT32_MAX+1u, "double2uint_z7");
    // todo test correct rounding around maximum precision
    test_checku(double2uint_z(4294967294.5), UINT32_MAX-1u, "double2uint_z8");
    test_checku(double2uint_z(4294967295.0), UINT32_MAX, "double2uint_z9");
    test_checku(double2uint_z(42949672950.0), UINT32_MAX, "double2uint_z10");
    test_checku(double2uint_z(make_positive_denormal_double()), 0, "double2uint_z11");
    test_checku(double2uint_z(make_negative_denormal_double()), 0, "double2uint_z12");
#endif

#if PICO_DOUBLE_HAS_DOUBLE_TO_INT64_Z_CONVERSIONS
    printf("double2int64_z\n");
    test_checki64(double2int64_z(0.0), 0, "double2int64_z1");
    test_checki64(double2int64_z(0.25), 0, "double2int64_z1b");
    test_checki64(double2int64_z(0.5), 0, "double2int64_z2");
    test_checki64(double2int64_z(0.75), 0, "double2int64_z2b");
    test_checki64(double2int64_z(1.0), 1, "double2int64_z3");
    test_checki64(double2int64_z(-10.0), -10, "double2int64_z3b");
    test_checki64(double2int64_z(-0.0), 0, "double2int64_z3c");
    test_checki64(double2int64_z(-0.25), 0, "double2int64_z4");
    test_checki64(double2int64_z(-0.5), 0, "double2int64_z4b");
    test_checki64(double2int64_z(-0.75), 0, "double2int64_z5");
    test_checki64(double2int64_z(-1.0), -1, "double2int64_z5b");
    // todo test correct rounding around maximum precision
    test_checki64(double2int64_z(2147483647.0), 2147483647ll, "double2int64_z6");
    test_checki64(double2int64_z(21474836470.0), 21474836470ll, "double2int64_z7");
    test_checki64(double2int64_z(-2147483648.0), INT32_MIN, "double2int64_z8");
    test_checki64(double2int64_z(-21474836480.0), -21474836480ll, "double2int64_z9");
    test_checki64(double2int64_z(-2.5), -2, "double2int64_z10");
    test_checki64(double2int64_z(-2.4), -2, "double2int64_z11");
    test_checki64(double2int64_z(make_positive_denormal_double()), 0, "double2int64_z12");
    test_checki64(double2int64_z(make_negative_denormal_double()), 0, "double2int64_z13");

    printf("double2uint64_z\n");
    test_checku64(double2uint64_z(0.0), 0, "double2uint64_z1");
    test_checku64(double2uint64_z(0.25), 0, "double2uint64_z2");
    test_checku64(double2uint64_z(0.5), 0, "double2uint64_z3");
    test_checku64(double2uint64_z(0.75), 0, "double2uint64_z4");
    test_checku64(double2uint64_z(1.0), 1, "double2uint64_z5");
    test_checku64(double2uint64_z(2147483647.0), INT32_MAX, "double2uint64_z6");
    test_checku64(double2uint64_z(2147483648.0), INT32_MAX+1u, "double2uint64_z7");
    // todo test correct rounding around maximum precision
    test_checku64(double2uint64_z(4294967294.5), 4294967294ull, "double2uint64_z8");
    test_checku64(double2uint64_z(4294967295.0), 4294967295ull, "double2uint64_z9");
    test_checku64(double2uint64_z(4294967296.0), 4294967296ull, "double2uint64_z9b");
    test_checku64(double2uint64_z(42949672950.0), 42949672950ull, "double2uint64_z10");
    test_checku64(double2uint64_z(make_positive_denormal_double()), 0, "double2uint64_z11");
    test_checku64(double2uint64_z(make_negative_denormal_double()), 0, "double2uint64_z12");
#endif

    // double exp10(double x);
    // void sincos(double x, double *sinx, double *cosx);
    // double powint(double x, int y);
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
