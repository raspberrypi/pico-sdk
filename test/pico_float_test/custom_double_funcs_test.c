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

#if !(LIB_PICO_DOUBLE_COMPILER || defined(__riscv))
static inline double fix2double_8(int32_t m) { return fix2double(m, 8); }
static inline double fix2double_12(int32_t m) { return fix2double(m, 12); }
static inline double fix2double_16(int32_t m) { return fix2double(m, 16); }
static inline double fix2double_24(int32_t m) { return fix2double(m, 24); }
static inline double fix2double_28(int32_t m) { return fix2double(m, 28); }
static inline double fix2double_32(int32_t m) { return fix2double(m, 32); }

static inline double ufix2double_12(int32_t m) { return ufix2double(m, 12); }

static inline double double2fix_12(int32_t m) { return double2fix(m, 12); }

static inline double double2ufix_12(int32_t m) { return double2ufix(m, 12); }
#endif

#if 1 && (LIB_PICO_DOUBLE_COMPILER || defined(__riscv))
#define double2int_z(f) ({ double _d = f; pico_default_asm_volatile("" : "+r" (_d)); double2 ## int_z(_d); })
#define double2uint_z(f) ({ double _d = f; pico_default_asm_volatile("" : "+r" (_d)); double2 ## uint_z(_d); })
#define double2int64_z(f) ({ double _d = f; pico_default_asm_volatile("" : "+r" (_d)); double2 ## int64_z(_d); })
#define double2uint64_z(f) ({ double _d = f; pico_default_asm_volatile("" : "+r" (_d)); double2 ## uint64_z(_d); })
#define int2double(i) ({ int32_t _i = i; pico_default_asm_volatile("" : "+r" (_i)); int2 ## double(_i); })
#define uint2double(i) ({ uint32_t _i = i; pico_default_asm_volatile("" : "+r" (_i)); uint2 ## double(_i); })
#define int642double(i) ({ int64_t _i = i; pico_default_asm_volatile("" : "+r" (_i)); int642 ## double(_i); })
#define uint642double(i) ({ uint64_t _i = i; pico_default_asm_volatile("" : "+r" (_i)); uint642 ## double(_i); })
#endif

