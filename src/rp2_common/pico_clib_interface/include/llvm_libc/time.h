/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_LLVM_LIBC_TIME_H
#define _PICO_LLVM_LIBC_TIME_H

#include <__llvm-libc-common.h>

#include <llvm-libc-types/struct_timespec.h>
#include <llvm-libc-types/struct_tm.h>
#include <llvm-libc-types/time_t.h>

__BEGIN_C_DECLS

struct tm* localtime_r(const time_t* timer, struct tm* buf) __NOEXCEPT;
time_t mktime(struct tm *);

__END_C_DECLS

#endif // _PICO_LLVM_LIBC_TIME_H
