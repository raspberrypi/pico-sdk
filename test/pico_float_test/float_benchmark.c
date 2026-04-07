#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico/float.h"
#include "pico/platform/cpu_regs.h"

#if defined(LLVM_LIBC_COMMON_H) && !defined(__LLVM_LIBC__)
#define __LLVM_LIBC__ 1
#endif

static void init_systick() {
#ifdef __riscv
    // Stop, clear then start 64-bit RISC-V platform timer for boot timing
    sio_hw->mtime_ctrl = 0;
    sio_hw->mtime = 0;
    sio_hw->mtimeh = 0;
    sio_hw->mtime_ctrl = SIO_MTIME_CTRL_FULLSPEED_BITS | SIO_MTIME_CTRL_EN_BITS;
#else
    systick_hw->csr = 0;
    systick_hw->rvr = ARM_CPU_PREFIXED(SYST_RVR_RELOAD_BITS);
    systick_hw->csr = ARM_CPU_PREFIXED(SYST_CSR_CLKSOURCE_BITS) | ARM_CPU_PREFIXED(SYST_CSR_ENABLE_BITS);
#endif
}

// Stop the compiler from constant-folding a hardware base pointer into the
// pointers to individual registers, in cases where constant folding has
// produced redundant 32-bit pointer literals that could have been load/store
// offsets. (Note typeof(ptr+0) gives non-const, for +r constraint.) E.g.
//     uart_hw_t *uart0 = __get_opaque_ptr(uart0_hw);
#define __get_opaque_ptr(ptr) ({ \
    typeof((ptr)+0) __opaque_ptr = (ptr); \
    asm ("" : "+r"(__opaque_ptr)); \
    __opaque_ptr; \
})

static __force_inline io_ro_32 *systick_value_ptr() {
#ifdef __riscv
    return &sio_hw->mtime;
#else
    return __get_opaque_ptr(&systick_hw->cvr);
#endif
}

static int cycle_diff(uint32_t systick1, uint32_t systick2) {
#ifdef __riscv
    return systick2 - systick1 - 1;
#else
    static_assert(ARM_CPU_PREFIXED(SYST_CVR_CURRENT_LSB) == 0, "");
    uint32_t shift = 32 - ARM_CPU_PREFIXED(SYST_CVR_CURRENT_MSB);
    return (((int32_t)((systick1 << shift) - (systick2 << shift))) >> shift) - 1; // -1 since the second systick read costs one
#endif
}

#define timer_func_def(name) static __noinline int __not_in_flash_func(time_##name)

static float f_a[] = {1.3f, -200.3f, 1.6e15f, 1e-2f};
static float f_b[] = {-121.3f, 50.3f, 27.9f, 1.7e23f};
static float f_c[] = {20.3f, -50.3f, -3.9e-3f, -4.1e7f};
static float f_m1to1[] = {-0.5f, .9999f, 0.1f, -0.999999f};

static int32_t i_pow[] = {3,6,27,-10};
static float f_positive[] = {0.0f, 3.7f, 1245325.f, 1e27f};
static float f_1plus[] = {1.0f, 3.7f, 1245325.f, 1e27f};

static float f_smaller[] = {-1000.3f, 200.3f, 1.6e15f};
static float f_bigger[] = {-121.3f, 5000.3f, 1.6e16f};

static int32_t i_32[]  = { 0, 3, -200, INT32_MIN, INT32_MAX };
static int64_t i_64[]  = { 0, 3, -200, 0x123456789abcll, -0x123456789abcll, INT64_MIN, INT64_MAX };

// bits for fixed point conversions
static int32_t n_32[]  = { 0, 3, -3, 16, -16 };

static_assert(count_of(f_a) == count_of(f_b), "");
static_assert(count_of(f_a) == count_of(f_c), "");
static_assert(count_of(f_a) == count_of(i_pow), "");
static_assert(count_of(f_a) == count_of(f_positive), "");

static_assert(count_of(f_smaller) == count_of(f_bigger), "");

static float time_unary_func(int (*timer)(float), float *f, uint count) {
    float total = 0.f;
    for (uint i=0;i<count;i++) {
        total += (float)timer(f[i]);
    }
    return total / count;
}

static float time_unary_n_func(int (*timer)(float, int32_t), float *f, int32_t *n, uint count) {
    float total = 0.f;
    for (uint i=0;i<count;i++) {
        total += (float)timer(f[i], n[i]);
    }
    return total / count;
}

static float time_binary_func(int (*timer)(float,float), float *f1, float *f2, uint count) {
    float total = 0.f;
    for (uint i=0;i<count;i++) {
        total += (float)timer(f1[i], f2[i]);
    }
    return total / count;
}

static float time_binary_int_func(int (*timer)(float,int32_t), float *f, int32_t *i32, uint count) {
    float total = 0.f;
    for (uint i=0;i<count;i++) {
        total += (float)timer(f[i], i32[i]);
    }
    return total / count;
}

