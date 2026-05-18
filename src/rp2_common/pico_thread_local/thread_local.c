/*
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <string.h>
#include <stdlib.h>
#include "pico.h"
#include <picotls.h>
#include "pico/thread_local.h"
#include "pico/runtime_init.h"
#include "hardware/sync.h"

#if PICO_THREAD_LOCAL_MODE_PER_THREAD + PICO_THREAD_LOCAL_MODE_GLOBAL + PICO_THREAD_LOCAL_MODE_NONE > 1
#error Only one of PICO_THREAD_LOCAL_MODE_PER_THREAD and PICO_THREAD_LOCAL_MODE_GLOBAL and PICO_THREAD_LOCAL_MODE_NONE may be specified
#endif

#if !PICO_THREAD_LOCAL_MODE_PER_THREAD
// if not using per-thread mode we use tdata and tbss directly, so don't waste space on them.
// this is a bit of a hacky way of telling the linker this info!
char __used __attribute__((section(".tlsX_not_needed_marker"))) _tlsX_not_needed_marker;
#endif

#if PICO_THREAD_LOCAL_MODE_PER_THREAD
// ------------------------------------------------------------
// Proper TLS support per thread (PICO_THREAD_LOCAL_MODE_PER_THREAD = 1)
// -------------------------------------------------------------

#if !PICO_THREAD_LOCAL_THREAD_POINTER_VIA_RISCV_REG
// we don't have TP register so need to track one tls region per core
static_assert(NUM_CORES <= 2, "");

// per core pointers (not needed on RISC-V as we use TP reg)
static void *__tls_adjusted_by_core[2];

#if PICO_THREAD_LOCAL_SUPPORT_THREAD_POINTER && !__riscv
/* The size of the thread control block.
 * TLS relocations are generated relative to
 * a location this far *before* the first thread
 * variable (!)
 * NB: The actual size before tp also includes padding
 * to align up to the alignment of .tdata/.tbss.
 */
extern uint8_t __arm32_tls_tcb_offset;
#define TLS_ADJUST ((size_t)&__arm32_tls_tcb_offset)
#endif
#endif

#ifndef TLS_ADJUST
#define TLS_ADJUST 0
#endif

// Note our emutls support is only inside of PICO_THREAD_LOCAL_MODE_PER_THREAD as
// the library version suffices otherwise
#if PICO_THREAD_LOCAL_SUPPORT_EMUTLS
// From emutls.c:
// 'For every TLS variable xyz, there is one __emutls_control variable named __emutls_v.xyz. If xyz has
// non-zero initial value, __emutls_v.xyz's "value" will point to __emutls_t.xyz, which has the initial value.'
//
// The linker script groups all the __emutls_v.xyz variables into a single array and provides symbols
// __emutls_array_start and __emutls_array_end, which can be used to iterate over the array. This allows
// the storage for each core's thread local variables to be pre-allocated and pre-initialized, which leaves
// minimal work for __wrap___emutls_get_address.
//
// This array is available to other TLS implementations too, such as a TLS implementation for an RTOS.

// Same layout as libgcc __emutls_object. Unfortunately, __emutls_object doesn't appear in any header files.
typedef struct {
    uint32_t size;
    uint32_t align;
    uint32_t offset;
    void *tplate;
} tls_object_t;

extern __weak tls_object_t __emutls_array_start;
extern __weak tls_object_t __emutls_array_end;

static volatile bool emutls_one_time_init_done;
static uint32_t _emutls_size;
static uint32_t _emutls_align;
#endif

