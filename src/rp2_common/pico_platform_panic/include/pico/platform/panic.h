/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_PLATFORM_PANIC_H
#define _PICO_PLATFORM_PANIC_H

#ifdef __cplusplus
extern "C" {
#endif

// PICO_CONFIG: PICO_PANIC_FUNCTION, Name of a function to use in place of the stock panic function or empty string to simply breakpoint on panic, group=pico_runtime

// PICO_CONFIG: PICO_PANIC_FUNCTION_WITH_CALL_STACK, Ensure the call stack is available when using a custom panic function via PICO_PANIC_FUNCTION. When set to 1 it conflicts with PICO_PANIC_FUNCTION_WITH_ALL_VAARGS as a stack frame must be pushed onto the stack corrupting later vaargs. Note this defaults to 1 as most custom panic functions don't actually use the arguments and the call stack seems more useful in general, default=1, group=pico_runtime
#ifndef PICO_PANIC_FUNCTION_WITH_CALL_STACK
#define PICO_PANIC_FUNCTION_WITH_CALL_STACK 1
#endif

// PICO_CONFIG: PICO_PANIC_FUNCTION_WITH_ALL_VAARGS, Ensures that all vaargs are faithfully passed to the custom panic function via PICO_PANIC_FUNCTION. When set to 1 it conflicts with PICO_PANIC_FUNCTION_WITH_CALL_STACK because both the later vaargs and the stack frame would be expected at the top of the stack. Note this defaults to 0 as most custom panic functions don't actually use the arguments and the call stack seems more useful in general, default=0, group=pico_runtime
#ifndef PICO_PANIC_FUNCTION_WITH_ALL_VAARGS
#define PICO_PANIC_FUNCTION_WITH_ALL_VAARGS 0
#endif

// PICO_CONFIG: PICO_PANIC_FUNCTION_DOES_NOT_RETURN, Indicate that the user supplied PICO_PANIC_FUNCTION does not return so the caller does not need to inject a breakpoint. When this is 1 the full call stack is available and all vaargs are correctly passed to the user function, default=0, group=pico_runtime
#ifndef PICO_PANIC_FUNCTION_DOES_NOT_RETURN
#define PICO_PANIC_FUNCTION_DOES_NOT_RETURN 0
#endif

#ifndef __ASSEMBLER__
/*! \brief Panics with the message "Unsupported"
 *  \ingroup pico_platform
 *  \see panic
 */
void __attribute__((noreturn)) panic_unsupported(void);

/*! \brief Displays a panic message and halts execution
 *  \ingroup pico_platform
 *
 * An attempt is made to output the message to all registered STDOUT drivers
 * after which this method executes a BKPT instruction.
 *
 * @param fmt format string (printf-like)
 * @param ...  printf-like arguments
 */
void __attribute__((noreturn)) panic(const char *fmt, ...);

#ifdef NDEBUG
#define panic_compact(...) panic(__VA_ARGS__)
#else
#define panic_compact(...) panic("")
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif
