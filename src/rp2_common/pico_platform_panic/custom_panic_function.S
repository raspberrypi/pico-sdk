/*
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/asm_helper.S"

#ifdef PICO_PANIC_FUNCTION
#define	__CONCAT1(x,y)	x ## y
#define	__CONCAT(x,y)	__CONCAT1(x,y)

#define PICO_PANIC_FUNCTION_IS_EMPTY (__CONCAT(PICO_PANIC_FUNCTION, 1))
.text
// Use a forwarding method here as it is a little simpler than renaming the symbol as it is used from assembler
.weak panic // also allow override
.type panic,%function
panic:
#ifdef __riscv
    // we seem to need to help gdb out with call frame on RISC-V
    .cfi_sections .debug_frame
    .cfi_startproc
    #if PICO_PANIC_FUNCTION_IS_EMPTY
        1:  ebreak
            j 1b
    #elif PICO_PANIC_FUNCTION_DOES_NOT_RETURN
        // if you get an undefined reference here, you didn't define your PICO_PANIC_FUNCTION!
        jal zero, PICO_PANIC_FUNCTION
    #else
        #if PICO_PANIC_FUNCTION_WITH_CALL_STACK
            #if PICO_PANIC_FUNCTION_WITH_ALL_VAARGS
            #error PICO_PANIC_FUNCTION_WITH_CALL_STACK & PICO_PANIC_FUNCTION_WITH_ALL_VAARGS are incompatible when PICO_PANIC_FUNCTION_DOES_NOT_RETURN is 0
            #endif
            // .insn 0xb842      // cm.push {ra}, -16: Save ultimate return address
            // use explicit for better compatibility with gdb stack unwinding
            addi sp,sp,-16
            .cfi_def_cfa_offset 16
            sw ra,12(sp)
            .cfi_offset ra, -4
        #endif
        // if you get an undefined reference here, you didn't define your PICO_PANIC_FUNCTION!
        jal PICO_PANIC_FUNCTION
        #if !PICO_PANIC_FUNCTION_DOES_NOT_RETURN
        1:  ebreak
            j 1b
       #endif
   #endif
    .cfi_endproc
#else
    #if PICO_PANIC_FUNCTION_IS_EMPTY
        1:  bkpt #0
            b 1b
    #elif PICO_PANIC_FUNCTION_DOES_NOT_RETURN
        // if you get an undefined reference here, you didn't define your PICO_PANIC_FUNCTION!
        ldr r4, =PICO_PANIC_FUNCTION
        bx r4
    #else
        #if PICO_PANIC_FUNCTION_WITH_CALL_STACK
            #if PICO_PANIC_FUNCTION_WITH_ALL_VAARGS
            #error PICO_PANIC_FUNCTION_WITH_CALL_STACK & PICO_PANIC_FUNCTION_WITH_ALL_VAARGS are incompatible when PICO_PANIC_FUNCTION_DOES_NOT_RETURN is 0
            #endif
            push {r4, lr} // must keep 8 byte alignment for call to user function
        #endif
        // if you get an undefined reference here, you didn't define your PICO_PANIC_FUNCTION!
        bl PICO_PANIC_FUNCTION
        #if !PICO_PANIC_FUNCTION_DOES_NOT_RETURN
        1:  bkpt #0
            b 1b
        #endif
    #endif
#endif

#endif // PICO_PANIC_FUNCTION


