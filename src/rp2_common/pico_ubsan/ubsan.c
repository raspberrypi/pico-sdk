/*
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Handlers for the subset of UndefinedBehaviorSanitizer enabled by
// PICO_UBSAN_ALIGNMENT_CHECKS and PICO_UBSAN_NULL_CHECKS.
//
// The structure layouts below are the UBSan ABI

#include <stdint.h>
#include <stdio.h>
#include "pico.h"
#include "pico/platform/panic.h"

// These functions must not be instrumented themselves: a check firing inside a
// handler would call the handler again
#define PICO_UBSAN_HANDLER __attribute__((no_sanitize("undefined")))

struct source_location {
    const char *file_name;
    uint32_t line;
    uint32_t column;
};

struct type_descriptor {
    uint16_t type_kind;
    uint16_t type_info;
    char type_name[1];
};

struct type_mismatch_data_v1 {
    struct source_location loc;
    const struct type_descriptor *type;
    unsigned char log_alignment;
    unsigned char type_check_kind;
};

// type_check_kind values we can usefully distinguish
#define TYPE_CHECK_KIND_LOAD    0
#define TYPE_CHECK_KIND_STORE   1

// UBSan calls one of two entry points depending on whether the check was
// compiled as recoverable.
//
//   __ubsan_handle_type_mismatch_v1        must return; the program then goes
//                                          ahead and performs the access
//   __ubsan_handle_type_mismatch_v1_abort  must not return

// PICO_CONFIG: PICO_UBSAN_MAX_REPORTS, Maximum number of reports the recoverable UBSan handler will print before falling silent, min=1, default=20, group=pico_ubsan
#ifndef PICO_UBSAN_MAX_REPORTS
#define PICO_UBSAN_MAX_REPORTS 20
#endif

static unsigned reports;

struct mismatch {
    const char *file;
    unsigned line;
    const char *type_name;
    const char *op;
    uintptr_t ptr;
    unsigned alignment;
};

PICO_UBSAN_HANDLER static struct mismatch describe(void *_data, void *_ptr) {
    struct type_mismatch_data_v1 *data = _data;
    // Report just the file name; with the line number
    const char *file = data->loc.file_name;
    for (const char *s = file; *s; s++) {
        if (*s == '/' || *s == '\\') {
            file = s + 1;
        }
    }
    struct mismatch m = {
        .file = file,
        .line = (unsigned)data->loc.line,
        .type_name = data->type->type_name,
        .op = data->type_check_kind == TYPE_CHECK_KIND_STORE ? "store" : "load",
        .ptr = (uintptr_t)_ptr,
        .alignment = 1u << data->log_alignment,
    };
    return m;
}

// Recoverable: report and return. Execution then performs the access anyway,
// which on Arm succeeds, and on RP2350's RISC-V core will fault
void __ubsan_handle_type_mismatch_v1(void *data, void *ptr);
PICO_UBSAN_HANDLER void __ubsan_handle_type_mismatch_v1(void *data, void *ptr) {
    if (++reports > PICO_UBSAN_MAX_REPORTS) {
        if (reports == PICO_UBSAN_MAX_REPORTS + 1) {
            printf("ubsan: further reports suppressed\n");
        }
        return;
    }
    struct mismatch m = describe(data, ptr);
    if (!m.ptr) {
        printf("ubsan: %s:%u null pointer access to %s\n", m.file, m.line, m.type_name);
    } else {
        printf("ubsan: %s:%u misaligned %s of %s at %p (needs %u)\n",
               m.file, m.line, m.op, m.type_name, (void *)m.ptr, m.alignment);
    }
}

// Non-recoverable, emitted under -fno-sanitize-recover. It does not return
void __ubsan_handle_type_mismatch_v1_abort(void *data, void *ptr);
PICO_UBSAN_HANDLER void __ubsan_handle_type_mismatch_v1_abort(void *data, void *ptr) {
    struct mismatch m = describe(data, ptr);
    // Break for clang which discards the return address as panic is noreturn
    __breakpoint();
    if (!m.ptr) {
        panic("ubsan: %s:%u null pointer access to %s", m.file, m.line, m.type_name);
    }
    panic("ubsan: %s:%u misaligned %s of %s at %p (needs %u)",
          m.file, m.line, m.op, m.type_name, (void *)m.ptr, m.alignment);
}
