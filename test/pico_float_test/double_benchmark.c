#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico/double.h"
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

static double d_a[] = {1.3, -200.3, 1.6e15, 1e-2};
static double d_b[] = {-121.3, 50.3, 27.9, 1.7e23};
static double d_c[] = {20.3, -50.3, -3.9e-3, -4.1e7};
static double d_m1to1[] = {-0.5, .9999, 0.1, -0.999999};

static int32_t i_pow[] = {3,6,27,-10};
static double d_positive[] = {0.0, 3.7, 1245325., 1e27};
static double d_1plus[] = {1.0, 3.7, 1245325., 1e27};

static double d_smaller[] = {-1000.3, 200.3, 1.6e15};
static double d_bigger[] = {-121.3, 5000.3, 1.6e16};

static int32_t i_32[]  = { 0, 3, -200, INT32_MIN, INT32_MAX };
static int64_t i_64[]  = { 0, 3, -200, 0x123456789abcll, -0x123456789abcll, INT64_MIN, INT64_MAX };

// bits for fixed point conversions
static int32_t n_32[]  = { 0, 3, -3, 16, -16 };

static_assert(count_of(d_a) == count_of(d_b), "");
static_assert(count_of(d_a) == count_of(d_c), "");
static_assert(count_of(d_a) == count_of(i_pow), "");
static_assert(count_of(d_a) == count_of(d_positive), "");

static_assert(count_of(d_smaller) == count_of(d_bigger), "");

static double time_unary_func(int (*timer)(double), double *d, uint count) {
    double total = 0.f;
    for (uint i=0;i<count;i++) {
        total += (double)timer(d[i]);
    }
    return total / count;
}

static double time_unary_n_func(int (*timer)(double, int32_t), double *d, int32_t *n, uint count) {
    double total = 0.f;
    for (uint i=0;i<count;i++) {
        total += (double)timer(d[i], n[i]);
    }
    return total / count;
}

static double time_binary_func(int (*timer)(double,double), double *d1, double *d2, uint count) {
    double total = 0.f;
    for (uint i=0;i<count;i++) {
        total += (double)timer(d1[i], d2[i]);
    }
    return total / count;
}

static double time_binary_int_func(int (*timer)(double,int32_t), double *d, int32_t *i32, uint count) {
    double total = 0.f;
    for (uint i=0;i<count;i++) {
        total += (double)timer(d[i], i32[i]);
    }
    return total / count;
}

static double time_ternary_func(int (*timer)(double,double,double), double *d1, double *d2, double *d3, uint count) {
    double total = 0.f;
    for (uint i=0;i<count;i++) {
        total += (double)timer(d1[i], d2[i], d3[i]);
    }
    return total / count;
}

static double time_unary_int32_func(int (*timer)(int32_t), int32_t *i32, uint count) {
    double total = 0.f;
    for (uint i=0;i<count;i++) {
        total += (double)timer(i32[i]);
    }
    return total / count;
}

static double time_unary_int32_n_func(int (*timer)(int32_t, int32_t), int32_t *i32, int32_t *n, uint count) {
    double total = 0.f;
    for (uint i=0;i<count;i++) {
        total += (double)timer(i32[i], n[i]);
    }
    return total / count;
}

static double time_unary_int64_func(int (*timer)(int64_t), int64_t *i64, uint count) {
    double total = 0.f;
    for (uint i=0;i<count;i++) {
        total += (double)timer(i64[i]);
    }
    return total / count;
}

static double time_unary_int64_n_func(int (*timer)(int64_t, int32_t), int64_t *i64, int32_t *n, uint count) {
    double total = 0.f;
    for (uint i=0;i<count;i++) {
        total += (double)timer(i64[i], n[i]);
    }
    return total / count;
}

#if !defined(__ARM_FP) || ((__ARM_FP & 8) == 0)
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

#define DOUBLE_INPUT_COST (LOAD_COST * 2)
#define DOUBLE_OUTPUT_COST (STORE_COST * 2)
#define INT_INPUT_COST LOAD_COST
#define INT_OUTPUT_COST STORE_COST
#define INT64_INPUT_COST (LOAD_COST * 2)
#define INT64_OUTPUT_COST (STORE_COST * 2)
#define FLOAT_OUTPUT_COST STORE_COST

timer_func_def(dadd)(volatile double a, volatile double b) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = a + b;
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST * 2 - DOUBLE_OUTPUT_COST;
}

timer_func_def(dsub)(volatile double a, volatile double b) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = a - b;
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST * 2 - DOUBLE_OUTPUT_COST;
}

timer_func_def(dmul)(volatile double a, volatile double b) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = a * b;
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST * 2 - DOUBLE_OUTPUT_COST;
}

