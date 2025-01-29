/*
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico.h"
#include "cyw43.h"
#include "btstack_config.h"
#include "hci.h"
#include "hci_transport.h"
#include "pico/btstack_hci_transport_cyw43.h"
#include "pico/btstack_chipset_cyw43.h"

// cyw43_bluetooth_hci_write and cyw43_bluetooth_hci_read require a custom 4-byte packet header in front of the actual HCI packet
// the HCI packet type is stored in the fourth byte of the packet header
#define CYW43_PACKET_HEADER_SIZE 4

// assert outgoing pre-buffer for cyw43 header is available
#if !defined(HCI_OUTGOING_PRE_BUFFER_SIZE) || (HCI_OUTGOING_PRE_BUFFER_SIZE < CYW43_PACKET_HEADER_SIZE)
#error HCI_OUTGOING_PRE_BUFFER_SIZE not defined or smaller than 4 (CYW43_PACKET_HEADER_SIZE) bytes. Please update btstack_config.h
#endif

// assert outgoing packet fragments are word aligned
#if !defined(HCI_ACL_CHUNK_SIZE_ALIGNMENT) || ((HCI_ACL_CHUNK_SIZE_ALIGNMENT & 3) != 0)
#error HCI_ACL_CHUNK_SIZE_ALIGNMENT not defined or not a multiple of 4. Please update btstack_config.h
#endif

// ensure incoming pre-buffer for cyw43 header is available (defaults from btstack/src/hci.h)
#if HCI_INCOMING_PRE_BUFFER_SIZE < CYW43_PACKET_HEADER_SIZE
#undef HCI_INCOMING_PRE_BUFFER_SIZE
#define HCI_INCOMING_PRE_BUFFER_SIZE CYW43_PACKET_HEADER_SIZE
#endif

// ensure buffer for cyw43_bluetooth_hci_read starts word aligned (word align pre buffer)
#define HCI_INCOMING_PRE_BUFFER_SIZE_ALIGNED ((HCI_INCOMING_PRE_BUFFER_SIZE + 3) & ~3)

#define BT_DEBUG_ENABLED 0
#if BT_DEBUG_ENABLED
#define BT_DEBUG(...) CYW43_PRINTF(__VA_ARGS__)
#else
#define BT_DEBUG(...) (void)0
#endif

// Callback when we have data
static void (*hci_transport_cyw43_packet_handler)(uint8_t packet_type, uint8_t *packet, uint16_t size) = NULL;

// The incoming packet buffer consist of a pre-buffer and the actual HCI packet
// For the call to cyw43_bluetooth_hci_read, the last 4 bytes (CY43_PACKET_HEADER_SIZE) of the pre-buffer is used for the CYW43 packet header
// After that, only the actual HCI packet is forwarded to BTstack, which expects HCI_INCOMING_PACKET_BUFFER_SIZE of pre-buffer bytes for its own use.
__attribute__((aligned(4)))
static uint8_t hci_packet_with_pre_buffer[HCI_INCOMING_PRE_BUFFER_SIZE_ALIGNED + HCI_INCOMING_PACKET_BUFFER_SIZE ];
static uint8_t * cyw43_receive_buffer = &hci_packet_with_pre_buffer[HCI_INCOMING_PRE_BUFFER_SIZE_ALIGNED - CYW43_PACKET_HEADER_SIZE];

static btstack_data_source_t transport_data_source;
static bool hci_transport_ready;

// Forward declaration
static void hci_transport_cyw43_process(void);

static void hci_transport_data_source_process(btstack_data_source_t *ds, btstack_data_source_callback_type_t callback_type) {
    assert(callback_type == DATA_SOURCE_CALLBACK_POLL);
    assert(ds == &transport_data_source);
    (void)callback_type;
    (void)ds;
    hci_transport_cyw43_process();
}

static void hci_transport_cyw43_init(const void *transport_config) {
    UNUSED(transport_config);
}

static int hci_transport_cyw43_open(void) {
    int err = cyw43_bluetooth_hci_init();
    if (err != 0) {
        CYW43_PRINTF("Failed to open cyw43 hci controller: %d\n", err);
        return err;
    }

    // OTP should be set in which case BT gets an address of wifi mac + 1
    // If OTP is not set for some reason BT gets set to 43:43:A2:12:1F:AC.
    // So for safety, set the bluetooth device address here.
    bd_addr_t addr;
    cyw43_hal_get_mac(0, (uint8_t*)&addr);
    addr[BD_ADDR_LEN - 1]++;
    hci_set_chipset(btstack_chipset_cyw43_instance());
    hci_set_bd_addr(addr);

    btstack_run_loop_set_data_source_handler(&transport_data_source, &hci_transport_data_source_process);
    btstack_run_loop_enable_data_source_callbacks(&transport_data_source, DATA_SOURCE_CALLBACK_POLL);
    btstack_run_loop_add_data_source(&transport_data_source);
    hci_transport_ready = true;

    return 0;
}

static int hci_transport_cyw43_close(void) {
    btstack_run_loop_disable_data_source_callbacks(&transport_data_source, DATA_SOURCE_CALLBACK_POLL);
    btstack_run_loop_remove_data_source(&transport_data_source);
    hci_transport_ready = false;

    return 0;
}

static void hci_transport_cyw43_register_packet_handler(void (*handler)(uint8_t packet_type, uint8_t *packet, uint16_t size)) {
    hci_transport_cyw43_packet_handler = handler;
}

static int hci_transport_cyw43_can_send_now(uint8_t packet_type) {
    UNUSED(packet_type);
    return true;
}

static int hci_transport_cyw43_send_packet(uint8_t packet_type, uint8_t *packet, int size) {
    // store packet type before actual data and increase size
    // This relies on HCI_OUTGOING_PRE_BUFFER_SIZE being set
    uint8_t *buffer = &packet[-CYW43_PACKET_HEADER_SIZE];
    uint32_t buffer_size = size + CYW43_PACKET_HEADER_SIZE;
    buffer[3] = packet_type;

    CYW43_THREAD_ENTER
    int err = cyw43_bluetooth_hci_write(buffer, buffer_size);

    if (err != 0) {
        CYW43_PRINTF("Failed to send cyw43 hci packet: %d\n", err);
        assert(false);
    } else {
        BT_DEBUG("bt sent %lu\n", buffer_size);
        static uint8_t packet_sent_event[] = { HCI_EVENT_TRANSPORT_PACKET_SENT, 0};
        hci_transport_cyw43_packet_handler(HCI_EVENT_PACKET, &packet_sent_event[0], sizeof(packet_sent_event));
    }
    CYW43_THREAD_EXIT
    return err;
}

// configure and return hci transport singleton
static const hci_transport_t hci_transport_cyw43 = {
        /* const char * name; */                                        "CYW43",
        /* void   (*init) (const void *transport_config); */            &hci_transport_cyw43_init,
        /* int    (*open)(void); */                                     &hci_transport_cyw43_open,
        /* int    (*close)(void); */                                    &hci_transport_cyw43_close,
        /* void   (*register_packet_handler)(void (*handler)(...); */   &hci_transport_cyw43_register_packet_handler,
        /* int    (*can_send_packet_now)(uint8_t packet_type); */       &hci_transport_cyw43_can_send_now,
        /* int    (*send_packet)(...); */                               &hci_transport_cyw43_send_packet,
        /* int    (*set_baudrate)(uint32_t baudrate); */                NULL,
        /* void   (*reset_link)(void); */                               NULL,
        /* void   (*set_sco_config)(uint16_t voice_setting, int num_connections); */ NULL,
};