int test() {
    int rc = 0;
#if LIB_PICO_DOUBLE_PICO
    printf(">>> Using PICO\n");
#endif
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
    test_checkd(int642double(INT64_MIN), -9223372036854775808.0, "int642doubl11e");

    printf("uint642double\n");
    test_checkd(uint642double(0), 0.0, "uint642double1");
    test_checkd(uint642double(1), 1.0, "uint642double2");
    test_checkd(uint642double(INT32_MAX-1), 2147483646.0, "uint642double3");
    test_checkd(uint642double(INT32_MAX), 2147483647.0, "uint642double4");
    test_checkd(uint642double(INT32_MAX+1ll), 2147483648.0, "uint642double5");
    test_checkd(uint642double(INT64_MAX), 9223372036854775807.0, "uint642double6");
    // todo test correct rounding around maximum precision
    test_checkd(uint642double(UINT64_MAX), 18446744073709551615.0, "uint642double7");

    union {
        uint64_t u;
        double d;
    } u64d;

#if !(LIB_PICO_DOUBLE_COMPILER || defined(__riscv))
    printf("fix2double\n");
    // todo test correct rounding around maximum precision
    test_checkd(fix2double(-3, 1), -1.5, "fix2double1");
    test_checkd(fix2double(-3, 1), -1.5, "fix2double2");
    test_checkd(fix2double(-3, -4), -48.0, "fix2double3");

    printf("ufix2double\n");
    // todo test correct rounding around maximum precision
    test_checkd(ufix2double(0xa0000000, 30), 2.5, "ufix2double1");
    test_checkd(ufix2double(3, -4), 48.0, "ufix2double2");

    printf("fix64double\n");
    // todo test correct rounding around maximum precision
    test_checkd(fix642double(-0xa000000000ll, 38), -2.5, "fix642double1");
    test_checkd(fix642double(-3, -34), -51539607552.0, "fix642double2");

    printf("ufix642double\n");
    // todo test correct rounding around maximum precision
    test_checkd(ufix642double(0xa000000000ll, 38), 2.5, "ufix642double1");
    test_checkd(ufix642double(3, -34), 51539607552.0, "fix64double2");

    test_checkd(fix2double_8(128), 0.5, "fix2double_8_1");
    test_checkd(fix2double_8(-128), -0.5, "fix2double_8_2");
    test_checkd(fix2double_16(8192), 0.125, "fix2double_8_3");
    test_checkd(fix2double_16(-8192), -0.125, "fix2double_8_4");
    test_checkd(fix2double_24(3<<23), 1.5, "fix2double_8_5");
    test_checkd(fix2double_24(-(3<<23)), -1.5, "fix2double_8_6");

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

    printf("double2ufix\n");
    test_checku(double2ufix(3.5, 8), 0x380, "double2ufix1");
    test_checku(double2ufix(-3.5, 8), 0, "double2ufix2");
    test_checku(double2ufix(32768.0, 16), 32768 << 16, "double2ufix3");
    test_checku(double2ufix(65536.0, 16), UINT32_MAX, "double2ufix4");
    test_checku(double2ufix(INFINITY, 16), UINT32_MAX, "double2ufix5");
    test_checku(double2ufix(-INFINITY, 16), 0, "double2ufix5b");
    test_checku(double2ufix(INFINITY, -16), UINT32_MAX, "double2ufix5c");
    test_checku(double2ufix(-INFINITY, -16), 0, "double2ufix5d");
    test_checku(double2ufix(3.24999, 2), 12, "double2ufix6");
    test_checku(double2ufix(3.25, 2), 13, "double2ufix7");
    test_checku(double2ufix(3.0, -1), 1, "double2ufix8"); // not very useful
    test_checki(double2ufix(0.0, 16), 0, "double2ufix12");
    test_checki(double2ufix(-0.0, 16), 0, "double2fix13");
    test_checki(double2ufix(0.0, -16), 0, "double2ufix14");
    test_checki(double2ufix(-0.0, -16), 0, "double2fix15");

    printf("double2fix64\n");
    test_checki64(double2fix64(3.5, 8), 0x380, "double2fix641");
    test_checki64(double2fix64(-3.5, 8), -0x380, "double2fix642");
    test_checki64(double2fix64(32768.0, 16), 32768ll << 16, "double2fix643");
    test_checki64(double2fix64(65536.0, 16), 65536ll << 16, "double2fix644");
    test_checki64(double2fix64(2147483648.0, 16), 2147483648ll << 16, "double2ufix644b");
    test_checki64(double2fix64(65536.0 * 65536.0 * 32768.0, 16), INT64_MAX, "double2fix644c");
    test_checki64(double2fix64(INFINITY, 16), INT64_MAX, "double2fix645");
    test_checki64(double2fix64(-INFINITY, 16), INT64_MIN, "double2fix645b");
    test_checki64(double2fix64(INFINITY, -16), INT64_MAX, "double2fix645c");
    test_checki64(double2fix64(-INFINITY, -16), INT64_MIN, "double2fix645d");
    test_checki64(double2fix64(3.24999, 2), 12, "double2fix646");
    test_checki64(double2fix64(3.25, 2), 13, "double2fix647");
    test_checki64(double2fix64(-3.24999, 2), -13, "double2fix648");
    test_checki64(double2fix64(-3.25, 2), -13, "double2fix649");
    test_checki64(double2fix64(-3.0, -1), -2, "double2fix6410"); // not very useful
    test_checki64(double2fix64(2147483648.0 * 2147483648.0, 16), INT64_MAX, "double2ufix6411");
    test_checki64(double2fix64(0.0, 16), 0, "double2fix6412");
    test_checki64(double2fix64(-0.0, 16), 0, "double2fix6413");
    test_checki64(double2fix64(0.0, -16), 0, "double2fix6412b");
    test_checki64(double2fix64(-0.0, -16), 0, "double2fix6413b");
    test_checki64(double2fix64(-3.25, 40), -13ll * (1ll << 38), "double2fix6414");
    u64d.u = 0xc00a000000000001;
    test_checki64(double2fix64(u64d.d, 40), -13ll * (1ll << 38) - 1ll, "double2fix6414b");

    u64d.u = 0xc00a000080000001;
    test_checki64(double2fix64(u64d.d, 20), -13ll * (1ll << 18) - 2ll, "double2fix6415c");
    u64d.u = 0xc00a000080000000;
    test_checki64(double2fix64(u64d.d, 20), -13ll * (1ll << 18) - 1ll, "double2fix6415d");
    u64d.u = 0xc00a000000000001;
    test_checki64(double2fix64(u64d.d, 20), -13ll * (1ll << 18) - 1ll, "double2fix6415e");
    u64d.u = 0xc00a000000000000;
    test_checki64(double2fix64(u64d.d, 20), -13ll * (1ll << 18), "double2fix6415g");

    u64d.u = 0xc00a000080000001;
    test_checki64(double2fix64(u64d.d, 19), -13ll * (1ll << 17) - 1ll, "double2fix6415h");
    u64d.u = 0xc00a000080000000;
    test_checki64(double2fix64(u64d.d, 19), -13ll * (1ll << 17) - 1ll, "double2fix6415i");
    u64d.u = 0xc00a000000000001;
    test_checki64(double2fix64(u64d.d, 19), -13ll * (1ll << 17) - 1ll, "double2fix6415j");
    u64d.u = 0xc00a000000000000;
    test_checki64(double2fix64(u64d.d, 19), -13ll * (1ll << 17), "double2fix6415k");

    printf("double2ufix64\n");
    test_checku64(double2ufix64(3.5, 8), 0x380, "double2ufix641");
    test_checku64(double2ufix64(-3.5, 8), 0, "double2ufix642");
    test_checku64(double2ufix64(32768.0, 16), 32768ull << 16, "double2ufix643");
    test_checku64(double2ufix64(65536.0, 16), 65536ull << 16, "double2ufix644");
    test_checku64(double2ufix64(2147483648.0, 16), 2147483648ull << 16, "double2ufix644b");
    test_checku64(double2ufix64(INFINITY, 16), UINT64_MAX, "double2ufix645");
    test_checku64(double2ufix64(-INFINITY, 16), 0, "double2ufix645b");
    test_checku64(double2ufix64(INFINITY, -16), UINT64_MAX, "double2ufix645c");
    test_checku64(double2ufix64(-INFINITY, -16), 0, "double2ufix645d");
    test_checku64(double2ufix64(3.24999, 2), 12, "double2ufix646");
    test_checku64(double2ufix64(3.25, 2), 13, "double2ufix647");
    test_checku64(double2ufix64(3.0, -1), 1, "double2ufix648"); // not very useful
    test_checki64(double2ufix64(0.0, 16), 0, "double2ufix649");
    test_checki64(double2ufix64(-0.0, 16), 0, "double2ufix6410");

    printf("double2fix_z\n");
    test_checki(double2fix_z(3.5, 8), 0x380, "double2fix_z1");
    test_checki(double2fix_z(-3.5, 8), -0x380, "double2fix_z2");
    test_checki(double2fix_z(32768.0, 16), INT32_MAX, "double2fix_z3");
    test_checki(double2fix_z(65536.0, 16), INT32_MAX, "double2fix_z4");
    test_checki(double2fix_z(INFINITY, 16), INT32_MAX, "double2fix_z5");
    test_checki(double2fix_z(-INFINITY, 16), INT32_MIN, "double2fix_z5b");
    test_checki(double2fix_z(INFINITY, -50), INT32_MAX, "double2fix_z5c");
    test_checki(double2fix_z(-INFINITY, -50), INT32_MIN, "double2fix_z5d");
    test_checki(double2fix_z(3.24999, 2), 12, "double2fix_z6");
    test_checki(double2fix_z(3.25, 2), 13, "double2fix_z7");
    test_checki(double2fix_z(-3.24999, 2), -12, "double2fix_z8");
    test_checki(double2fix_z(-3.25, 2), -13, "double2fix_z9");
    test_checki(double2fix_z(-0.75, 1), -1, "double2fix_z10");
    test_checki(double2fix_z(-3.0, -1), -1, "double2fix_z11"); // not very useful
    test_checki(double2fix_z(0.0, 16), 0, "double2fix_z12");
    test_checki(double2fix_z(-0.0, 16), 0, "double2fix_z13");
    test_checki(double2fix_z(0.0, -16), 0, "double2fix_z12b");
    test_checki(double2fix_z(-0.0, -16), 0, "double2fix_z13b");

    printf("double2ufix_z\n");
    test_checku(double2ufix_z(3.5, 8), 0x380, "double2ufix_z1");
    test_checku(double2ufix_z(-3.5, 8), 0, "double2ufix_z2");
    test_checku(double2ufix_z(32768.0, 16), 32768 << 16, "double2ufix_z3");
    test_checku(double2ufix_z(65536.0, 16), UINT32_MAX, "double2ufix_z4");
    test_checku(double2ufix_z(INFINITY, 16), UINT32_MAX, "double2ufix_z5");
    test_checku(double2ufix_z(-INFINITY, 16), 0, "double2ufix_z5b");
    test_checku(double2ufix_z(INFINITY, 16), UINT32_MAX, "double2ufix_z5c");
    test_checku(double2ufix_z(-INFINITY, 16), 0, "double2ufix_z5d");
    test_checku(double2ufix_z(3.24999, 2), 12, "double2ufix_z6");
    test_checku(double2ufix_z(3.25, 2), 13, "double2ufix_z7");
    test_checku(double2ufix_z(3.0, -1), 1, "double2ufix_z8"); // not very useful
    test_checki(double2ufix_z(0.0, 16), 0, "double2fix_z9");
    test_checki(double2ufix_z(-0.0, 16), 0, "double2fix_z10");
    test_checki(double2ufix_z(0.0, -16), 0, "double2fix_z11");
    test_checki(double2ufix_z(-0.0, -16), 0, "double2fix_z12");

    printf("double2fix64_z\n");
    test_checki64(double2fix64_z(3.5, 8), 0x380, "double2fix64_z1");
    test_checki64(double2fix64_z(-3.5, 8), -0x380, "double2fix64_z2");
    test_checki64(double2fix64_z(32768.0, 16), 32768ll << 16, "double2fix64_z3");
    test_checki64(double2fix64_z(65536.0, 16), 65536ll << 16, "double2fix64_z4");
    test_checki64(double2fix64_z(65536.0 * 65536.0 * 32768.0, 16), INT64_MAX, "double2fix64_z4b");
    test_checki64(double2fix64_z(INFINITY, 16), INT64_MAX, "double2fix64_z5");
    test_checki64(double2fix64_z(-INFINITY, 16), INT64_MIN, "double2fix64_z5");
    test_checki64(double2fix64_z(INFINITY, 16), INT64_MAX, "double2fix64_z5");
    test_checki64(double2fix64_z(-INFINITY, 16), INT64_MIN, "double2fix64_z5");
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
    test_checki64(double2fix64_z(u64d.d, 20), -13ll * (1ll << 18), "double2fix64_z15g");

    u64d.u = 0xc00a000080000001;
    test_checki64(double2fix64_z(u64d.d, 19), -13ll * (1ll << 17), "double2fix64_z15h");
    u64d.u = 0xc00a000080000000;
    test_checki64(double2fix64_z(u64d.d, 19), -13ll * (1ll << 17), "double2fix64_z15i");
    u64d.u = 0xc00a000000000001;
    test_checki64(double2fix64_z(u64d.d, 19), -13ll * (1ll << 17), "double2fix64_z15j");
    u64d.u = 0xc00a000000000000;
    test_checki64(double2fix64_z(u64d.d, 19), -13ll * (1ll << 17), "double2fix64_z15k");

    printf("double2ufix64_z\n");
    test_checku64(double2ufix64_z(3.5, 8), 0x380, "double2ufix64_z1");
    test_checku64(double2ufix64_z(-3.5, 8), 0, "double2ufix64_z2");
    test_checku64(double2ufix64_z(32768.0, 16), 32768ll << 16, "double2ufix64_z3");
    test_checku64(double2ufix64_z(65536.0, 16), 65536ll << 16, "double2ufix64_z4");
    test_checki64(double2ufix64_z(65536.0 * 65536.0 * 65536.0, 16), UINT64_MAX, "double2fix64_z4b");
    test_checku64(double2ufix64_z(INFINITY, 16), UINT64_MAX, "double2ufix64_z5");
    test_checku64(double2ufix64_z(-INFINITY, 16), 0, "double2ufix64_z5b");
    test_checku64(double2ufix64_z(INFINITY, 16), UINT64_MAX, "double2ufix64_z5c");
    test_checku64(double2ufix64_z(-INFINITY, 16), 0, "double2ufix64_z5d");
    test_checku64(double2ufix64_z(3.24999, 2), 12, "double2ufix64_z6");
    test_checku64(double2ufix64_z(3.25, 2), 13, "double2ufix64_z7");
    test_checki64(double2ufix64_z(3.0, -1), 1, "double2fuix64_z8"); // not very useful
    test_checki64(double2ufix64_z(0.0, 16), 0, "double2ufix64_z9");
    test_checki64(double2ufix64_z(-0.0, 16), 0, "double2ufix64_z10");
    test_checki64(double2ufix64_z(0.0, -16), 0, "double2ufix64_z11");
    test_checki64(double2ufix64_z(-0.0, -16), 0, "double2ufix64_z12");

    printf("double2int\n");
    test_checki(double2int(0.0), 0, "double2int1");
    test_checki(double2int(0.25), 0, "double2int1b");
    test_checki(double2int(0.5), 0, "double2int2");
    test_checki(double2int(0.75), 0, "double2int2b");
    test_checki(double2int(1.0), 1, "double2int3");
    test_checki(double2int(-10.0), -10, "double2int3a");
    test_checki(double2int(-0.0), 0, "double2int3b");
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

    printf("double2int64\n");
    test_checki64(double2int64(0.0), 0, "double2int641");
    test_checki64(double2int64(0.25), 0, "double2int641b");
    test_checki64(double2int64(0.5), 0, "double2int642");
    test_checki64(double2int64(0.75), 0, "double2int642b");
    test_checki64(double2int64(1.0), 1, "double2int643");
    test_checki64(double2int64(-10.0), -10, "double2int643a");
    test_checki64(double2int64(-0.0), 0, "double2int643b");
    test_checki64(double2int64(-0.25), -1, "double2int644");
    test_checki64(double2int64(-0.5), -1, "double2int644b");
    test_checki64(double2int64(-0.75), -1, "double2int645");
    test_checki64(double2int64(-1.0), -1, "double2int645b");
    // todo test correct rounding around maximum precision
    test_checki64(double2int64(2147483647.0), INT32_MAX, "double2int646");
    test_checki64(double2int64(21474836470.0), 21474836470ll, "double2int647");
    test_checki64(double2int64(-2147483648.0), INT32_MIN, "double2int648");
    test_checki64(double2int64(-21474836480.0), -21474836480ll, "double2int649");
    test_checki64(double2int64(-2.5), -3, "double2int6410");
    test_checki64(double2int64(-2.4), -3, "double2int6411");
    u64d.u = 0xc000000000000000ull;
    test_checki64(double2int64(u64d.d), -2, "double2int6412");
    u64d.u = 0xc008000000000000ull;
    test_checki64(double2int64(u64d.d), -3, "double2int6412b");
    u64d.u = 0xc000000000000001ull;
    test_checki64(double2int64(u64d.d), -3, "double2int6412c");
    u64d.u = 0xc000000080000000ull;
    test_checki64(double2int64(u64d.d), -3, "double2int6412d");
    u64d.u = 0xc000000100000000ull;
    test_checki64(double2int64(u64d.d), -3, "double2int6412e");
    u64d.u = 0xc000000100000001ull;
    test_checki64(double2int64(u64d.d), -3, "double2int6412f");

    printf("double2uint64\n");
    test_checku64(double2uint64(0.0), 0, "double2uint641");
    test_checku64(double2uint64(0.25), 0, "double2uint642");
    test_checku64(double2uint64(0.5), 0, "double2uint643");
    test_checku64(double2uint64(0.75), 0, "double2uint644");
    test_checku64(double2uint64(1.0), 1, "double2uint645");
    test_checku64(double2uint64(2147483647.0), INT32_MAX, "double2uint646");
    test_checku64(double2uint64(2147483648.0), INT32_MAX+1u, "double2uint647");
    // todo test correct rounding around maximum precision
    test_checku64(double2uint64(4294967294.5), 4294967294ull, "double2uint648");
    test_checku64(double2uint64(4294967295.0), 4294967295ull, "double2uint649");
    test_checku64(double2uint64(42949672950.0), 42949672950, "double2uint6410");
#endif

    // // These methods round towards 0.
    printf("double2int_z\n");
    test_checki(double2int_z(0.0), 0, "double2int_z1");
    test_checki(double2int_z(0.25), 0, "double2int_z1b");
    test_checki(double2int_z(0.5), 0, "double2int_z2");
    test_checki(double2int_z(0.75), 0, "double2int_z2b");
    test_checki(double2int_z(1.0), 1, "double2int_z3");
    test_checki(double2int_z(-10.0), -10, "double2int_z3a");
    test_checki(double2int_z(-0.0), 0, "double2int_z3b");
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

    printf("double2int64_z\n");
    test_checki64(double2int64_z(0.0), 0, "double2int64_z1");
    test_checki64(double2int64_z(0.25), 0, "double2int64_z1b");
    test_checki64(double2int64_z(0.5), 0, "double2int64_z2");
    test_checki64(double2int64_z(0.75), 0, "double2int64_z2b");
    test_checki64(double2int64_z(1.0), 1, "double2int64_z3");
    test_checki64(double2int64_z(-10.0), -10, "double2int64_z3a");
    test_checki64(double2int64_z(-0.0), 0, "double2int64_z3b");
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