static float time_ternary_func(int (*timer)(float,float,float), float *f1, float *f2, float *f3, uint count) {
    float total = 0.f;
    for (uint i=0;i<count;i++) {
        total += (float)timer(f1[i], f2[i], f3[i]);
    }
    return total / count;
}

static float time_unary_int32_func(int (*timer)(int32_t), int32_t *i32, uint count) {
    float total = 0.f;
    for (uint i=0;i<count;i++) {
        total += (float)timer(i32[i]);
    }
    return total / count;
}

static float time_unary_int32_n_func(int (*timer)(int32_t, int32_t), int32_t *i32, int32_t *n, uint count) {
    float total = 0.f;
    for (uint i=0;i<count;i++) {
        total += (float)timer(i32[i], n[i]);
    }
    return total / count;
}

static float time_unary_int64_func(int (*timer)(int64_t), int64_t *i64, uint count) {
    float total = 0.f;
    for (uint i=0;i<count;i++) {
        total += (float)timer(i64[i]);
    }
    return total / count;
}

static float time_unary_int64_n_func(int (*timer)(int64_t, int32_t), int64_t *i64, int32_t *n, uint count) {
    float total = 0.f;
    for (uint i=0;i<count;i++) {
        total += (float)timer(i64[i], n[i]);
    }
    return total / count;
}

#if !defined(__ARM_FP) || ((__ARM_FP & 4) == 0)
#  define EMITS_VFP 0
#else
#  define EMITS_VFP 1
#endif

#if defined(__ARM_PCS_VFP)
#  define USING_HARD_FLOAT_ABI 1
#else
#  define USING_HARD_FLOAT_ABI 0
#endif

// #pragma message("__ARM_FP = " __XSTRING(__ARM_FP))
// #pragma message("__ARM_PCS_VFP = " __XSTRING(__ARM_PCS_VFP))
// #pragma message("__SOFTFP__ = " __XSTRING(__SOFTFP__))
// #pragma message("EMITS_VFP = " __XSTRING(EMITS_VFP))
// #pragma message("USING_HARD_FLOAT_ABI = " __XSTRING(USING_HARD_FLOAT_ABI))

#ifdef __riscv
#define LOAD_COST 1
#define STORE_COST 1
#else
#define LOAD_COST 2
#define STORE_COST 2
#endif

#define FLOAT_INPUT_COST LOAD_COST
#define FLOAT_OUTPUT_COST STORE_COST
#define INT_INPUT_COST LOAD_COST
#define INT_OUTPUT_COST STORE_COST
#define INT64_INPUT_COST (LOAD_COST * 2)
#define INT64_OUTPUT_COST (STORE_COST * 2)
#define DOUBLE_OUTPUT_COST (STORE_COST * 2)

timer_func_def(fadd)(volatile float a, volatile float b) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = a + b;
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST * 2 - FLOAT_OUTPUT_COST;
}

timer_func_def(fsub)(volatile float a, volatile float b) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = a - b;
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST * 2 - FLOAT_OUTPUT_COST;
}

timer_func_def(fmul)(volatile float a, volatile float b) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = a * b;
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST * 2 - FLOAT_OUTPUT_COST;
}

timer_func_def(fdiv)(volatile float a, volatile float b) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = a / b;
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST * 2 - FLOAT_OUTPUT_COST;
}

timer_func_def(fdiv_fast)(volatile float a, volatile float b) {
#if PICO_FLOAT_HAS_FDIV_FAST
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = fdiv_fast(a,b);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST * 2 - FLOAT_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(fsqrt)(volatile float a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = sqrtf(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - FLOAT_OUTPUT_COST;
}

timer_func_def(fsqrt_fast)(volatile float a) {
#if PICO_FLOAT_HAS_SQRTF_FAST
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = sqrtf_fast(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - FLOAT_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(ffma)(volatile float a, volatile float b, volatile float c) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = fmaf(a, b, c);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST * 3 - FLOAT_OUTPUT_COST;
}

#define DCMP_OVERHEAD 4
timer_func_def(fcmpeq)(volatile float a, volatile float b) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile bool v = a == b;
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST * 2 - DCMP_OVERHEAD;
}

timer_func_def(fcmplt)(volatile float a, volatile float b) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile bool v = a < b;
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST * 2 - DCMP_OVERHEAD;
}

timer_func_def(fcmple)(volatile float a, volatile float b) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile bool v = a <= b;
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST * 2 - DCMP_OVERHEAD;
}

timer_func_def(fcmpgt)(volatile float a, volatile float b) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile bool v = a > b;
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST * 2 - DCMP_OVERHEAD;
}

