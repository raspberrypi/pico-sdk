/*
 * Copyright (C) 2019 BlueKitchen GmbH
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 4. Any redistribution, use, or modification is done solely for
 *    personal benefit and not for any commercial purpose or for
 *    monetary gain.
 *
 * THIS SOFTWARE IS PROVIDED BY BLUEKITCHEN GMBH AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL BLUEKITCHEN
 * GMBH OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Please inquire about commercial licensing options at 
 * contact@bluekitchen-gmbh.com
 *
 */

/**
 * Basic Mesh Node demo
 */

#define BTSTACK_FILE__ "mesh_node_demo.c"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "btstack.h"
#include "mesh_node_demo.h"

const char * device_uuid_string = "001BDC0810210B0E0A0C000B0E0A0C00";

// general
#define MESH_BLUEKITCHEN_MODEL_ID_TEST_SERVER   0x0000u

static mesh_model_t                 mesh_vendor_model;

static mesh_model_t                 mesh_generic_on_off_server_model;
static mesh_generic_on_off_state_t  mesh_generic_on_off_state;

static char gap_name_buffer[] = "Mesh 00:00:00:00:00:00";

static btstack_packet_callback_registration_t hci_event_callback_registration;

#ifdef ENABLE_MESH_GATT_BEARER
static void packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
    UNUSED(channel);
    UNUSED(size);

    if (packet_type != HCI_EVENT_PACKET) return;

    bd_addr_t addr;
    
    switch (hci_event_packet_get_type(packet)) {
        case BTSTACK_EVENT_STATE:
            if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING) break;
            // setup gap name from local address
            gap_local_bd_addr(addr);
            btstack_replace_bd_addr_placeholder((uint8_t*)gap_name_buffer, sizeof(gap_name_buffer), addr);
            break;
        default:
            break;
    }
}

static uint16_t att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size){
    UNUSED(connection_handle);
    if (att_handle == ATT_CHARACTERISTIC_GAP_DEVICE_NAME_01_VALUE_HANDLE){
        return att_read_callback_handle_blob((const uint8_t *)gap_name_buffer, (uint16_t) strlen(gap_name_buffer), offset, buffer, buffer_size);
    }
    return 0;
}
#endif

