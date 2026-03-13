#include <stdio.h>
#include <stdarg.h>

#include "pico/stdlib.h"

#define MAGIC1 ((const char *)0x87654321)
#define MAGIC2 0x97654321
#define MAGIC3 3.14159265358979323846
#define MAGIC4 123
#define MAGIC5 456
#define MAGIC6 789
#define MAGIC7 0xabc

#define PICO_PANIC_FUNCTION_IS_EMPTY (__CONCAT(PICO_PANIC_FUNCTION, 1))

void __printflike(1, 0) handle_panic(const char *magic1, ...)
{
    printf("checking first arg...\n");
    if (magic1 != MAGIC1) {
        printf("magic1 (%08x) != 0x%08x\n", magic1, MAGIC1);
        return;
    }
    va_list args;
    va_start(args, magic1);
    printf("checking early vaargs...\n");
    int magic2 = va_arg(args, int);
    if (magic2 != MAGIC2) {
        printf("magic2 (%08x) != 0x%08x\n", magic2, MAGIC2);
        return;
    }
    double magic3 = va_arg(args, double);
    if (magic3 != MAGIC3) {
        printf("magic3 (%f) != 0x%f\n", magic3, MAGIC3);
        return;
    }
#if PICO_PANIC_FUNCTION_WITH_ALL_VAARGS
    printf("checking remaining vaargs...\n");
    int magic4 = va_arg(args, int);
    if (magic4 != MAGIC4) {
        printf("magic4 (%08x) != 0x%08x\n", magic4, MAGIC4);
        return;
    }
    int magic5 = va_arg(args, int);
    if (magic5 != MAGIC5) {
        printf("magic5 (%08x) != 0x%08x\n", magic5, MAGIC5);
        return;
    }
    int magic6 = va_arg(args, int);
    if (magic6 != MAGIC6) {
        printf("magic6 (%08x) != 0x%08x\n", magic6, MAGIC6);
        return;
    }
    int magic7 = va_arg(args, int);
    if (magic7 != MAGIC7) {
        printf("magic7 (%08x) != 0x%08x\n", magic7, MAGIC7);
        return;
    }
#endif
    va_end(args);
    puts("PASSED");
#if PICO_PANIC_FUNCTION_DOES_NOT_RETURN
    __breakpoint();
#endif
}

void main() {
    stdio_init_all();
#ifndef PICO_PANIC_FUNCTION
    printf("Using default panic function...\n");
    panic("PASSED"); // should be printed
#else
    #if PICO_PANIC_FUNCTION_IS_EMPTY
    printf("Using empty (bkpt only) panic function...\n");
    printf("PASSED\n"); // we should breakpoint next
    panic(MAGIC1, MAGIC2, MAGIC3, MAGIC4, MAGIC5, MAGIC6, MAGIC7);
    #endif
    printf("Using custom panic function '" __XSTRING(PICO_PANIC_FUNCTION) "'...\n");
    panic(MAGIC1, MAGIC2, MAGIC3, MAGIC4, MAGIC5, MAGIC6, MAGIC7);
#endif
    printf("FAILED: expected panic not to return");
}
