/*
 * Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
 * Copyright (c) 2024 Stephen Street (stephen@redrocketcomputing.com).
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __STDATOMIC_H
#define __STDATOMIC_H

#include_next <stdatomic.h>

#undef atomic_flag_test_and_set
#undef atomic_flag_test_and_set_explicit
#undef atomic_flag_clear
#undef atomic_flag_clear_explicit

extern _Bool __atomic_test_and_set_m0(volatile void *mem, int model);
extern void __atomic_clear_m0 (volatile void *mem, int model);

#define atomic_flag_test_and_set(PTR) __atomic_test_and_set_m0((PTR), __ATOMIC_SEQ_CST)
#define atomic_flag_test_and_set_explicit(PTR, MO) __atomic_test_and_set_m0((PTR), (MO))

#define atomic_flag_clear(PTR) __atomic_clear_m0((PTR), __ATOMIC_SEQ_CST)
#define atomic_flag_clear_explicit(PTR, MO) __atomic_clear_m0((PTR), (MO))

#endif
