/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hardware/sync.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/sem.h"

#include <picotls.h>
#define COUNT 10000

volatile __thread int counter = 7;
__thread int zero;
int core1_count;

semaphore_t sem;

int do_count(int delta) {
    for (int i=0;i<COUNT;i++) {
        counter += delta;
    }
    return counter;
}

void core1_entry() {
    core1_count = do_count(2);
    sem_release(&sem);
}

void core1_entry2() {
    core1_count = counter * 2;
    sem_release(&sem);
}

bool run_test(void) {
#if __riscv
    puts("Running on RISC-V");
#else
    puts("Running on ARM");
#endif

#if PICO_THREAD_LOCAL_MODE_PER_THREAD
    puts("TLS MODE: per thread");
#elif PICO_THREAD_LOCAL_MODE_GLOBAL
    puts("TLS MODE: global");
#elif PICO_THREAD_LOCAL_MODE_NONE
    puts("TLS MODE: none");
#else
#error
#endif

    printf("BSS value: %d (expected %d)\n", zero, 0);
    if (zero != 0) {
        return false;
    }
#if PICO_THREAD_LOCAL_MODE_PER_THREAD
    //printf("AH have %d\n", _tls_size());
    sem_init(&sem, 0, 1);
    multicore_launch_core1(core1_entry);
    int core0_count = do_count(1);
    sem_acquire_blocking(&sem);
    printf("Core 0: %d (expected %d)\n", core0_count, COUNT + 7);
    printf("Core 1: %d (expected %d)\n", core1_count, COUNT*2 + 7);
    if (core0_count != COUNT + 7 || core1_count != COUNT*2 + 7) {
        return false;
        return -1;
    }
    multicore_reset_core1();
    multicore_launch_core1(core1_entry2);
    sem_acquire_blocking(&sem);
    printf("Core 0: %d (expected %d)\n", counter, COUNT + 7);
    printf("Restart core1 1: %d (expected %d)\n", core1_count, 14);
    if (core0_count != COUNT + 7 || core1_count != 14) {
        return false;
        return -1;
    }
#elif PICO_THREAD_LOCAL_MODE_GLOBAL
    int core0_count = do_count(1);
    printf("Core 0: %d (expected %d)\n", core0_count, COUNT + 7);
#endif
    return true;
}

int main() {
    stdio_init_all();
    if (run_test()) {
        puts("PASSED");
    } else {
        puts("FAILED");
    }
}