timer_func_def(fcmpge)(volatile float a, volatile float b) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile bool v = a >= b;
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST * 2 - DCMP_OVERHEAD;
}

timer_func_def(fcmpun)(volatile float a, volatile float b) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile bool v = __builtin_isunordered(a, b);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST * 2 - DCMP_OVERHEAD;
}

timer_func_def(i2f)(volatile int32_t i) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = (float)i;
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - INT_INPUT_COST - FLOAT_OUTPUT_COST;
}

timer_func_def(ui2f)(volatile int32_t i) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = (float)(uint32_t)i;
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - INT_INPUT_COST - FLOAT_OUTPUT_COST;
}

timer_func_def(int2float)(volatile int32_t i) {
#if PICO_FLOAT_HAS_INT32_TO_FLOAT_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = int2float(i);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - INT_INPUT_COST - FLOAT_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(uint2float)(volatile int32_t i) {
#if PICO_FLOAT_HAS_INT32_TO_FLOAT_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = uint2float(i);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - INT_INPUT_COST - FLOAT_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(f2i)(volatile float a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile int32_t x = (int32_t) a;
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - INT_OUTPUT_COST;
}

timer_func_def(f2ui)(volatile float a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile uint32_t x = (uint32_t) a;
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - INT_OUTPUT_COST;
}

timer_func_def(float2int_z)(volatile float a) {
#if PICO_FLOAT_HAS_FLOAT_TO_INT32_Z_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile int32_t x = float2int_z(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - INT_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(float2uint_z)(volatile float a) {
#if PICO_FLOAT_HAS_FLOAT_TO_INT32_Z_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile uint32_t x = float2uint_z(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - INT_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(float2int)(volatile float a) {
#if PICO_FLOAT_HAS_FLOAT_TO_INT32_M_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile int32_t x = float2int(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - INT_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(float2uint)(volatile float a) {
#if PICO_FLOAT_HAS_FLOAT_TO_INT32_M_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile uint32_t x = float2uint(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - INT_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(l2f)(volatile int64_t i) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = (float)i;
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - INT64_INPUT_COST - FLOAT_OUTPUT_COST;
}

timer_func_def(ul2f)(volatile int64_t i) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = (float)(uint64_t)i;
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - INT64_INPUT_COST - FLOAT_OUTPUT_COST;
}

timer_func_def(int642float)(volatile int64_t i) {
#if PICO_FLOAT_HAS_INT64_TO_FLOAT_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = int642float(i);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - INT64_INPUT_COST - FLOAT_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(uint642float)(volatile int64_t i) {
#if PICO_FLOAT_HAS_INT64_TO_FLOAT_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = uint642float(i);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - INT64_INPUT_COST - FLOAT_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(f2l)(volatile float a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile int64_t x = (int64_t) a;
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - INT64_OUTPUT_COST;
}

timer_func_def(f2ul)(volatile float a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile uint64_t x = (uint64_t) a;
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - INT64_OUTPUT_COST;
}

timer_func_def(float2int64_z)(volatile float a) {
#if PICO_FLOAT_HAS_FLOAT_TO_INT64_Z_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile int64_t x = float2int64_z(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - INT64_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(float2uint64_z)(volatile float a) {
#if PICO_FLOAT_HAS_FLOAT_TO_INT64_Z_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile uint64_t x = float2uint64_z(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - INT64_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(float2int64)(volatile float a) {
#if PICO_FLOAT_HAS_FLOAT_TO_INT64_M_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile int64_t x = float2int64(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - INT64_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(float2uint64)(volatile float a) {
#if PICO_FLOAT_HAS_FLOAT_TO_INT64_M_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile uint64_t x = float2uint64(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - INT64_OUTPUT_COST;
#else
    return -1;
#endif
}

// ----------------------

timer_func_def(f2d)(volatile float a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = (double) a;
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - DOUBLE_OUTPUT_COST;
}

// ----------------------