#if PICO_THREAD_LOCAL_PROVIDE_INIT_TLS
// fill a linear region with data from the tls metadata (either emutls objects or tdata/tbss)
static inline void _tls_init_from_emutls_or_tdata(void *tls) {
#if PICO_THREAD_LOCAL_SUPPORT_EMUTLS
    uint8_t *tls_adjusted = ((uint8_t *)tls) - TLS_ADJUST;
    for (tls_object_t* tls_obj = &__emutls_array_start; tls_obj < &__emutls_array_end; ++tls_obj) {
        if (tls_obj->tplate) {
            memcpy(tls_adjusted + tls_obj->offset, tls_obj->tplate, tls_obj->size);
        } else {
            memset(tls_adjusted + tls_obj->offset, 0, tls_obj->size);
        }
    }
#endif
#if PICO_THREAD_LOCAL_SUPPORT_THREAD_POINTER
    // when using thread pointers we expect data to come from tdata/tbss
    extern __weak uint8_t __tdata_source[];
    extern __weak uint8_t __tdata_size[];
    extern __weak uint8_t __tbss_size[];

    uint8_t *tdata_dest = ((uint8_t *)tls);
    uint8_t *tbss_dest  = ((uint8_t *)tls) + (size_t)__tdata_size;

    /* Copy initialized TLS data from the template */
    memcpy(tdata_dest, __tdata_source, (size_t)__tdata_size);

    /* Zero the uninitialized TLS data (.tbss) */
    memset(tbss_dest, 0, (size_t)__tbss_size);
#endif
}
#define _INIT_TLS_IMPL _tls_init_from_emutls_or_tdata
#endif

#if PICO_THREAD_LOCAL_PROVIDE_SET_TLS
static inline void _set_tls_per_thread(void *tls) {
    assert(tls); // we should never be setting 0
#if !PICO_THREAD_LOCAL_THREAD_POINTER_VIA_RISCV_REG
    __tls_adjusted_by_core[get_core_num()] = (uint8_t *)tls - TLS_ADJUST;
#else
    pico_default_asm_volatile("mv tp, %0\n" : : "r" (tls));
#endif
}
#define _SET_TLS_IMPL _set_tls_per_thread
#endif

#if PICO_THREAD_LOCAL_SUPPORT_THREAD_POINTER
static __used void *_init_core_local_tls(void) {
    /* Initialized by the linker, one per core */
    extern uint8_t __tls0_base[], __tls1_base[];
    static void * const __tls_bases[2] = { __tls0_base, __tls1_base };
    void *tls = __tls_bases[get_core_num()];
    _init_tls(tls);
    _set_tls(tls);
    return tls - TLS_ADJUST;
}

#if PICO_THREAD_LOCAL_THREAD_POINTER_VIA_ARM_EABI
uint32_t __attribute__((naked)) __aeabi_read_tp(void) {
#if !__ARM_ARCH_6M__
    pico_default_asm_volatile(
        "push {r1,lr}           /* Save R1 (and LR) */\n"
        "ldr r1,=0xd0000000     /* Address of SIO->CPUID */\n"
        "ldr r1,[r1]            /* Fetch active core */\n"
        "ldr r0,=%0             /* Address of __tls array */\n"
        "ldr r0,[r0,r1,lsl #2]         /* Fetch __tls[CPUID] */\n"
        "cbz r0, 1f\n"
        "pop {r1,pc}\n"           /* Restore R1 and return  */
        "1:\n"
        "pop {r1}\n"
        "push {r1-r3}\n"
        "bl _init_core_local_tls\n"
        "pop {r1-r3,pc}\n"
        : : "i" (__tls_adjusted_by_core) : "ip", "cc"
    );
#else
    pico_default_asm_volatile(
        "push {r1,lr}           /* Save R1 (and LR) */\n"
        "ldr r1,=0xd0000000     /* Address of SIO->CPUID */\n"
        "ldr r1,[r1]            /* Fetch active core */\n"
        "ldr r0,=%0             /* Address of __tls array */\n"
        "lsls r1, #2\n"
        "ldr r0,[r0,r1]         /* Fetch __tls[CPUID] */\n"
        "cmp r0, #0\n"
        "beq 1f\n"
        "pop {r1,pc}\n"           /* Restore R1 and return  */
        "1:\n"
        "pop {r1}\n"
        "push {r1-r3}\n"
        "bl _init_core_local_tls\n"
        "pop {r1-r3,pc}\n"
        : : "i" (__tls_adjusted_by_core) : "ip", "cc"
    );
#endif
}
#endif
#endif