timer_func_def(ddiv)(volatile double a, volatile double b) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = a / b;
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST * 2 - DOUBLE_OUTPUT_COST;
}

timer_func_def(ddiv_fast)(volatile double a, volatile double b) {
#if PICO_DOUBLE_HAS_DDIV_FAST
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = ddiv_fast(a,b);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST * 2 - DOUBLE_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(dsqrt)(volatile double a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = sqrt(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - DOUBLE_OUTPUT_COST;
}

timer_func_def(dsqrt_fast)(volatile double a) {
#if PICO_DOUBLE_HAS_SQRT_FAST
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = sqrt_fast(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - DOUBLE_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(dfma)(volatile double a, volatile double b, volatile double c) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = fma(a, b, c);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST * 3 - DOUBLE_OUTPUT_COST;
}

timer_func_def(dfma_fast)(volatile double a, volatile double b, volatile double c) {
#if PICO_DOUBLE_HAS_FMA_FAST
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = fma_fast(a, b, c);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST * 3 - DOUBLE_OUTPUT_COST;
#else
    return -1;
#endif
}


#define DCMP_OVERHEAD 4
timer_func_def(dcmpeq)(volatile double a, volatile double b) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile bool v = a == b;
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST * 2 - DCMP_OVERHEAD;
}

timer_func_def(dcmplt)(volatile double a, volatile double b) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile bool v = a < b;
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST * 2 - DCMP_OVERHEAD;
}

timer_func_def(dcmple)(volatile double a, volatile double b) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile bool v = a <= b;
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST * 2 - DCMP_OVERHEAD;
}

timer_func_def(dcmpgt)(volatile double a, volatile double b) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile bool v = a > b;
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST * 2 - DCMP_OVERHEAD;
}

timer_func_def(dcmpge)(volatile double a, volatile double b) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile bool v = a >= b;
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST * 2 - DCMP_OVERHEAD;
}

timer_func_def(dcmpun)(volatile double a, volatile double b) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile bool v = __builtin_isunordered(a, b);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST * 2 - DCMP_OVERHEAD;
}

timer_func_def(i2d)(volatile int32_t i) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = (double)i;
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - INT_INPUT_COST - DOUBLE_OUTPUT_COST;
}

timer_func_def(ui2d)(volatile int32_t i) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = (double)(uint32_t)i;
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - INT_INPUT_COST - DOUBLE_OUTPUT_COST;
}

timer_func_def(int2double)(volatile int32_t i) {
#if PICO_DOUBLE_HAS_INT32_TO_DOUBLE_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = int2double(i);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - INT_INPUT_COST - DOUBLE_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(uint2double)(volatile int32_t i) {
#if PICO_DOUBLE_HAS_INT32_TO_DOUBLE_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = uint2double(i);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - INT_INPUT_COST - DOUBLE_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(d2i)(volatile double a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile int32_t x = (int32_t) a;
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - INT_OUTPUT_COST;
}

timer_func_def(d2ui)(volatile double a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile uint32_t x = (uint32_t) a;
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - INT_OUTPUT_COST;
}

timer_func_def(double2int_z)(volatile double a) {
#if PICO_DOUBLE_HAS_DOUBLE_TO_INT32_Z_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile int32_t x = double2int_z(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - INT_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(double2uint_z)(volatile double a) {
#if PICO_DOUBLE_HAS_DOUBLE_TO_INT32_Z_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile uint32_t x = double2uint_z(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - INT_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(double2int)(volatile double a) {
#if PICO_DOUBLE_HAS_DOUBLE_TO_INT32_M_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile int32_t x = double2int(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - INT_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(double2uint)(volatile double a) {
#if PICO_DOUBLE_HAS_DOUBLE_TO_INT32_M_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile uint32_t x = double2uint(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - INT_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(l2d)(volatile int64_t i) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = (double)i;
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - INT64_INPUT_COST - DOUBLE_OUTPUT_COST;
}

timer_func_def(ul2d)(volatile int64_t i) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = (double)(uint64_t)i;
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - INT64_INPUT_COST - DOUBLE_OUTPUT_COST;
}

timer_func_def(int642double)(volatile int64_t i) {
#if PICO_DOUBLE_HAS_INT64_TO_DOUBLE_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = int642double(i);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - INT64_INPUT_COST - DOUBLE_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(uint642double)(volatile int64_t i) {
#if PICO_DOUBLE_HAS_INT64_TO_DOUBLE_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = uint642double(i);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - INT64_INPUT_COST - DOUBLE_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(d2l)(volatile double a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile int64_t x = (int64_t) a;
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - INT64_OUTPUT_COST;
}