timer_func_def(fix2float)(volatile int32_t i, volatile int32_t nn) {
#if PICO_FLOAT_HAS_FIX32_TO_FLOAT_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    int32_t n = nn; pico_default_asm_volatile( "" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile float x = fix2float(i, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - INT_INPUT_COST - FLOAT_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(ufix2float)(volatile int32_t i, volatile int32_t nn) {
#if PICO_FLOAT_HAS_FIX32_TO_FLOAT_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    int32_t n = nn; pico_default_asm_volatile( "" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile float x = ufix2float(i, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - INT_INPUT_COST - FLOAT_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(float2fix_z)(volatile float a, volatile int32_t nn) {
#if PICO_FLOAT_HAS_FLOAT_TO_FIX32_Z_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    int n = nn; pico_default_asm_volatile( "" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile int32_t x = float2fix_z(a, nn);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - INT_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(float2ufix_z)(volatile float a, volatile int32_t nn) {
#if PICO_FLOAT_HAS_FLOAT_TO_FIX32_Z_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    int32_t n = nn; pico_default_asm_volatile( "" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile uint32_t x = float2ufix_z(a, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - INT_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(float2fix)(volatile float a, volatile int32_t nn) {
#if PICO_FLOAT_HAS_FLOAT_TO_FIX32_M_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    int32_t n = nn; pico_default_asm_volatile( "" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile int32_t x = float2fix(a, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - INT_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(float2ufix)(volatile float a, volatile int32_t nn) {
#if PICO_FLOAT_HAS_FLOAT_TO_FIX32_M_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    int32_t n = nn; pico_default_asm_volatile( "" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile uint32_t x = float2ufix(a, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - INT_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(fix642float)(volatile int64_t i, volatile int32_t nn) {
#if PICO_FLOAT_HAS_FIX64_TO_FLOAT_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    int32_t n = nn; pico_default_asm_volatile( "" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile float x = fix642float(i, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - INT64_INPUT_COST - FLOAT_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(ufix642float)(volatile int64_t i, volatile int32_t nn) {
#if PICO_FLOAT_HAS_FIX64_TO_FLOAT_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    int32_t n = nn; pico_default_asm_volatile( "" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile float x = ufix642float(i, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - INT64_INPUT_COST - FLOAT_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(float2fix64_z)(volatile float a, volatile int32_t nn) {
#if PICO_FLOAT_HAS_FLOAT_TO_FIX64_Z_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    int32_t n = nn; pico_default_asm_volatile( "" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile int64_t x = float2fix64_z(a, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - INT64_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(float2ufix64_z)(volatile float a, volatile int32_t nn) {
#if PICO_FLOAT_HAS_FLOAT_TO_FIX64_Z_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    int32_t n = nn; pico_default_asm_volatile( "" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile uint64_t x = float2ufix64_z(a, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - INT64_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(float2fix64)(volatile float a, volatile int32_t nn) {
#if PICO_FLOAT_HAS_FLOAT_TO_FIX64_M_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    int32_t n = nn; pico_default_asm_volatile( "" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile int64_t x = float2fix64(a, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - INT64_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(float2ufix64)(volatile float a, volatile int32_t nn) {
#if PICO_FLOAT_HAS_FLOAT_TO_FIX64_M_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    int32_t n = nn; pico_default_asm_volatile( "" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile uint64_t x = float2ufix64(a, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - INT64_OUTPUT_COST;
#else
    return -1;
#endif
}

// ----------------------

// ----------------------

timer_func_def(fix2float_c)(volatile int32_t i) {
#if PICO_FLOAT_HAS_FIX32_TO_FLOAT_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    const int n = 16; pico_default_asm_volatile("" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile float x = fix2float(i, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - INT_INPUT_COST - FLOAT_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(ufix2float_c)(volatile int32_t i) {
#if PICO_FLOAT_HAS_FIX32_TO_FLOAT_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    const int n = 16; pico_default_asm_volatile("" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile float x = ufix2float(i, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - INT_INPUT_COST - FLOAT_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(float2fix_z_c)(volatile float a) {
#if PICO_FLOAT_HAS_FLOAT_TO_FIX32_Z_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    const int n = 16; pico_default_asm_volatile("" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile int32_t x = float2fix_z(a, 16);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - INT_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(float2ufix_z_c)(volatile float a) {
#if PICO_FLOAT_HAS_FLOAT_TO_FIX32_Z_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    const int n = 16; pico_default_asm_volatile("" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile uint32_t x = float2ufix_z(a, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - INT_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(float2fix_c)(volatile float a) {
#if PICO_FLOAT_HAS_FLOAT_TO_FIX32_M_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    const int n = 16; pico_default_asm_volatile("" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile int32_t x = float2fix(a, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - INT_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(float2ufix_c)(volatile float a) {
#if PICO_FLOAT_HAS_FLOAT_TO_FIX32_M_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    const int n = 16; pico_default_asm_volatile("" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile uint32_t x = float2ufix(a, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - INT_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(fix642float_c)(volatile int64_t i) {
#if PICO_FLOAT_HAS_FIX64_TO_FLOAT_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    const int n = 16; pico_default_asm_volatile("" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile float x = fix642float(i, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - INT64_INPUT_COST - FLOAT_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(ufix642float_c)(volatile int64_t i) {
#if PICO_FLOAT_HAS_FIX64_TO_FLOAT_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    const int n = 16; pico_default_asm_volatile("" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile float x = ufix642float(i, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - INT64_INPUT_COST - FLOAT_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(float2fix64_z_c)(volatile float a) {
#if PICO_FLOAT_HAS_FLOAT_TO_FIX64_Z_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    const int n = 16; pico_default_asm_volatile("" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile int64_t x = float2fix64_z(a, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - INT64_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(float2ufix64_z_c)(volatile float a) {
#if PICO_FLOAT_HAS_FLOAT_TO_FIX64_Z_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    const int n = 16; pico_default_asm_volatile("" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile uint64_t x = float2ufix64_z(a, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - INT64_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(float2fix64_c)(volatile float a) {
#if PICO_FLOAT_HAS_FLOAT_TO_FIX64_M_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    const int n = 16; pico_default_asm_volatile("" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile int64_t x = float2fix64(a, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - INT64_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(float2ufix64_c)(volatile float a) {
#if PICO_FLOAT_HAS_FLOAT_TO_FIX64_M_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    const int n = 16; pico_default_asm_volatile("" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile uint64_t x = float2ufix64(a, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - INT64_OUTPUT_COST;
#else
    return -1;
#endif
}

