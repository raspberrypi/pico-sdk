/**
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "tusb.h"

#if !defined(LIB_TINYUSB_HOST) || (defined(LIB_TINYUSB_HOST) && defined(CFG_TUH_RPI_PIO_USB))
#include "pico/bootrom.h"
#include "pico/usb_reset.h"

#if PICO_ENABLE_USB_RESET_VIA_VENDOR_INTERFACE && !(PICO_USB_RESET_SUPPORT_RESET_TO_BOOTSEL || PICO_USB_RESET_SUPPORT_RESET_TO_FLASH_BOOT)
#warning PICO_ENABLE_USB_RESET_VIA_VENDOR_INTERFACE has been selected but neither PICO_USB_RESET_SUPPORT_RESET_TO_BOOTSEL nor PICO_USB_RESET_SUPPORT_RESET_TO_FLASH_BOOT have been selected.
#endif

#if PICO_ENABLE_USB_RESET_VIA_VENDOR_INTERFACE
#include "hardware/watchdog.h"

static uint8_t itf_num;

#if PICO_USB_RESET_SUPPORT_MS_OS_20_DESCRIPTOR
// Support for Microsoft OS 2.0 descriptor
#define BOS_TOTAL_LEN      (TUD_BOS_DESC_LEN + TUD_BOS_MICROSOFT_OS_DESC_LEN)

#define MS_OS_20_DESC_LEN  (RPI_RESET_MS_OS_20_DESC_LEN + 0x0A)

uint8_t const desc_bos[] =
{
    // total length, number of device caps
    TUD_BOS_DESCRIPTOR(BOS_TOTAL_LEN, 1),

    // Microsoft OS 2.0 descriptor
    TUD_BOS_MS_OS_20_DESCRIPTOR(MS_OS_20_DESC_LEN, 1)
};

TU_VERIFY_STATIC(sizeof(desc_bos) == BOS_TOTAL_LEN, "Incorrect size");

uint8_t const * tud_descriptor_bos_cb(void)
{
    return desc_bos;
}

static const uint8_t desc_ms_os_20[] =
{
    // Set header: length, type, windows version, total length
    U16_TO_U8S_LE(0x000A), U16_TO_U8S_LE(MS_OS_20_SET_HEADER_DESCRIPTOR), U32_TO_U8S_LE(0x06030000), U16_TO_U8S_LE(MS_OS_20_DESC_LEN),

    RPI_RESET_MS_OS_20_DESCRIPTOR(PICO_USB_RESET_MS_OS_20_DESCRIPTOR_ITF)
};

TU_VERIFY_STATIC(sizeof(desc_ms_os_20) == MS_OS_20_DESC_LEN, "Incorrect size");

bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request)
{
    // nothing to with DATA & ACK stage
    if (stage != CONTROL_STAGE_SETUP) return true;

    if (request->bRequest == 1 && request->wIndex == 7) {
        // Get Microsoft OS 2.0 compatible descriptor
        return tud_control_xfer(rhport, request, (void*)(uintptr_t) desc_ms_os_20, sizeof(desc_ms_os_20));
    } else {
        return false;
    }

    // stall unknown request
    return false;
}
#endif

void usb_reset_interface_init(void) {
}

void usb_reset_interface_reset(uint8_t __unused rhport) {
    itf_num = 0;
}

uint16_t usb_reset_interface_open(uint8_t __unused rhport, tusb_desc_interface_t const *itf_desc, uint16_t max_len) {
    TU_VERIFY(TUSB_CLASS_VENDOR_SPECIFIC == itf_desc->bInterfaceClass &&
              RESET_INTERFACE_SUBCLASS == itf_desc->bInterfaceSubClass &&
              RESET_INTERFACE_PROTOCOL == itf_desc->bInterfaceProtocol, 0);

    uint16_t const drv_len = sizeof(tusb_desc_interface_t);
    TU_VERIFY(max_len >= drv_len, 0);

    itf_num = itf_desc->bInterfaceNumber;
    return drv_len;
}

// Support for parameterized reset via vendor interface control request
bool usb_reset_interface_control_xfer_cb(uint8_t __unused rhport, uint8_t stage, tusb_control_request_t const * request) {
    // nothing to do with DATA & ACK stage
    if (stage != CONTROL_STAGE_SETUP) return true;

    if (request->wIndex == itf_num) {

#if PICO_USB_RESET_SUPPORT_RESET_TO_BOOTSEL
        if (request->bRequest == RESET_REQUEST_BOOTSEL) {
#ifdef PICO_USB_RESET_BOOTSEL_ACTIVITY_LED
            int gpio = PICO_USB_RESET_BOOTSEL_ACTIVITY_LED;
            bool active_low = PICO_USB_RESET_BOOTSEL_ACTIVITY_LED_ACTIVE_LOW;
#else
            int gpio = -1;
            bool active_low = false;
#endif
#if !PICO_USB_RESET_BOOTSEL_FIXED_ACTIVITY_LED
            // wValue layout:
            //   bits 0-1  : forwarded as the bootrom disable_interface_mask
            //   bit 7     : 1 if the activity-LED GPIO is active-low
            //   bit 8     : 1 if an activity-LED GPIO is being specified
            //   bits 9-15 : activity-LED GPIO number (0-127)
            if (request->wValue & (1u << 8)) {
                active_low = request->wValue & (1u << 7);
                gpio = request->wValue >> 9u;
            }
#endif
            rom_reset_usb_boot_extra(gpio, (request->wValue & 0x3) | PICO_USB_RESET_BOOTSEL_INTERFACE_DISABLE_MASK, active_low);
            // does not return, otherwise we'd return true
        }
#endif

#if PICO_USB_RESET_SUPPORT_RESET_TO_FLASH_BOOT
        if (request->bRequest == RESET_REQUEST_FLASH) {
            watchdog_reboot(0, 0, PICO_USB_RESET_RESET_TO_FLASH_DELAY_MS);
            return true;
        }
#endif

    }
    return false;
}

bool usb_reset_interface_xfer_cb(uint8_t __unused rhport, uint8_t __unused ep_addr, xfer_result_t __unused result, uint32_t __unused xferred_bytes) {
    return true;
}

#if PICO_USB_RESET_INCLUDE_DEFAULT_APP_DRIVER_CB
// Implement callback to add our custom driver
usbd_class_driver_t const *usbd_app_driver_get_cb(uint8_t *driver_count) {
    *driver_count = 1;
    return &usb_reset_interface_driver;
}
#endif
#endif

#if PICO_ENABLE_USB_RESET_VIA_BAUD_RATE
// Support for default BOOTSEL reset by changing baud rate
void tud_cdc_line_coding_cb(__unused uint8_t itf, cdc_line_coding_t const* p_line_coding) {
    if (p_line_coding->bit_rate == PICO_USB_RESET_MAGIC_BAUD_RATE) {
#ifdef PICO_USB_RESET_BOOTSEL_ACTIVITY_LED
        int gpio = PICO_USB_RESET_BOOTSEL_ACTIVITY_LED;
        bool active_low = PICO_USB_RESET_BOOTSEL_ACTIVITY_LED_ACTIVE_LOW;
#else
        int gpio = -1;
        bool active_low = false;
#endif
        rom_reset_usb_boot_extra(gpio, PICO_USB_RESET_BOOTSEL_INTERFACE_DISABLE_MASK, active_low);
    }
}
#endif
#endif // !defined(LIB_TINYUSB_HOST) || (defined(LIB_TINYUSB_HOST) && defined(CFG_TUH_RPI_PIO_USB))