#if PICO_THREAD_LOCAL_SUPPORT_EMUTLS
static inline void *_get_tls_adjusted_for_core(uint core_num) {
#if !PICO_THREAD_LOCAL_THREAD_POINTER_VIA_RISCV_REG
    return __tls_adjusted_by_core[core_num];
#else
#error unsupported tls configuration // we haven't seen this in the wild so error for now
    // ((void)core_num);
    // void *tls;
    // pico_default_asm_volatile("mv %0, tp\n" : "+r" (tls));
    // return tls;
#endif
}

// This is called lazily (either as a result of __emutls_get_address
// or someone (perhaps RTOS) calling _runtime_tls_size first) to figure out
// the size of the TLS area per thread
static void _emutls_one_time_init(void) {
    // Use spinlock so we don't add dependency on mutex code - we don't call anything
    spin_lock_t *lock = spin_lock_instance(PICO_SPINLOCK_ID_HARDWARE_CLAIM);
    uint32_t save = spin_lock_blocking(lock);
    if (!emutls_one_time_init_done) {
        // Calculate the offset of each thread local variable and the total storage to be allocated for each thread.
        assert(!_emutls_size);
        _emutls_align = 1;
        for (tls_object_t* tls_obj = &__emutls_array_start; tls_obj < &__emutls_array_end; ++tls_obj) {
            assert((tls_obj->align & (tls_obj->align - 1)) == 0);

            if (tls_obj->align > _emutls_align) {
                _emutls_align = tls_obj->align;
            }

            _emutls_size = (_emutls_size + tls_obj->align - 1) & ~(tls_obj->align - 1);
            tls_obj->offset = _emutls_size + TLS_ADJUST;
            _emutls_size += tls_obj->size;
        }
        emutls_one_time_init_done = true;
    }
    spin_unlock(lock, save);
}

// When we support EMUTLS we have _tls_size() redirect here (from our replacement <picotls.h>)
size_t _runtime_tls_size(void) {
    static_assert(PICO_THREAD_LOCAL_SUPPORT_EMUTLS, ""); // this function is only provided in this case
    if (!emutls_one_time_init_done) _emutls_one_time_init();
    size_t tls_size = _emutls_size;
#if PICO_THREAD_LOCAL_SUPPORT_THREAD_POINTER
    extern __weak char __tls_size[];
    // be defensive about having someone put both types of data
    tls_size = MAX(tls_size, (size_t)&__tls_size);
#endif
    return tls_size;
}

static void *_emutls_per_core_init(void) {
    uint core_num = get_core_num();
    assert(!_get_tls_adjusted_for_core(core_num));
    // do this first, as _emutls_align may be set as a side effect
    size_t size = _tls_size();
    assert(size);
    if (size) {
        // aligned_alloc is not available in all libraries we support, and it isn't thread safe anyway,
        // so we'll just do the padded malloc
        // tls = aligned_alloc(size, _emutls_align);
        void *tls = malloc(size + _emutls_align - 1);
        // note we never free the memory, so bumping the pointer is fine
        tls = (void *)((((uintptr_t)tls) + (_emutls_align - 1)) & ~(_emutls_align - 1));
        if (tls) {
            _init_tls(tls);
            _set_tls(tls);
        }
    }
    return _get_tls_adjusted_for_core(core_num);
}

void* __emutls_get_address(void* obj) {
    void *tls_adjusted = _get_tls_adjusted_for_core(get_core_num());
    if (!tls_adjusted) {
        tls_adjusted = _emutls_per_core_init();
    }
    assert(tls_adjusted);
    return tls_adjusted + ((tls_object_t *)obj)->offset;
}
#endif

