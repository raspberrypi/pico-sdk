/*
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "pico/unique_id.h"
#include "cyw43.h"
#include "pico/cyw43_driver.h"
#include "pico/async_context.h"

#ifndef CYW43_GPIO_IRQ_HANDLER_PRIORITY
#define CYW43_GPIO_IRQ_HANDLER_PRIORITY 0x40
#endif

#ifndef CYW43_SLEEP_CHECK_MS
#define CYW43_SLEEP_CHECK_MS 50
#endif

static async_context_t *cyw43_async_context = NULL;

#if CYW43_USE_PARTITION_FIRMWARE
    #include "pico/bootrom.h"
    #include "hardware/flash.h"
    #include "boot/picobin.h"
    #include <stdlib.h>

    // PICO_CONFIG: CYW43_WIFI_FW_PARTITION_ID, ID of Wi-Fi firmware partition, type=int, default=0x776966696669726d (wififirm), group=pico_cyw43_driver
    #ifndef CYW43_WIFI_FW_PARTITION_ID
    #define CYW43_WIFI_FW_PARTITION_ID 0x776966696669726d // wififirm
    #endif

    int32_t cyw43_wifi_fw_len;
    int32_t cyw43_clm_len;
    uintptr_t fw_data;
#endif

static void cyw43_sleep_timeout_reached(async_context_t *context, async_at_time_worker_t *worker);
static void cyw43_do_poll(async_context_t *context, async_when_pending_worker_t *worker);

static async_at_time_worker_t sleep_timeout_worker = {
        .do_work = cyw43_sleep_timeout_reached
};

static async_when_pending_worker_t cyw43_poll_worker = {
        .do_work = cyw43_do_poll
};

static void cyw43_set_irq_enabled(bool enabled) {
    gpio_set_irq_enabled(CYW43_PIN_WL_HOST_WAKE, GPIO_IRQ_LEVEL_HIGH, enabled);
}

// GPIO interrupt handler to tell us there's cyw43 has work to do
static void cyw43_gpio_irq_handler(void)
{
    uint32_t events = gpio_get_irq_event_mask(CYW43_PIN_WL_HOST_WAKE);
    if (events & GPIO_IRQ_LEVEL_HIGH) {
        // As we use a high level interrupt, it will go off forever until it's serviced
        // So disable the interrupt until this is done. It's re-enabled again by CYW43_POST_POLL_HOOK
        // which is called at the end of cyw43_poll_func
        cyw43_set_irq_enabled(false);
        async_context_set_work_pending(cyw43_async_context, &cyw43_poll_worker);
    }
}

uint32_t cyw43_irq_init(__unused void *param) {
#ifndef NDEBUG
    assert(get_core_num() == async_context_core_num(cyw43_async_context));
#endif
    gpio_add_raw_irq_handler_with_order_priority(CYW43_PIN_WL_HOST_WAKE, cyw43_gpio_irq_handler, CYW43_GPIO_IRQ_HANDLER_PRIORITY);
    cyw43_set_irq_enabled(true);
    irq_set_enabled(IO_IRQ_BANK0, true);
    return 0;
}

uint32_t cyw43_irq_deinit(__unused void *param) {
#ifndef NDEBUG
    assert(get_core_num() == async_context_core_num(cyw43_async_context));
#endif
    cyw43_set_irq_enabled(false);
    gpio_remove_raw_irq_handler(CYW43_PIN_WL_HOST_WAKE, cyw43_gpio_irq_handler);
    return 0;
}

void cyw43_post_poll_hook(void) {
#ifndef NDEBUG
    assert(get_core_num() == async_context_core_num(cyw43_async_context));
#endif
    cyw43_set_irq_enabled(true);
}

void cyw43_schedule_internal_poll_dispatch(__unused void (*func)(void)) {
    assert(func == cyw43_poll);
    async_context_set_work_pending(cyw43_async_context, &cyw43_poll_worker);
}

static void cyw43_do_poll(async_context_t *context, __unused async_when_pending_worker_t *worker) {
#ifndef NDEBUG
    assert(get_core_num() == async_context_core_num(cyw43_async_context));
#endif
    if (cyw43_poll) {
        if (cyw43_sleep > 0) {
            cyw43_sleep--;
        }
        cyw43_poll();
        if (cyw43_sleep) {
            async_context_add_at_time_worker_in_ms(context, &sleep_timeout_worker, CYW43_SLEEP_CHECK_MS);
        } else {
            async_context_remove_at_time_worker(context, &sleep_timeout_worker);
        }
    }
}

static void cyw43_sleep_timeout_reached(async_context_t *context, __unused async_at_time_worker_t *worker) {
    assert(context == cyw43_async_context);
    assert(worker == &sleep_timeout_worker);
    async_context_set_work_pending(context, &cyw43_poll_worker);
}

bool cyw43_driver_init(async_context_t *context) {
#if CYW43_USE_PARTITION_FIRMWARE
    const int buf_words = (16 * 4) + 1;   // maximum of 16 partitions, each with maximum of 4 words returned, plus 1
    uint32_t* buffer = malloc(buf_words * 4);
    int ret = rom_get_partition_table_info(buffer, buf_words, PT_INFO_PARTITION_LOCATION_AND_FLAGS | PT_INFO_PARTITION_ID);

    assert(buffer[0] == (PT_INFO_PARTITION_LOCATION_AND_FLAGS | PT_INFO_PARTITION_ID));

    if (ret > 0) {
        int i = 1;
        int p = 0;
        int picked_p = -1;
        while (i < ret) {
            i++;
            uint32_t flags_and_permissions = buffer[i++];
            bool has_id = (flags_and_permissions & PICOBIN_PARTITION_FLAGS_HAS_ID_BITS);
            if (has_id) {
                uint64_t id = 0;
                id |= buffer[i++];
                id |= ((uint64_t)(buffer[i++]) << 32ull);
                if (id == CYW43_WIFI_FW_PARTITION_ID) {
                    picked_p = p;
                }
            }

            p++;
        }

        if (picked_p >= 0) {
            uint32_t* workarea = malloc(0x1000);
            picked_p = rom_pick_ab_update_partition(workarea, 0x1000, picked_p);
            free(workarea);

            if (picked_p < 0) {
                if (picked_p == BOOTROM_ERROR_NOT_FOUND) {
                    CYW43_DEBUG("Chosen CYW43 firmware partition was not verified\n");
                } else if (picked_p == BOOTROM_ERROR_NOT_PERMITTED) {
                    CYW43_DEBUG("Too many update boots going on at once\n");
                }
                return false;
            }

            CYW43_DEBUG("Chosen CYW43 firmware in partition %d\n", picked_p);
            int ret = rom_get_partition_table_info(buffer, buf_words, PT_INFO_PARTITION_LOCATION_AND_FLAGS | PT_INFO_SINGLE_PARTITION | (picked_p << 24));
            assert(buffer[0] == (PT_INFO_PARTITION_LOCATION_AND_FLAGS | PT_INFO_SINGLE_PARTITION));
            assert(ret == 3);

            uint32_t location_and_permissions = buffer[1];
            uint32_t saddr = ((location_and_permissions >> PICOBIN_PARTITION_LOCATION_FIRST_SECTOR_LSB) & 0x1fffu) * FLASH_SECTOR_SIZE;
            uint32_t eaddr = (((location_and_permissions >> PICOBIN_PARTITION_LOCATION_LAST_SECTOR_LSB) & 0x1fffu) + 1) * FLASH_SECTOR_SIZE;
            // Starts with metadata block
            while(saddr < eaddr && *(uint32_t*)(XIP_NOCACHE_NOALLOC_NOTRANSLATE_BASE + saddr) != PICOBIN_BLOCK_MARKER_END) {
                saddr += 4;
            }
            saddr += 4;
            // Now onto data
            cyw43_wifi_fw_len = *(uint32_t*)(XIP_NOCACHE_NOALLOC_NOTRANSLATE_BASE + saddr);
            cyw43_clm_len = *(uint32_t*)(XIP_NOCACHE_NOALLOC_NOTRANSLATE_BASE + saddr + 4);
            fw_data = XIP_NOCACHE_NOALLOC_NOTRANSLATE_BASE + saddr + 8;
        }
        free(buffer);
    } else {
        free(buffer);
        CYW43_DEBUG("No partition table, so cannot get firmware from partition - get_partition_table_info returned %d\n", ret);
        return false;
    }

#endif
    cyw43_init(&cyw43_state);
    cyw43_async_context = context;
    // we need the IRQ to be on the same core as the context, because we need to be able to enable/disable the IRQ
    // from there later
    async_context_execute_sync(context, cyw43_irq_init, NULL);
    async_context_add_when_pending_worker(context, &cyw43_poll_worker);
    return true;
}

void cyw43_driver_deinit(async_context_t *context) {
    if (cyw43_async_context != NULL) {
        assert(context == cyw43_async_context);
        async_context_remove_at_time_worker(context, &sleep_timeout_worker);
        async_context_remove_when_pending_worker(context, &cyw43_poll_worker);
        // the IRQ IS on the same core as the context, so must be de-initialized there
        async_context_execute_sync(context, cyw43_irq_deinit, NULL);
        cyw43_deinit(&cyw43_state);
        cyw43_async_context = NULL;
    }
}

// todo maybe add an #ifdef in cyw43_driver
uint32_t storage_read_blocks(__unused uint8_t *dest, __unused uint32_t block_num, __unused uint32_t num_blocks) {
    // shouldn't be used
    panic_unsupported();
}

// Generate a mac address if one is not set in otp
void __attribute__((weak)) cyw43_hal_generate_laa_mac(__unused int idx, uint8_t buf[6]) {
    CYW43_DEBUG("Warning. No mac in otp. Generating mac from board id\n");
    pico_unique_board_id_t board_id;
    pico_get_unique_board_id(&board_id);
    memcpy(buf, &board_id.id[2], 6);
    buf[0] &= (uint8_t)~0x1; // unicast
    buf[0] |= 0x2; // locally administered
}

// Return mac address
void cyw43_hal_get_mac(__unused int idx, uint8_t buf[6]) {
    // The mac should come from cyw43 otp.
    // This is loaded into the state after the driver is initialised
    // cyw43_hal_generate_laa_mac is called by the driver to generate a mac if otp is not set
    memcpy(buf, cyw43_state.mac, 6);
}

// Prevent background processing in pensv and access by the other core
// These methods are called in pensv context and on either core
// They can be called recursively
void cyw43_thread_enter(void) {
    async_context_acquire_lock_blocking(cyw43_async_context);
}

void cyw43_thread_exit(void) {
    async_context_release_lock(cyw43_async_context);
}

#ifndef NDEBUG
void cyw43_thread_lock_check(void) {
    async_context_lock_check(cyw43_async_context);
}
#endif

void cyw43_await_background_or_timeout_us(uint32_t timeout_us) {
    if (__get_current_exception() > 0) {
        async_context_wait_until(cyw43_async_context, make_timeout_time_us(timeout_us));
        return;
    }
    async_context_wait_for_work_until(cyw43_async_context, make_timeout_time_us(timeout_us));
}

void cyw43_delay_ms(uint32_t ms) {
    async_context_wait_until(cyw43_async_context, make_timeout_time_ms(ms));
}

void cyw43_delay_us(uint32_t us) {
    async_context_wait_until(cyw43_async_context, make_timeout_time_us(us));
}

#if !CYW43_LWIP
static void no_lwip_fail() {
    panic("cyw43 has no ethernet interface");
}
void __attribute__((weak)) cyw43_cb_tcpip_init(cyw43_t *self, int itf) {
}
void __attribute__((weak)) cyw43_cb_tcpip_deinit(cyw43_t *self, int itf) {
}
void __attribute__((weak)) cyw43_cb_tcpip_set_link_up(cyw43_t *self, int itf) {
    no_lwip_fail();
}
void __attribute__((weak)) cyw43_cb_tcpip_set_link_down(cyw43_t *self, int itf) {
    no_lwip_fail();
}
void __attribute__((weak)) cyw43_cb_process_ethernet(void *cb_data, int itf, size_t len, const uint8_t *buf) {
    no_lwip_fail();
}
#endif