static void mesh_provisioning_message_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
    UNUSED(packet_type);
    UNUSED(channel);
    UNUSED(size);

    if (packet_type != HCI_EVENT_PACKET) return;

    switch(packet[0]){
        case HCI_EVENT_MESH_META:
            switch(packet[2]){
                case MESH_SUBEVENT_PB_TRANSPORT_LINK_OPEN:
                    printf("Provisioner link opened");
                    break;
                case MESH_SUBEVENT_ATTENTION_TIMER:
                    printf("Attention Timer: %u\n", mesh_subevent_attention_timer_get_attention_time(packet));
                    break;
                case MESH_SUBEVENT_PB_TRANSPORT_LINK_CLOSED:
                    printf("Provisioner link close");
                    break;
                case MESH_SUBEVENT_PB_PROV_COMPLETE:
                    printf("Provisioning complete\n");
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}

static void mesh_state_update_message_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
    UNUSED(channel);
    UNUSED(size);

    if (packet_type != HCI_EVENT_PACKET) return;
   
    switch(packet[0]){
        case HCI_EVENT_MESH_META:
            switch(packet[2]){
                case MESH_SUBEVENT_STATE_UPDATE_BOOL:
                    printf("State update: model identifier 0x%08x, state identifier 0x%08x, reason %u, state %u\n",
                        mesh_subevent_state_update_bool_get_model_identifier(packet),
                        mesh_subevent_state_update_bool_get_state_identifier(packet),
                        mesh_subevent_state_update_bool_get_reason(packet),
                        mesh_subevent_state_update_bool_get_value(packet));
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}

static void show_usage(void){
    bd_addr_t      iut_address;
    gap_local_bd_addr(iut_address);
    printf("\n--- Bluetooth Mesh Console at %s ---\n", bd_addr_to_str(iut_address));
    printf("8      - Delete provisioning data\n");
    printf("g      - Generic ON/OFF Server Toggle Value\n");
    printf("\n");
}

static void stdin_process(char cmd){
    switch (cmd){
        case '8':
            mesh_node_reset();
            printf("Mesh Node Reset!\n");
            mesh_proxy_start_advertising_unprovisioned_device();
            break;
        case 'g':
            printf("Generic ON/OFF Server Toggle Value\n");
            mesh_generic_on_off_server_set(&mesh_generic_on_off_server_model, 1-mesh_generic_on_off_server_get(&mesh_generic_on_off_server_model), 0, 0);
            break;
        case ' ':
            show_usage();
            break;
        default:
            printf("Command: '%c' not implemented\n", cmd);
            show_usage();
            break;
    }
}

static int scan_hex_byte(const char * byte_string){
    int upper_nibble = nibble_for_char(*byte_string++);
    if (upper_nibble < 0) return -1;
    int lower_nibble = nibble_for_char(*byte_string);
    if (lower_nibble < 0) return -1;
    return (upper_nibble << 4) | lower_nibble;
}

static int btstack_parse_hex(const char * string, uint16_t len, uint8_t * buffer){
    int i;
    for (i = 0; i < len; i++) {
        int single_byte = scan_hex_byte(string);
        if (single_byte < 0) return 0;
        string += 2;
        buffer[i] = (uint8_t)single_byte;
        // don't check seperator after last byte
        if (i == len - 1) {
            return 1;
        }
        // optional seperator
        char separator = *string;
        if (separator == ':' && separator == '-' && separator == ' ') {
            string++;
        }
    }
    return 1;
}

int btstack_main(void);
int btstack_main(void)
{
#ifdef HAVE_BTSTACK_STDIN
    // console
    btstack_stdin_setup(stdin_process);
#endif

    // crypto
    btstack_crypto_init();

#ifdef ENABLE_MESH_GATT_BEARER
    // l2cap
    l2cap_init();

    // setup ATT server
    att_server_init(profile_data, &att_read_callback, NULL);    

    // 
    sm_init();
#endif

#ifdef ENABLE_MESH_GATT_BEARER
    // register for HCI events
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);
#endif

    // mesh
    mesh_init();

#ifdef ENABLE_MESH_GATT_BEARER
    // setup connectable advertisments
    bd_addr_t null_addr;
    memset(null_addr, 0, 6);
    uint8_t adv_type = 0;   // AFV_IND
    uint16_t adv_int_min = 0x0030;
    uint16_t adv_int_max = 0x0030;
    adv_bearer_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);
#endif

    // Track Provisioning as device role
    mesh_register_provisioning_device_packet_handler(&mesh_provisioning_message_handler);

    // Loc - bottom - https://www.bluetooth.com/specifications/assigned-numbers/gatt-namespace-descriptors
    mesh_node_set_element_location(mesh_node_get_primary_element(), 0x103);

    // Setup Generic On/Off model
    mesh_generic_on_off_server_model.model_identifier = mesh_model_get_model_identifier_bluetooth_sig(MESH_SIG_MODEL_ID_GENERIC_ON_OFF_SERVER);
    mesh_generic_on_off_server_model.operations = mesh_generic_on_off_server_get_operations();    
    mesh_generic_on_off_server_model.model_data = (void *) &mesh_generic_on_off_state;
    mesh_generic_on_off_server_register_packet_handler(&mesh_generic_on_off_server_model, &mesh_state_update_message_handler);
    mesh_element_add_model(mesh_node_get_primary_element(), &mesh_generic_on_off_server_model);

    // Setup our custom model
    mesh_vendor_model.model_identifier = mesh_model_get_model_identifier(BLUETOOTH_COMPANY_ID_BLUEKITCHEN_GMBH, MESH_BLUEKITCHEN_MODEL_ID_TEST_SERVER);
    mesh_element_add_model(mesh_node_get_primary_element(), &mesh_vendor_model);
    
    // Enable Output OOB
    provisioning_device_set_output_oob_actions(0x08, 0x08);

    // Enable PROXY
    mesh_foundation_gatt_proxy_set(1);
    
#if defined(ENABLE_MESH_ADV_BEARER)
    // setup scanning when supporting ADV Bearer
    gap_set_scan_parameters(0, 0x300, 0x300);
    gap_start_scan();
#endif

    uint8_t device_uuid[16];
    btstack_parse_hex(device_uuid_string, 16, device_uuid);
    mesh_node_set_device_uuid(device_uuid);

    // turn on!
	hci_power_control(HCI_POWER_ON);
	    
    return 0;
}

int main(int argc, char *argv[]) {
    btstack_main();
    return 0;
}
/* EXAMPLE_END */