const hci_transport_t *hci_transport_cyw43_instance(void) {
    return &hci_transport_cyw43;
}

// Called to perform bt work from a data source
static void hci_transport_cyw43_process(void) {
    CYW43_THREAD_LOCK_CHECK
    uint32_t len = 0;
    bool has_work;
#ifdef PICO_BTSTACK_CYW43_MAX_HCI_PROCESS_LOOP_COUNT
    uint32_t loop_count = 0;
#endif
    do {
        int err = cyw43_bluetooth_hci_read(cyw43_receive_buffer, CYW43_PACKET_HEADER_SIZE + HCI_INCOMING_PACKET_BUFFER_SIZE , &len);
        BT_DEBUG("bt in len=%lu err=%d\n", len, err);
        if (err == 0 && len > 0) {
            hci_transport_cyw43_packet_handler(cyw43_receive_buffer[3], &cyw43_receive_buffer[CYW43_PACKET_HEADER_SIZE], len - CYW43_PACKET_HEADER_SIZE);
            has_work = true;
        } else {
            has_work = false;
        }
// PICO_CONFIG: PICO_BTSTACK_CYW43_MAX_HCI_PROCESS_LOOP_COUNT, limit the max number of iterations of the hci processing loop, type=int, advanced=true, group=pico_btstack
#ifdef PICO_BTSTACK_CYW43_MAX_HCI_PROCESS_LOOP_COUNT
        if (++loop_count >= PICO_BTSTACK_CYW43_MAX_HCI_PROCESS_LOOP_COUNT) {
            break;
        }
#endif
    } while (has_work);
}

// This is called from cyw43_poll_func.
void cyw43_bluetooth_hci_process(void) {
    if (hci_transport_ready) {
        btstack_run_loop_poll_data_sources_from_irq();
    }
}