timer_func_def(d2ul)(volatile double a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile uint64_t x = (uint64_t) a;
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - INT64_OUTPUT_COST;
}

timer_func_def(double2int64_z)(volatile double a) {
#if PICO_DOUBLE_HAS_DOUBLE_TO_INT64_Z_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile int64_t x = double2int64_z(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - INT64_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(double2uint64_z)(volatile double a) {
#if PICO_DOUBLE_HAS_DOUBLE_TO_INT64_Z_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile uint64_t x = double2uint64_z(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - INT64_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(double2int64)(volatile double a) {
#if PICO_DOUBLE_HAS_DOUBLE_TO_INT64_M_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile int64_t x = double2int64(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - INT64_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(double2uint64)(volatile double a) {
#if PICO_DOUBLE_HAS_DOUBLE_TO_INT64_M_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile uint64_t x = double2uint64(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - INT64_OUTPUT_COST;
#else
    return -1;
#endif
}

// ----------------------

timer_func_def(d2f)(volatile double a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile float x = (float) a;
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - FLOAT_OUTPUT_COST;
}

// ----------------------

timer_func_def(fix2double)(volatile int32_t i, volatile int32_t nn) {
#if PICO_DOUBLE_HAS_FIX32_TO_DOUBLE_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    int32_t n = nn; pico_default_asm_volatile( "" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile double x = fix2double(i, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - INT_INPUT_COST - DOUBLE_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(ufix2double)(volatile int32_t i, volatile int32_t nn) {
#if PICO_DOUBLE_HAS_FIX32_TO_DOUBLE_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    int32_t n = nn; pico_default_asm_volatile( "" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile double x = ufix2double(i, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - INT_INPUT_COST - DOUBLE_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(double2fix_z)(volatile double a, volatile int32_t nn) {
#if PICO_DOUBLE_HAS_DOUBLE_TO_FIX32_Z_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    int n = nn; pico_default_asm_volatile( "" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile int32_t x = double2fix_z(a, nn);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - INT_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(double2ufix_z)(volatile double a, volatile int32_t nn) {
#if PICO_DOUBLE_HAS_DOUBLE_TO_FIX32_Z_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    int32_t n = nn; pico_default_asm_volatile( "" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile uint32_t x = double2ufix_z(a, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - INT_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(double2fix)(volatile double a, volatile int32_t nn) {
#if PICO_DOUBLE_HAS_DOUBLE_TO_FIX32_M_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    int32_t n = nn; pico_default_asm_volatile( "" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile int32_t x = double2fix(a, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - INT_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(double2ufix)(volatile double a, volatile int32_t nn) {
#if PICO_DOUBLE_HAS_DOUBLE_TO_FIX32_M_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    int32_t n = nn; pico_default_asm_volatile( "" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile uint32_t x = double2ufix(a, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - INT_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(fix642double)(volatile int64_t i, volatile int32_t nn) {
#if PICO_DOUBLE_HAS_FIX64_TO_DOUBLE_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    int32_t n = nn; pico_default_asm_volatile( "" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile double x = fix642double(i, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - INT64_INPUT_COST - DOUBLE_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(ufix642double)(volatile int64_t i, volatile int32_t nn) {
#if PICO_DOUBLE_HAS_FIX64_TO_DOUBLE_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    int32_t n = nn; pico_default_asm_volatile( "" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile double x = ufix642double(i, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - INT64_INPUT_COST - DOUBLE_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(double2fix64_z)(volatile double a, volatile int32_t nn) {
#if PICO_DOUBLE_HAS_DOUBLE_TO_FIX64_Z_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    int32_t n = nn; pico_default_asm_volatile( "" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile int64_t x = double2fix64_z(a, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - INT64_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(double2ufix64_z)(volatile double a, volatile int32_t nn) {
#if PICO_DOUBLE_HAS_DOUBLE_TO_FIX64_Z_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    int32_t n = nn; pico_default_asm_volatile( "" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile uint64_t x = double2ufix64_z(a, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - INT64_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(double2fix64)(volatile double a, volatile int32_t nn) {
#if PICO_DOUBLE_HAS_DOUBLE_TO_FIX64_M_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    int32_t n = nn; pico_default_asm_volatile( "" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile int64_t x = double2fix64(a, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - INT64_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(double2ufix64)(volatile double a, volatile int32_t nn) {
#if PICO_DOUBLE_HAS_DOUBLE_TO_FIX64_M_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    int32_t n = nn; pico_default_asm_volatile( "" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile uint64_t x = double2ufix64(a, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - INT64_OUTPUT_COST;
#else
    return -1;
#endif
}

// ----------------------