#if PICO_THREAD_LOCAL_SUPPORT_THREAD_POINTER && PICO_THREAD_LOCAL_THREAD_POINTER_VIA_RISCV_REG
// on RISC-V we must set up the pointer each time, note we don't actually respect PICO_THREAD_LOCAL_CORE1_REINITIALIZE
// on RISC-V as it is an optimization flag not a behavioral flag (i.e. it is intended to be set to 0
// if you don't need it vs don't want it)
#define _RUNTIME_INIT_PER_CORE_TLS_SETUP_IMPL _init_core_local_tls
#elif PICO_THREAD_LOCAL_CORE1_REINITIALIZE
static inline void _defer_core_local_init(void) {
    // note that __tls_adjusted is in .bss, so on core 0 init it is definitely 0 already, so no need to set it to 0
    // to save space, we don't call get_core_num() but just clear __tls_adjusted[1], as runtime_init_per_core_tls_setup
    // for core 0 will always be called before runtime_init_per_core_tls_setup for core 1 (so setting __tls_adjusted[1]
    // to 0 is idempotent, and then runtime_init_per_core_tls_setup is never called again

    // note we defer any actual work
    // __tls_adjusted_by_core[get_core_num()] = 0;
    __tls_adjusted_by_core[1] = 0;
}
#define _RUNTIME_INIT_PER_CORE_TLS_SETUP_IMPL _defer_core_local_init
#endif

#elif PICO_THREAD_LOCAL_MODE_GLOBAL
#if PICO_THREAD_LOCAL_MODE_NONE
#error PICO_THREAD_LOCAL_MODE_GLOBAL and PICO_THREAD_LOCAL_MODE_NONE are both specified
#endif

// ------------------------------------------------------------
// TLS values are global
// -------------------------------------------------------------
#define _INIT_TLS_IMPL(tls) ((void)tls)
#define _SET_TLS_IMPL(tls) ((void)tls)

#if PICO_THREAD_LOCAL_SUPPORT_THREAD_POINTER
#if PICO_THREAD_LOCAL_THREAD_POINTER_VIA_ARM_EABI
// naked as we must preserve all regs
uint32_t __weak __attribute__((naked)) __aeabi_read_tp(void) {
    pico_default_asm_volatile(
        ".weak __tls_start\n"
        "push {r1,lr}\n"
        "ldr r0, = __tls_start\n"
        "ldr r1, = __arm32_tls_tcb_offset\n"
        "subs r0, r1\n"
        "pop {r1, pc}\n"
    );
}
#elif PICO_THREAD_LOCAL_THREAD_POINTER_VIA_RISCV_REG
static inline void _global_tp_init(void) {
    extern __weak char __tls_start[];
    pico_default_asm_volatile("mv tp, %0\n" : : "r" (__tls_start));
}
#define _RUNTIME_INIT_PER_CORE_TLS_SETUP_IMPL _global_tp_init
#endif
#endif
#endif

#if PICO_THREAD_LOCAL_PROVIDE_INIT_TLS
void _init_tls(void *tls) {
    // we expect an impl in this case
    _INIT_TLS_IMPL(tls);
}
#endif

#if PICO_THREAD_LOCAL_PROVIDE_SET_TLS
void _set_tls(void *tls) {
    // we expect an impl in this case
    _SET_TLS_IMPL(tls);
}
#endif

#ifdef _RUNTIME_INIT_PER_CORE_TLS_SETUP_IMPL
void runtime_init_per_core_tls_setup(void) {
    _RUNTIME_INIT_PER_CORE_TLS_SETUP_IMPL();
}

#if !PICO_RUNTIME_SKIP_INIT_PER_CORE_TLS_SETUP
PICO_RUNTIME_INIT_FUNC_PER_CORE(runtime_init_per_core_tls_setup, PICO_RUNTIME_INIT_PER_CORE_TLS_SETUP);
#endif
#endif

