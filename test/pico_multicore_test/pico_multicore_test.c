/**
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/test.h"
#include "pico/time.h"
#include "hardware/irq.h"

#define CORE0_GPIO 7
#define CORE1_GPIO 8

PICOTEST_MODULE_NAME("pico_multicore_test", "pico_multicore test harness");

static volatile bool test_alarm_called_core0;
static volatile bool test_alarm_called_core1;
static int64_t test_alarm(alarm_id_t id, void *param) {
    printf("test_alarm called on core%d\n", get_core_num());
    if (get_core_num() == 0) {
        test_alarm_called_core0 = true;
        gpio_put(CORE1_GPIO, true); // trigger test_gpio_core1
    } else {
        test_alarm_called_core1 = true;
        gpio_put(CORE0_GPIO, true); // trigger test_gpio_core0
    }
    return 0;
}

static volatile bool test_gpio_called_core0;
static volatile bool test_gpio_called_core1;

#if PICO_VTABLE_PER_CORE
void test_gpio_core0(void) {
    printf("test_gpio_core0 called on core%d\n", get_core_num());
    test_gpio_called_core0 = true;
    gpio_acknowledge_irq(CORE0_GPIO, GPIO_IRQ_EDGE_RISE);
}
irq_handler_t test_gpio_irq_handler_core0 = &test_gpio_core0;

void test_gpio_core1(void) {
    printf("test_gpio_core1 called on core%d\n", get_core_num());
    test_gpio_called_core1 = true;
    gpio_acknowledge_irq(CORE1_GPIO, GPIO_IRQ_EDGE_RISE);
}
irq_handler_t test_gpio_irq_handler_core1 = &test_gpio_core1;
#else
void test_gpio(void) {
    printf("test_gpio called on core%d\n", get_core_num());
    if (get_core_num() == 0) {
        test_gpio_called_core0 = true;
        gpio_acknowledge_irq(CORE0_GPIO, GPIO_IRQ_EDGE_RISE);
    } else {
        test_gpio_called_core1 = true;
        gpio_acknowledge_irq(CORE1_GPIO, GPIO_IRQ_EDGE_RISE);
    }
}

irq_handler_t test_gpio_irq_handler_core0 = &test_gpio;
irq_handler_t test_gpio_irq_handler_core1 = &test_gpio;
#endif

static volatile uint32_t core1_vtor;
static void main_core1(void) {
#ifdef __riscv
    core1_vtor = riscv_read_csr(RVCSR_MTVEC_OFFSET) & ~0x3u;
#else
    core1_vtor = scb_hw->vtor;
#endif
    printf("core1 vtor %p\n", (uint32_t*)core1_vtor);

    alarm_pool_t *core1_alarm_pool = alarm_pool_create_on_timer_with_unused_hardware_alarm(PICO_DEFAULT_TIMER_INSTANCE(), PICO_TIME_DEFAULT_ALARM_POOL_MAX_TIMERS);

    alarm_id_t alarm_id = alarm_pool_add_alarm_in_ms(core1_alarm_pool, 500, test_alarm, NULL, false);
    
    gpio_init(CORE1_GPIO);
    gpio_set_dir(CORE1_GPIO, GPIO_OUT);
    irq_set_exclusive_handler(IO_IRQ_BANK0, test_gpio_irq_handler_core1);
    gpio_set_irq_enabled(CORE1_GPIO, GPIO_IRQ_EDGE_RISE, true);
    irq_set_enabled(IO_IRQ_BANK0, true);

    while (true) tight_loop_contents();
}

int main() {
    stdio_init_all();

#ifdef __riscv
    uint32_t core0_vtor = riscv_read_csr(RVCSR_MTVEC_OFFSET) & ~0x3u;
#else
    uint32_t core0_vtor = scb_hw->vtor;
#endif
    printf("core0 vtor %p\n", (uint32_t*)core0_vtor);

    printf("pico_multicore_test begins\n");
    PICOTEST_START();

    // Check default config has valid data in it
    PICOTEST_START_SECTION("multicore test");
    multicore_launch_core1(main_core1);

    alarm_id_t alarm_id = add_alarm_in_ms(1000, test_alarm, NULL, false);

    gpio_init(CORE0_GPIO);
    gpio_set_dir(CORE0_GPIO, GPIO_OUT);
    irq_set_exclusive_handler(IO_IRQ_BANK0, test_gpio_irq_handler_core0);
    gpio_set_irq_enabled(CORE0_GPIO, GPIO_IRQ_EDGE_RISE, true);
    irq_set_enabled(IO_IRQ_BANK0, true);
    sleep_ms(1500);

#if PICO_VTABLE_PER_CORE
    PICOTEST_CHECK(core0_vtor != core1_vtor, "core0 and core1 have same vtor");
#else
    PICOTEST_CHECK(core0_vtor == core1_vtor, "core0 and core1 have different vtors");
#endif
    PICOTEST_CHECK(test_alarm_called_core0, "test_alarm was not called on core0");
    PICOTEST_CHECK(test_alarm_called_core1, "test_alarm was not called on core1");
    PICOTEST_CHECK(test_gpio_called_core0, "test_gpio was not called on core0");
    PICOTEST_CHECK(test_gpio_called_core1, "test_gpio was not called on core1");
    PICOTEST_END_SECTION();

    PICOTEST_END_TEST();
}