timer_func_def(fix2double_c)(volatile int32_t i) {
#if PICO_DOUBLE_HAS_FIX32_TO_DOUBLE_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    const int n = 16; pico_default_asm_volatile("" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile double x = fix2double(i, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - INT_INPUT_COST - DOUBLE_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(ufix2double_c)(volatile int32_t i) {
#if PICO_DOUBLE_HAS_FIX32_TO_DOUBLE_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    const int n = 16; pico_default_asm_volatile("" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile double x = ufix2double(i, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - INT_INPUT_COST - DOUBLE_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(double2fix_z_c)(volatile double a) {
#if PICO_DOUBLE_HAS_DOUBLE_TO_FIX32_Z_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    const int n = 16; pico_default_asm_volatile("" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile int32_t x = double2fix_z(a, 16);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - INT_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(double2ufix_z_c)(volatile double a) {
#if PICO_DOUBLE_HAS_DOUBLE_TO_FIX32_Z_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    const int n = 16; pico_default_asm_volatile("" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile uint32_t x = double2ufix_z(a, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - INT_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(double2fix_c)(volatile double a) {
#if PICO_DOUBLE_HAS_DOUBLE_TO_FIX32_M_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    const int n = 16; pico_default_asm_volatile("" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile int32_t x = double2fix(a, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - INT_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(double2ufix_c)(volatile double a) {
#if PICO_DOUBLE_HAS_DOUBLE_TO_FIX32_M_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    const int n = 16; pico_default_asm_volatile("" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile uint32_t x = double2ufix(a, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - INT_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(fix642double_c)(volatile int64_t i) {
#if PICO_DOUBLE_HAS_FIX64_TO_DOUBLE_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    const int n = 16; pico_default_asm_volatile("" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile double x = fix642double(i, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - INT64_INPUT_COST - DOUBLE_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(ufix642double_c)(volatile int64_t i) {
#if PICO_DOUBLE_HAS_FIX64_TO_DOUBLE_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    const int n = 16; pico_default_asm_volatile("" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile double x = ufix642double(i, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - INT64_INPUT_COST - DOUBLE_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(double2fix64_z_c)(volatile double a) {
#if PICO_DOUBLE_HAS_DOUBLE_TO_FIX64_Z_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    const int n = 16; pico_default_asm_volatile("" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile int64_t x = double2fix64_z(a, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - INT64_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(double2ufix64_z_c)(volatile double a) {
#if PICO_DOUBLE_HAS_DOUBLE_TO_FIX64_Z_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    const int n = 16; pico_default_asm_volatile("" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile uint64_t x = double2ufix64_z(a, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - INT64_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(double2fix64_c)(volatile double a) {
#if PICO_DOUBLE_HAS_DOUBLE_TO_FIX64_M_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    const int n = 16; pico_default_asm_volatile("" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile int64_t x = double2fix64(a, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - INT64_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(double2ufix64_c)(volatile double a) {
#if PICO_DOUBLE_HAS_DOUBLE_TO_FIX64_M_CONVERSIONS
    register io_ro_32 *systick_ptr = systick_value_ptr();
    const int n = 16; pico_default_asm_volatile("" : : "r" (n) : "memory");
    uint32_t t0 = *systick_ptr;
    volatile uint64_t x = double2ufix64(a, n);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - INT64_OUTPUT_COST;
#else
    return -1;
#endif
}

// ----------------------

