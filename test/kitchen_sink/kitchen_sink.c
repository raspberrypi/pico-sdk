/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
// Include all headers to check for compiler warnings
#include "hardware/adc.h"
#include "hardware/claim.h"
#include "hardware/clocks.h"
#include "hardware/divider.h"
#include "hardware/dma.h"
#include "hardware/exception.h"
#include "hardware/flash.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/interp.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "hardware/pio_instructions.h"
#include "hardware/pll.h"
#include "hardware/pwm.h"
#include "hardware/resets.h"
#if PICO_RP2040
#include "hardware/rtc.h"
#endif
#if !PICO_RP2040
#include "hardware/sha256.h"
#endif
#include "hardware/spi.h"
#include "hardware/sync.h"
#include "hardware/timer.h"
#include "hardware/ticks.h"
#include "hardware/uart.h"
#include "hardware/vreg.h"
#include "hardware/watchdog.h"
#include "hardware/xosc.h"
#include "pico/aon_timer.h"
#include "pico/binary_info.h"
#include "pico/bit_ops.h"
#include "pico/bootrom.h"
#if LIB_PICO_CYW43_ARCH
#include "pico/cyw43_arch.h"
#endif
#include "pico/divider.h"
// todo we should have this but right now double.h is only present with double_implementation == pico
#if PICO_RP2040
#include "pico/double.h"
#endif
#include "pico/fix/rp2040_usb_device_enumeration.h"
#include "pico/flash.h"
// todo we should have this but right now float.h is only present with float_implementation == pico
#if PICO_RP2040
#include "pico/float.h"
#endif
#include "pico/i2c_slave.h"
#if LIB_PICO_INT64_OPS_PICO
#include "pico/int64_ops.h"
#endif
#include "pico/malloc.h"
#include "pico/multicore.h"
#include "pico/platform.h"
#include "pico/printf.h"
#include "pico/rand.h"
#include "pico/runtime.h"
#if LIB_PICO_SHA256
#include "pico/sha256.h"
#endif
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "pico/sync.h"
#include "pico/time.h"
#include "pico/unique_id.h"
#include "pico/util/datetime.h"
#include "pico/util/pheap.h"
#include "pico/util/queue.h"

#include "hardware/structs/adc.h"
#include "hardware/structs/busctrl.h"
#include "hardware/structs/clocks.h"
#include "hardware/structs/dma.h"
#include "hardware/structs/i2c.h"
#include "hardware/structs/interp.h"
#include "hardware/structs/io_bank0.h"
#include "hardware/structs/io_qspi.h"
#ifndef __riscv
#include "hardware/structs/mpu.h"
#include "hardware/structs/nvic.h"
#endif
#include "hardware/structs/pads_bank0.h"
#include "hardware/structs/pads_qspi.h"
#include "hardware/structs/pio.h"
#include "hardware/structs/pll.h"
#if PICO_RP2350
#include "hardware/structs/powman.h"
#endif
#include "hardware/structs/psm.h"
#include "hardware/structs/pwm.h"
#include "hardware/structs/resets.h"
#include "hardware/structs/rosc.h"
#if PICO_RP2040
#include "hardware/structs/rtc.h"
#endif
#ifndef __riscv
#include "hardware/structs/scb.h"
#endif
#include "hardware/structs/sio.h"
#if !PICO_RP2040
#include "hardware/structs/sha256.h"
#endif
#include "hardware/structs/spi.h"
#if PICO_RP2040
#include "hardware/structs/ssi.h"
#endif
#include "hardware/structs/syscfg.h"
#ifndef __riscv
#include "hardware/structs/systick.h"
#endif
#include "hardware/structs/timer.h"
#include "hardware/structs/uart.h"
#include "hardware/structs/usb.h"
#if PICO_RP2040
#include "hardware/structs/vreg_and_chip_reset.h"
#endif
#include "hardware/structs/watchdog.h"
#include "hardware/structs/xip_ctrl.h"
#include "hardware/structs/xosc.h"

#if LIB_PICO_MBEDTLS
#include "mbedtls/ssl.h"
#include "lwip/altcp_tcp.h"
#include "lwip/altcp_tls.h"
#endif

bi_decl(bi_block_device(
                           BINARY_INFO_MAKE_TAG('K', 'S'),
                           "foo",
                           0x80000,
                           0x40000,
                           NULL,
                           BINARY_INFO_BLOCK_DEV_FLAG_READ | BINARY_INFO_BLOCK_DEV_FLAG_WRITE |
                                   BINARY_INFO_BLOCK_DEV_FLAG_PT_UNKNOWN));

uint32_t *foo = (uint32_t *) 200;

uint32_t dma_to = 0;
uint32_t dma_from = 0xaaaa5555;

void __noinline spiggle(void) {
    dma_channel_config c = dma_channel_get_default_config(1);
    channel_config_set_bswap(&c, true);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_16);
    channel_config_set_ring(&c, true, 13);
    dma_channel_set_config(1, &c, false);
    dma_channel_transfer_from_buffer_now(1, foo, 23);
}

__force_inline int something_inlined(int x) {
    return x * 2;
}

auto_init_mutex(mutex);
auto_init_recursive_mutex(recursive_mutex);

float __attribute__((noinline)) foox(float x, float b) {
    return x * b;
}


int main(void) {
    spiggle();

    stdio_init_all();

    printf("HI %d\n", something_inlined((int)time_us_32()));
    puts("Hello Everything!");
    puts("Hello Everything2!");

    printf("main at %p\n", (void *)main);
    static uint x[2];
    printf("x[0] = %p, x[1] = %p\n", x, x+1);
#ifdef __riscv
    printf("RISC-V\n");
#else
    printf("ARM\n");
#endif
#ifdef KITCHEN_SINK_ID
    puts(KITCHEN_SINK_ID);
#endif
    hard_assert(mutex_try_enter(&mutex, NULL));
    hard_assert(!mutex_try_enter(&mutex, NULL));
    hard_assert(recursive_mutex_try_enter(&recursive_mutex, NULL));
    hard_assert(recursive_mutex_try_enter(&recursive_mutex, NULL));
    printf("%f\n", foox(1.3f, 2.6f));
#ifndef __riscv
    // this should compile as we are Cortex M0+
    pico_default_asm ("SVC #3");
#endif
}