// ----------------------

timer_func_def(fcos)(volatile float a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = cosf(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - FLOAT_OUTPUT_COST;
}

timer_func_def(fsin)(volatile float a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = sinf(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - FLOAT_OUTPUT_COST;
}

timer_func_def(fsincos)(volatile float a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    float s, c;
    sincosf(a, &s, &c);
    volatile float x = s;
    volatile float y = c;
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - FLOAT_OUTPUT_COST * 2;
}

timer_func_def(ftan)(volatile float a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = tanf(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - FLOAT_OUTPUT_COST;
}

timer_func_def(fatan2)(volatile float a, volatile float b) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = atan2f(a, b);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST * 2 - FLOAT_OUTPUT_COST;
}

timer_func_def(fexp)(volatile float a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = expf(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - FLOAT_OUTPUT_COST;
}

timer_func_def(flog)(volatile float a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = logf(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - FLOAT_OUTPUT_COST;
}

timer_func_def(fpowint)(volatile float a, int32_t pow) {
#if PICO_FLOAT_HAS_POWINTF
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = powintf(a, pow);;
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - FLOAT_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(fcopysign)(volatile float a, volatile float b) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = copysignf(a, b);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST * 2 - FLOAT_OUTPUT_COST;
}

timer_func_def(ftrunc)(volatile float a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = truncf(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - FLOAT_OUTPUT_COST;
}

timer_func_def(ffloor)(volatile float a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = floorf(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - FLOAT_OUTPUT_COST;
}

timer_func_def(fceil)(volatile float a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = ceilf(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - FLOAT_OUTPUT_COST;
}

timer_func_def(fround)(volatile float a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = roundf(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - FLOAT_OUTPUT_COST;
}

timer_func_def(ffmod)(volatile float a, volatile float b) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = fmodf(a, b);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST * 2 - FLOAT_OUTPUT_COST;
}

timer_func_def(fdrem)(volatile float a, volatile float b) {
    // LLVM libc is string betting the floating point functions
#if defined(__LLVM_LIBC__) && defined(__llvm__)// not sure when this is fixed && (__clang_major__ < 21)
    return -1;
#else
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = dremf(a, b);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST * 2 - FLOAT_OUTPUT_COST;
#endif
}

timer_func_def(fremainder)(volatile float a, volatile float b) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = remainderf(a, b);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST * 2 - FLOAT_OUTPUT_COST;
}

timer_func_def(fremquo)(volatile float a, volatile float b) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    int c;
    uint32_t t0 = *systick_ptr;
    volatile float x = remquof(a, b, &c);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST * 2 - FLOAT_OUTPUT_COST;
}

timer_func_def(fexp2)(volatile float a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = exp2f(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - FLOAT_OUTPUT_COST;
}

timer_func_def(flog2)(volatile float a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = log2f(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - FLOAT_OUTPUT_COST;
}

timer_func_def(fexp10)(volatile float a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = exp10f(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - FLOAT_OUTPUT_COST;
}

timer_func_def(flog10)(volatile float a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = log10f(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - FLOAT_OUTPUT_COST;
}

timer_func_def(fldexp)(volatile float a, int32_t b) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = ldexpf(a, b);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - FLOAT_OUTPUT_COST;
}

timer_func_def(fexpm1)(volatile float a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = expm1f(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - FLOAT_OUTPUT_COST;
}

timer_func_def(flog1p)(volatile float a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = log1pf(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - FLOAT_OUTPUT_COST;
}

timer_func_def(fpow)(volatile float a, volatile float b) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = powf(a, b);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST * 2 - FLOAT_OUTPUT_COST;
}

timer_func_def(fcbrt)(volatile float a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = cbrtf(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - FLOAT_OUTPUT_COST;
}

timer_func_def(facosh)(volatile float a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = acoshf(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - FLOAT_OUTPUT_COST;
}

timer_func_def(fatanh)(volatile float a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = atanhf(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - FLOAT_OUTPUT_COST;
}

timer_func_def(fhypot)(volatile float a, volatile float b) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = hypotf(a, b);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST * 2 - FLOAT_OUTPUT_COST;
}

timer_func_def(fasin)(volatile float a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = asinf(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - FLOAT_OUTPUT_COST;
}

timer_func_def(facos)(volatile float a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = acosf(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - FLOAT_OUTPUT_COST;
}

timer_func_def(fatan)(volatile float a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = atanf(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - FLOAT_OUTPUT_COST;
}

timer_func_def(fsinh)(volatile float a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = sinhf(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - FLOAT_OUTPUT_COST;
}

timer_func_def(fcosh)(volatile float a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = coshf(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - FLOAT_OUTPUT_COST;
}

timer_func_def(ftanh)(volatile float a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = tanhf(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - FLOAT_OUTPUT_COST;
}

timer_func_def(fasinh)(volatile float a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = asinhf(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - FLOAT_INPUT_COST - FLOAT_OUTPUT_COST;
}


int main() {
    init_systick();
    stdio_init_all();
#if PICO_C_COMPILER_IS_CLANG
    printf("================= Clang - ");
#else
    printf("================ GCC - ");
#endif
#if LIB_PICO_FLOAT_COMPILER
    printf("COMPILER ===\n");
#elif LIB_PICO_FLOAT_PICO
    printf("PICO ===\n");
#elif LIB_PICO_FLOAT_PICO_VFP
    printf("PICO VFP ===\n");
#elif LIB_PICO_FLOAT_PICO_DCP
    printf("PICO DCP ===\n");
#else
#error unknown float impl
#endif
#if EMITS_VFP
    printf("hard-float        true\n");
#else
    printf("hard-float        false\n");
#endif
#if USING_HARD_FLOAT_ABI
    printf("abi               hard\n");
#else
    printf("abi               soft(fp)\n");
#endif
#if PICO_RP2040
    printf("platform          rp2040\n");
#elif PICO_RP2350
    printf("platform          rp2350\n");
#endif
#if PICO_RISCV
    printf("arch              risc-v\n");
#else
    printf("arch              arm\n");
#endif
    printf("----------------- Basic ---\n");
    printf("fadd              %g\n", time_binary_func(time_fadd, f_a, f_b, count_of(f_a)));
    printf("fsub              %g\n", time_binary_func(time_fsub, f_a, f_b, count_of(f_a)));
    printf("fmul              %g\n", time_binary_func(time_fmul, f_a, f_b, count_of(f_a)));
    printf("fdiv              %g\n", time_binary_func(time_fdiv, f_a, f_b, count_of(f_a)));
    printf("fsqrt             %g\n", time_unary_func(time_fsqrt, f_positive, count_of(f_a)));
    printf("ffma              %g\n", time_ternary_func(time_ffma, f_a, f_b, f_c, count_of(f_a)));
    printf("fdiv_fast         %g\n", time_binary_func(time_fdiv_fast, f_a, f_b, count_of(f_a)));
    printf("fsqrt_fast        %g\n", time_unary_func(time_fsqrt_fast, f_positive, count_of(f_a)));
    printf("----------------- Comparison ---\n");
    // these are hard to make the compiler generate it seems
    // printf("fccmpeq               %g\n", time_binary_func(time_fccmpeq, f_a, f_b, count_of(f_a)));
    // printf("fcrcmple              %g\n", time_binary_func(time_fcrcmple, f_a, f_b, count_of(f_a)));
    // printf("fccmple               %g\n", time_binary_func(time_fccmple, f_a, f_b, count_of(f_a)));
    printf("fcmpeq            %g\n", time_binary_func(time_fcmpeq, f_a, f_a, count_of(f_a)));
    printf("fcmplt            %g\n", time_binary_func(time_fcmplt, f_smaller, f_bigger, count_of(f_smaller)));
    printf("fcmple            %g\n", time_binary_func(time_fcmple, f_smaller, f_bigger, count_of(f_smaller)));
    printf("fcmpge            %g\n", time_binary_func(time_fcmpge, f_bigger, f_smaller, count_of(f_bigger)));
    printf("fcmpgt            %g\n", time_binary_func(time_fcmpgt, f_bigger, f_smaller, count_of(f_bigger)));
    printf("fcmpun            %g\n", time_binary_func(time_fcmpun, f_a, f_a, count_of(f_a)));
    printf("----------------- 32-bit Conversions ---\n");
    printf("i2f               %g\n", time_unary_int32_func(time_i2f, i_32, count_of(i_32)));
    printf("ui2f              %g\n", time_unary_int32_func(time_ui2f, i_32, count_of(i_32)));
    printf("int2float         %g\n", time_unary_int32_func(time_int2float, i_32, count_of(i_32)));
    printf("uint2float        %g\n", time_unary_int32_func(time_uint2float, i_32, count_of(i_32)));
    printf("f2i               %g\n", time_unary_func(time_f2i, f_a, count_of(f_a)));
    printf("f2ui              %g\n", time_unary_func(time_f2ui, f_a, count_of(f_a)));
    printf("float2int_z       %g\n", time_unary_func(time_float2int_z, f_a, count_of(f_a)));
    printf("float2uint_z      %g\n", time_unary_func(time_float2uint_z, f_a, count_of(f_a)));
    printf("float2int         %g\n", time_unary_func(time_float2int, f_a, count_of(f_a)));
    printf("float2uint        %g\n", time_unary_func(time_float2uint, f_a, count_of(f_a)));
    printf("----------------- 64-bit Conversions ---\n");
    printf("l2f               %g\n", time_unary_int64_func(time_l2f, i_64, count_of(i_64)));
    printf("ul2f              %g\n", time_unary_int64_func(time_ul2f, i_64, count_of(i_64)));
    printf("int642float       %g\n", time_unary_int64_func(time_int642float, i_64, count_of(i_64)));
    printf("uint642float      %g\n", time_unary_int64_func(time_uint642float, i_64, count_of(i_64)));
    printf("f2l               %g\n", time_unary_func(time_f2l, f_a, count_of(f_a)));
    printf("f2ul              %g\n", time_unary_func(time_f2ul, f_a, count_of(f_a)));
    printf("float2int64_z     %g\n", time_unary_func(time_float2int64_z, f_a, count_of(f_a)));
    printf("float2uint64_z    %g\n", time_unary_func(time_float2uint64_z, f_a, count_of(f_a)));
    printf("float2int64       %g\n", time_unary_func(time_float2int64, f_a, count_of(f_a)));
    printf("float2uint64      %g\n", time_unary_func(time_float2uint64, f_a, count_of(f_a)));
    printf("f2d               %g\n", time_unary_func(time_f2d, f_a, count_of(f_a)));
    printf("----------------- Fixed-point (Constant Point) 32-bit Conversions ---\n");
    printf("fix2float_c       %g\n", time_unary_int32_func(time_fix2float_c, i_32, count_of(i_32)));
    printf("ufix2float_c      %g\n", time_unary_int32_func(time_ufix2float_c, i_32, count_of(i_32)));
    printf("float2fix_z_c     %g\n", time_unary_func(time_float2fix_z_c, f_a, count_of(f_a)));
    printf("float2ufix_z_c    %g\n", time_unary_func(time_float2ufix_z_c, f_a, count_of(f_a)));
    printf("float2fix_c       %g\n", time_unary_func(time_float2fix_c, f_a, count_of(f_a)));
    printf("float2ufix_c      %g\n", time_unary_func(time_float2ufix_c, f_a, count_of(f_a)));
    printf("----------------- Fixed-point (Constant Point) 64-bit Conversions ---\n");
    printf("fix642float_c     %g\n", time_unary_int64_func(time_fix642float_c, i_64, count_of(i_64)));
    printf("ufix642float_c    %g\n", time_unary_int64_func(time_ufix642float_c, i_64, count_of(i_64)));
    printf("float2fix64_z_c   %g\n", time_unary_func(time_float2fix64_z_c, f_a, count_of(f_a)));
    printf("float2ufix64_z_c  %g\n", time_unary_func(time_float2ufix64_z_c, f_a, count_of(f_a)));
    printf("float2fix64_c     %g\n", time_unary_func(time_float2fix64_c, f_a, count_of(f_a)));
    printf("float2ufix64_c    %g\n", time_unary_func(time_float2ufix64_c, f_a, count_of(f_a)));
    printf("----------------- Fixed-point (Dynamic Point) 32-bit Conversions ---\n");
    printf("fix2float         %g\n", time_unary_int32_n_func(time_fix2float, i_32, n_32, count_of(i_32)));
    printf("ufix2float        %g\n", time_unary_int32_n_func(time_ufix2float, i_32, n_32, count_of(i_32)));
    printf("float2fix_z       %g\n", time_unary_n_func(time_float2fix_z, f_a, n_32, count_of(f_a)));
    printf("float2ufix_z      %g\n", time_unary_n_func(time_float2ufix_z, f_a, n_32, count_of(f_a)));
    printf("float2fix         %g\n", time_unary_n_func(time_float2fix, f_a, n_32, count_of(f_a)));
    printf("float2ufix        %g\n", time_unary_n_func(time_float2ufix, f_a, n_32, count_of(f_a)));
    printf("----------------- Fixed-point (Dynamic Point) 64-bit Conversions ---\n");
    printf("fix642float       %g\n", time_unary_int64_n_func(time_fix642float, i_64, n_32, count_of(i_64)));
    printf("ufix642float      %g\n", time_unary_int64_n_func(time_ufix642float, i_64, n_32, count_of(i_64)));
    printf("float2fix64_z     %g\n", time_unary_n_func(time_float2fix64_z, f_a, n_32, count_of(f_a)));
    printf("float2ufix64_z    %g\n", time_unary_n_func(time_float2ufix64_z, f_a, n_32, count_of(f_a)));
    printf("float2fix64       %g\n", time_unary_n_func(time_float2fix64, f_a, n_32, count_of(f_a)));
    printf("float2ufix64      %g\n", time_unary_n_func(time_float2ufix64, f_a, n_32, count_of(f_a)));

    printf("----------------- Trig (basic) ---\n");
    printf("fcos              %g\n", time_unary_func(time_fcos, f_a, count_of(f_a)));
    printf("fsin              %g\n", time_unary_func(time_fsin, f_a, count_of(f_a)));
    printf("ftan              %g\n", time_unary_func(time_ftan, f_a, count_of(f_a)));
    printf("fatan2            %g\n", time_binary_func(time_fatan2, f_a, f_b, count_of(f_a)));
    printf("fsincos           %g\n", time_unary_func(time_fsincos, f_a, count_of(f_a)));
    printf("----------------- Sci (basic) ---\n");
    printf("fexp              %g\n", time_unary_func(time_fexp, f_a, count_of(f_a)));
    printf("flog              %g\n", time_unary_func(time_flog, f_positive, count_of(f_positive)));
    printf("----------------- Misc ---\n");
    printf("fcopysign         %g\n", time_binary_func(time_fcopysign, f_a, f_b, count_of(f_a)));
    printf("ftrunc            %g\n", time_unary_func(time_ftrunc, f_a, count_of(f_a)));
    printf("ffloor            %g\n", time_unary_func(time_ffloor, f_a, count_of(f_a)));
    printf("fceil             %g\n", time_unary_func(time_fceil, f_a, count_of(f_a)));
    printf("fround            %g\n", time_unary_func(time_fround, f_a, count_of(f_a)));
    printf("ffmod             %g\n", time_binary_func(time_ffmod, f_a, f_b, count_of(f_a)));
    printf("fdrem             %g\n", time_binary_func(time_fdrem, f_a, f_b, count_of(f_a)));
    printf("fremainder        %g\n", time_binary_func(time_fremainder, f_a, f_b, count_of(f_a)));
    printf("fremquo           %g\n", time_binary_func(time_fremquo, f_a, f_b, count_of(f_a)));
    printf("----------------- Sci (extra) ---\n");
    printf("fexp2             %g\n", time_unary_func(time_fexp2, f_a, count_of(f_a)));
    printf("flog2             %g\n", time_unary_func(time_flog2, f_positive, count_of(f_positive)));
    printf("fexp10            %g\n", time_unary_func(time_fexp10, f_a, count_of(f_a)));
    printf("flog10            %g\n", time_unary_func(time_flog10, f_positive, count_of(f_positive)));
    printf("fldexp            %g\n", time_binary_int_func(time_fldexp, f_a, i_pow, count_of(f_a)));
    printf("fexpm1            %g\n", time_unary_func(time_fexpm1, f_a, count_of(f_a)));
    printf("flog1p            %g\n", time_unary_func(time_flog1p, f_positive, count_of(f_positive)));
    printf("fpowint           %g\n", time_binary_int_func(time_fpowint, f_a, i_32, count_of(f_a)));
    printf("fpow              %g\n", time_binary_func(time_fpow, f_a, f_b, count_of(f_a)));
    printf("fcbrt             %g\n", time_unary_func(time_fcbrt, f_a, count_of(f_a)));
    printf("----------------- Trig (extra) ---\n");
    printf("facos             %g\n", time_unary_func(time_facos, f_m1to1, count_of(f_m1to1)));
    printf("fasin             %g\n", time_unary_func(time_fasin, f_m1to1, count_of(f_m1to1)));
    printf("fatan             %g\n", time_unary_func(time_fatan, f_a, count_of(f_a)));
    printf("fcosh             %g\n", time_unary_func(time_fcosh, f_a, count_of(f_a)));
    printf("fsinh             %g\n", time_unary_func(time_fsinh, f_a, count_of(f_a)));
    printf("ftanh             %g\n", time_unary_func(time_ftanh, f_a, count_of(f_a)));
    printf("facosh            %g\n", time_unary_func(time_facosh, f_1plus, count_of(f_1plus)));
    printf("fasinh            %g\n", time_unary_func(time_fasinh, f_1plus, count_of(f_1plus)));
    printf("fatanh            %g\n", time_unary_func(time_fatanh, f_m1to1, count_of(f_m1to1)));
    printf("fhypot            %g\n", time_binary_func(time_fhypot, f_a, f_b, count_of(f_a)));

    printf("PASSED\n");
    return 0;
}