timer_func_def(dcos)(volatile double a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = cos(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - DOUBLE_OUTPUT_COST;
}

timer_func_def(dsin)(volatile double a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = sin(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - DOUBLE_OUTPUT_COST;
}

timer_func_def(dsincos)(volatile double a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    double s, c;
    sincos(a, &s, &c);
    volatile double x = s;
    volatile double y = c;
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - DOUBLE_OUTPUT_COST * 2;
}

timer_func_def(dtan)(volatile double a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = tan(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - DOUBLE_OUTPUT_COST;
}

timer_func_def(datan2)(volatile double a, volatile double b) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = atan2(a, b);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST * 2 - DOUBLE_OUTPUT_COST;
}

timer_func_def(dexp)(volatile double a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = exp(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - DOUBLE_OUTPUT_COST;
}

timer_func_def(dlog)(volatile double a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = log(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - DOUBLE_OUTPUT_COST;
}

timer_func_def(dpowint)(volatile double a, int32_t pow) {
#if PICO_DOUBLE_HAS_POWINT
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = powint(a, pow);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - DOUBLE_OUTPUT_COST;
#else
    return -1;
#endif
}

timer_func_def(dcopysign)(volatile double a, volatile double b) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = copysign(a, b);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST * 2 - DOUBLE_OUTPUT_COST;
}

timer_func_def(dtrunc)(volatile double a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = trunc(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - DOUBLE_OUTPUT_COST;
}

timer_func_def(dfloor)(volatile double a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = floor(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - DOUBLE_OUTPUT_COST;
}

timer_func_def(dceil)(volatile double a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = ceil(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - DOUBLE_OUTPUT_COST;
}

timer_func_def(dround)(volatile double a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = round(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - DOUBLE_OUTPUT_COST;
}

timer_func_def(dfmod)(volatile double a, volatile double b) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = fmod(a, b);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST * 2 - DOUBLE_OUTPUT_COST;
}

timer_func_def(ddrem)(volatile double a, volatile double b) {
    // LLVM libc is string betting the floating point functions
#if defined(__LLVM_LIBC__) && defined(__llvm__) // && (__clang_major__ < 23)
    return -1;
#else
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = drem(a, b);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST * 2 - DOUBLE_OUTPUT_COST;
#endif
}

timer_func_def(dremainder)(volatile double a, volatile double b) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = remainder(a, b);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST * 2 - DOUBLE_OUTPUT_COST;
}

timer_func_def(dremquo)(volatile double a, volatile double b) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    int c;
    uint32_t t0 = *systick_ptr;
    volatile double x = remquo(a, b, &c);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST * 2 - DOUBLE_OUTPUT_COST;
}

timer_func_def(dexp2)(volatile double a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = exp2(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - DOUBLE_OUTPUT_COST;
}

timer_func_def(dlog2)(volatile double a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = log2(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - DOUBLE_OUTPUT_COST;
}

timer_func_def(dexp10)(volatile double a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = exp10(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - DOUBLE_OUTPUT_COST;
}

timer_func_def(dlog10)(volatile double a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = log10(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - DOUBLE_OUTPUT_COST;
}

timer_func_def(dldexp)(volatile double a, int32_t b) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = ldexp(a, b);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - DOUBLE_OUTPUT_COST;
}

timer_func_def(dexpm1)(volatile double a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = expm1(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - DOUBLE_OUTPUT_COST;
}

timer_func_def(dlog1p)(volatile double a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = log1p(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - DOUBLE_OUTPUT_COST;
}

timer_func_def(dpow)(volatile double a, volatile double b) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = pow(a, b);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST * 2 - DOUBLE_OUTPUT_COST;
}

timer_func_def(dcbrt)(volatile double a) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = cbrt(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - DOUBLE_OUTPUT_COST;
}

timer_func_def(dacosh)(volatile double a) {
    // LLVM libc is string betting the floating point functions
#if defined(__LLVM_LIBC__) && defined(__llvm__) // && (__clang_major__ < 21)
    return -1;
#else
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = acosh(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - DOUBLE_OUTPUT_COST;
#endif
}

timer_func_def(datanh)(volatile double a) {
    // LLVM libc is string betting the floating point functions
#if defined(__LLVM_LIBC__) && defined(__llvm__) // && (__clang_major__ < 21)
    return -1;
#else
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = atanh(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - DOUBLE_OUTPUT_COST;
#endif
}

timer_func_def(dhypot)(volatile double a, volatile double b) {
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = hypot(a, b);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST * 2 - DOUBLE_OUTPUT_COST;
}

timer_func_def(dasin)(volatile double a) {
    // LLVM libc is string betting the floating point functions
#if defined(__LLVM_LIBC__) && defined(__llvm__) && (__clang_major__ < 23)
    return -1;
#else
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = asin(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - DOUBLE_OUTPUT_COST;
#endif
}

timer_func_def(dacos)(volatile double a) {
    // LLVM libc is string betting the floating point functions
#if defined(__LLVM_LIBC__) && defined(__llvm__) && (__clang_major__ < 23)
    return -1;
#else
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = acos(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - DOUBLE_OUTPUT_COST;
#endif
}

timer_func_def(datan)(volatile double a) {
    // LLVM libc is string betting the floating point functions
#if defined(__LLVM_LIBC__) && defined(__llvm__) && (__clang_major__ < 23)
    return -1;
#else
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = atan(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - DOUBLE_OUTPUT_COST;
#endif
}

timer_func_def(dsinh)(volatile double a) {
    // LLVM libc is string betting the floating point functions
#if defined(__LLVM_LIBC__) && defined(__llvm__) // && (__clang_major__ < 21)
    return -1;
#else
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = sinh(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - DOUBLE_OUTPUT_COST;
#endif
}

timer_func_def(dcosh)(volatile double a) {
    // LLVM libc is string betting the floating point functions
#if defined(__LLVM_LIBC__) && defined(__llvm__) // && (__clang_major__ < 21)
    return -1;
#else
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = cosh(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - DOUBLE_OUTPUT_COST;
#endif
}

timer_func_def(dtanh)(volatile double a) {
    // LLVM libc is string betting the floating point functions
#if defined(__LLVM_LIBC__) && defined(__llvm__) // && (__clang_major__ < 21)
    return -1;
#else
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = tanh(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - DOUBLE_OUTPUT_COST;
#endif
}

timer_func_def(dasinh)(volatile double a) {
    // LLVM libc is string betting the floating point functions
#if defined(__LLVM_LIBC__) && defined(__llvm__) // && (__clang_major__ < 21)
    return -1;
#else
    register io_ro_32 *systick_ptr = systick_value_ptr();
    uint32_t t0 = *systick_ptr;
    volatile double x = asinh(a);
    uint32_t t1 = *systick_ptr;
    return cycle_diff(t0, t1) - DOUBLE_INPUT_COST - DOUBLE_OUTPUT_COST;
#endif
}

int main() {
    init_systick();
    stdio_init_all();
#if PICO_C_COMPILER_IS_CLANG
    printf("================= Clang - ");
#else
    printf("================ GCC - ");
#endif
#if LIB_PICO_DOUBLE_COMPILER
    printf("COMPILER ===\n");
#elif LIB_PICO_DOUBLE_PICO
    printf("PICO ===\n");
#elif LIB_PICO_DOUBLE_PICO_VFP
    printf("PICO VFP ===\n");
#elif LIB_PICO_DOUBLE_PICO_DCP
    printf("PICO DCP ===\n");
#else
#error unknown float impl
#endif
#if EMITS_VFP
    printf("hard-double       true\n");
#else
    printf("hard-double       false\n");
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
    printf("add               %g\n", time_binary_func(time_dadd, d_a, d_b, count_of(d_a)));
    printf("sub               %g\n", time_binary_func(time_dsub, d_a, d_b, count_of(d_a)));
    printf("mul               %g\n", time_binary_func(time_dmul, d_a, d_b, count_of(d_a)));
    printf("div               %g\n", time_binary_func(time_ddiv, d_a, d_b, count_of(d_a)));
    printf("sqrt              %g\n", time_unary_func(time_dsqrt, d_positive, count_of(d_a)));
    printf("fma               %g\n", time_ternary_func(time_dfma, d_a, d_b, d_c, count_of(d_a)));
    printf("div_fast          %g\n", time_binary_func(time_ddiv_fast, d_a, d_b, count_of(d_a)));
    printf("sqrt_fast         %g\n", time_unary_func(time_dsqrt_fast, d_positive, count_of(d_a)));
    printf("fma_fast          %g\n", time_ternary_func(time_dfma_fast, d_a, d_b, d_c, count_of(d_a)));
    printf("----------------- Comparison ---\n");
    // these are hard to make the compiler generate it seems
    // printf("ccmpeq             %g\n", time_binary_func(time_dccmpeq, d_a, d_b, count_of(d_a)));
    // printf("crcmple            %g\n", time_binary_func(time_dcrcmple, d_a, d_b, count_of(d_a)));
    // printf("ccmple             %g\n", time_binary_func(time_dccmple, d_a, d_b, count_of(d_a)));
    printf("cmpeq             %g\n", time_binary_func(time_dcmpeq, d_a, d_a, count_of(d_a)));
    printf("cmplt             %g\n", time_binary_func(time_dcmplt, d_smaller, d_bigger, count_of(d_smaller)));
    printf("cmple             %g\n", time_binary_func(time_dcmple, d_smaller, d_bigger, count_of(d_smaller)));
    printf("cmpge             %g\n", time_binary_func(time_dcmpge, d_bigger, d_smaller, count_of(d_bigger)));
    printf("cmpgt             %g\n", time_binary_func(time_dcmpgt, d_bigger, d_smaller, count_of(d_bigger)));
    printf("cmpun             %g\n", time_binary_func(time_dcmpun, d_a, d_a, count_of(d_a)));
    printf("----------------- 32-bit Conversions ---\n");
    printf("i2d               %g\n", time_unary_int32_func(time_i2d, i_32, count_of(i_32)));
    printf("ui2d              %g\n", time_unary_int32_func(time_ui2d, i_32, count_of(i_32)));
    printf("int2double        %g\n", time_unary_int32_func(time_int2double, i_32, count_of(i_32)));
    printf("uint2double       %g\n", time_unary_int32_func(time_uint2double, i_32, count_of(i_32)));
    printf("d2i               %g\n", time_unary_func(time_d2i, d_a, count_of(d_a)));
    printf("d2ui              %g\n", time_unary_func(time_d2ui, d_a, count_of(d_a)));
    printf("double2int_z      %g\n", time_unary_func(time_double2int_z, d_a, count_of(d_a)));
    printf("double2uint_z     %g\n", time_unary_func(time_double2uint_z, d_a, count_of(d_a)));
    printf("double2int        %g\n", time_unary_func(time_double2int, d_a, count_of(d_a)));
    printf("double2uint       %g\n", time_unary_func(time_double2uint, d_a, count_of(d_a)));
    printf("----------------- 64-bit Conversions ---\n");
    printf("l2d               %g\n", time_unary_int64_func(time_l2d, i_64, count_of(i_64)));
    printf("ul2d              %g\n", time_unary_int64_func(time_ul2d, i_64, count_of(i_64)));
    printf("int642double      %g\n", time_unary_int64_func(time_int642double, i_64, count_of(i_64)));
    printf("uint642double     %g\n", time_unary_int64_func(time_uint642double, i_64, count_of(i_64)));
    printf("d2l               %g\n", time_unary_func(time_d2l, d_a, count_of(d_a)));
    printf("d2ul              %g\n", time_unary_func(time_d2ul, d_a, count_of(d_a)));
    printf("double2int64_z    %g\n", time_unary_func(time_double2int64_z, d_a, count_of(d_a)));
    printf("double2uint64_z   %g\n", time_unary_func(time_double2uint64_z, d_a, count_of(d_a)));
    printf("double2int64      %g\n", time_unary_func(time_double2int64, d_a, count_of(d_a)));
    printf("double2uint64     %g\n", time_unary_func(time_double2uint64, d_a, count_of(d_a)));
    printf("d2f               %g\n", time_unary_func(time_d2f, d_a, count_of(d_a)));
    printf("----------------- Fixed-point (Constant Point) 32-bit Conversions ---\n");
    printf("fix2double_c      %g\n", time_unary_int32_func(time_fix2double_c, i_32, count_of(i_32)));
    printf("ufix2double_c     %g\n", time_unary_int32_func(time_ufix2double_c, i_32, count_of(i_32)));
    printf("double2fix_z_c    %g\n", time_unary_func(time_double2fix_z_c, d_a, count_of(d_a)));
    printf("double2ufix_z_c   %g\n", time_unary_func(time_double2ufix_z_c, d_a, count_of(d_a)));
    printf("double2fix_c      %g\n", time_unary_func(time_double2fix_c, d_a, count_of(d_a)));
    printf("double2ufix_c     %g\n", time_unary_func(time_double2ufix_c, d_a, count_of(d_a)));
    printf("----------------- Fixed-point (Constant Point) 64-bit Conversions ---\n");
    printf("fix642double_c    %g\n", time_unary_int64_func(time_fix642double_c, i_64, count_of(i_64)));
    printf("ufix642double_c   %g\n", time_unary_int64_func(time_ufix642double_c, i_64, count_of(i_64)));
    printf("double2fix64_z_c  %g\n", time_unary_func(time_double2fix64_z_c, d_a, count_of(d_a)));
    printf("double2ufix64_z_c %g\n", time_unary_func(time_double2ufix64_z_c, d_a, count_of(d_a)));
    printf("double2fix64_c    %g\n", time_unary_func(time_double2fix64_c, d_a, count_of(d_a)));
    printf("double2ufix64_c   %g\n", time_unary_func(time_double2ufix64_c, d_a, count_of(d_a)));
    printf("----------------- Fixed-point (Dynamic Point) 32-bit Conversions ---\n");
    printf("fix2double        %g\n", time_unary_int32_n_func(time_fix2double, i_32, n_32, count_of(i_32)));
    printf("ufix2double       %g\n", time_unary_int32_n_func(time_ufix2double, i_32, n_32, count_of(i_32)));
    printf("double2fix_z      %g\n", time_unary_n_func(time_double2fix_z, d_a, n_32, count_of(d_a)));
    printf("double2ufix_z     %g\n", time_unary_n_func(time_double2ufix_z, d_a, n_32, count_of(d_a)));
    printf("double2fix        %g\n", time_unary_n_func(time_double2fix, d_a, n_32, count_of(d_a)));
    printf("double2ufix       %g\n", time_unary_n_func(time_double2ufix, d_a, n_32, count_of(d_a)));
    printf("----------------- Fixed-point (Dynamic Point) 64-bit Conversions ---\n");
    printf("fix642double      %g\n", time_unary_int64_n_func(time_fix642double, i_64, n_32, count_of(i_64)));
    printf("ufix642double     %g\n", time_unary_int64_n_func(time_ufix642double, i_64, n_32, count_of(i_64)));
    printf("double2fix64_z    %g\n", time_unary_n_func(time_double2fix64_z, d_a, n_32, count_of(d_a)));
    printf("double2ufix64_z   %g\n", time_unary_n_func(time_double2ufix64_z, d_a, n_32, count_of(d_a)));
    printf("double2fix64      %g\n", time_unary_n_func(time_double2fix64, d_a, n_32, count_of(d_a)));
    printf("double2ufix64     %g\n", time_unary_n_func(time_double2ufix64, d_a, n_32, count_of(d_a)));
    
    printf("----------------- Trig (basic) ---\n");
    printf("cos               %g\n", time_unary_func(time_dcos, d_positive, count_of(d_a)));
    printf("sin               %g\n", time_unary_func(time_dsin, d_positive, count_of(d_a)));
    printf("tan               %g\n", time_unary_func(time_dtan, d_positive, count_of(d_a)));
    printf("atan2             %g\n", time_binary_func(time_datan2, d_a, d_b, count_of(d_a)));
    printf("sincos            %g\n", time_unary_func(time_dsincos, d_positive, count_of(d_a)));
    printf("----------------- Sci (basic) ---\n");
    printf("dexp              %g\n", time_unary_func(time_dexp, d_a, count_of(d_a)));
    printf("dlog              %g\n", time_unary_func(time_dlog, d_positive, count_of(d_positive)));
    printf("----------------- Misc ---\n");
    printf("dcopysign         %g\n", time_binary_func(time_dcopysign, d_a, d_b, count_of(d_a)));
    printf("dtrunc            %g\n", time_unary_func(time_dtrunc, d_a, count_of(d_a)));
    printf("dfloor            %g\n", time_unary_func(time_dfloor, d_a, count_of(d_a)));
    printf("dceil             %g\n", time_unary_func(time_dceil, d_a, count_of(d_a)));
    printf("dround            %g\n", time_unary_func(time_dround, d_a, count_of(d_a)));
    printf("dfmod             %g\n", time_binary_func(time_dfmod, d_a, d_b, count_of(d_a)));
    printf("ddrem             %g\n", time_binary_func(time_ddrem, d_a, d_b, count_of(d_a)));
    printf("dremainder        %g\n", time_binary_func(time_dremainder, d_a, d_b, count_of(d_a)));
    printf("dremquo           %g\n", time_binary_func(time_dremquo, d_a, d_b, count_of(d_a)));
    printf("----------------- Sci (extra) ---\n");
    printf("dexp2             %g\n", time_unary_func(time_dexp2, d_a, count_of(d_a)));
    printf("dlog2             %g\n", time_unary_func(time_dlog2, d_positive, count_of(d_positive)));
    printf("dexp10            %g\n", time_unary_func(time_dexp10, d_a, count_of(d_a)));
    printf("dlog10            %g\n", time_unary_func(time_dlog10, d_positive, count_of(d_positive)));
    printf("dldexp            %g\n", time_binary_int_func(time_dldexp, d_a, i_pow, count_of(d_a)));
    printf("dexpm1            %g\n", time_unary_func(time_dexpm1, d_a, count_of(d_a)));
    printf("dlog1p            %g\n", time_unary_func(time_dlog1p, d_positive, count_of(d_positive)));
    printf("dpowint           %g\n", time_binary_int_func(time_dpowint, d_a, i_32, count_of(d_a)));
    printf("dpow              %g\n", time_binary_func(time_dpow, d_a, d_b, count_of(d_a)));
    printf("dcbrt             %g\n", time_unary_func(time_dcbrt, d_a, count_of(d_a)));
    printf("----------------- Trig (extra) ---\n");
    printf("dacos             %g\n", time_unary_func(time_dacos, d_m1to1, count_of(d_m1to1)));
    printf("dasin             %g\n", time_unary_func(time_dasin, d_m1to1, count_of(d_m1to1)));
    printf("datan             %g\n", time_unary_func(time_datan, d_a, count_of(d_a)));
    printf("dcosh             %g\n", time_unary_func(time_dcosh, d_a, count_of(d_a)));
    printf("dsinh             %g\n", time_unary_func(time_dsinh, d_a, count_of(d_a)));
    printf("dtanh             %g\n", time_unary_func(time_dtanh, d_a, count_of(d_a)));
    printf("dacosh            %g\n", time_unary_func(time_dacosh, d_1plus, count_of(d_1plus)));
    printf("dasinh            %g\n", time_unary_func(time_dasinh, d_1plus, count_of(d_1plus)));
    printf("datanh            %g\n", time_unary_func(time_datanh, d_m1to1, count_of(d_m1to1)));
    printf("dhypot            %g\n", time_binary_func(time_dhypot, d_a, d_b, count_of(d_a)));

    printf("PASSED\n");
    return 0;
}