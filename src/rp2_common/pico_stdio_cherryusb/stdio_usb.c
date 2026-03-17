#ifndef LIB_CHERRYUSB_HOST

#if PICO_STDIO_CHERRYUSB_ENABLE_RESET_VIA_VENDOR_INTERFACE && !(PICO_STDIO_CHERRYUSB_RESET_INTERFACE_SUPPORT_RESET_TO_BOOTSEL || PICO_STDIO_CHERRYUSB_RESET_INTERFACE_SUPPORT_RESET_TO_FLASH_BOOT)
#warning PICO_STDIO_CHERRYUSB_ENABLE_RESET_VIA_VENDOR_INTERFACE has been selected but neither PICO_STDIO_CHERRYUSB_RESET_INTERFACE_SUPPORT_RESET_TO_BOOTSEL nor PICO_STDIO_CHERRYUSB_RESET_INTERFACE_SUPPORT_RESET_TO_FLASH_BOOT have been selected.
#endif

#include "usbd_core.h"
#include "usbd_cdc_acm.h"
#include "pico/stdio_usb.h"

#include "pico/binary_info.h"
#include "pico/time.h"
#include "pico/stdio/driver.h"
#include "pico/mutex.h"
#include "hardware/irq.h"

#include "pico/unique_id.h"

#if PICO_STDIO_CHERRYUSB_ENABLE_RESET_VIA_VENDOR_INTERFACE
#include "pico/stdio_usb/reset_interface.h"
#include "hardware/watchdog.h"
#include "pico/bootrom.h"
#endif

/*!< endpoint address */
#define CDC_IN_EP  0x81
#define CDC_OUT_EP 0x02
#define CDC_INT_EP 0x83

#ifndef USBD_VID
#define USBD_VID (0x2E8A) // Raspberry Pi
#endif

#ifndef USBD_PID
#if PICO_RP2040
#define USBD_PID (0x000a) // Raspberry Pi Pico SDK CDC for RP2040
#else
#define USBD_PID (0x0009) // Raspberry Pi Pico SDK CDC
#endif
#endif

#ifndef USBD_MANUFACTURER
#define USBD_MANUFACTURER "Raspberry Pi"
#endif

#ifndef USBD_PRODUCT
#define USBD_PRODUCT "Pico"
#endif

#define TUD_RPI_RESET_DESC_LEN 9
#if !PICO_STDIO_CHERRYUSB_DEVICE_SELF_POWERED
#define USBD_CONFIGURATION_DESCRIPTOR_ATTRIBUTE USB_CONFIG_BUS_POWERED
#define USBD_MAX_POWER_MA                       (250)
#else
#define USBD_CONFIGURATION_DESCRIPTOR_ATTRIBUTE USB_CONFIG_SELF_POWERED
#define USBD_MAX_POWER_MA                       (1)
#endif

#define USBD_ITF_CDC (0) // needs 2 interfaces
#if !PICO_STDIO_CHERRYUSB_ENABLE_RESET_VIA_VENDOR_INTERFACE
#define USBD_ITF_MAX (2)
#else
#define USBD_ITF_RPI_RESET (2)
#define USBD_ITF_MAX       (3)
#endif

#define USBD_LANGID_STRING 1033

#if !PICO_STDIO_CHERRYUSB_ENABLE_RESET_VIA_VENDOR_INTERFACE
/*!< config descriptor size */
#define USB_CONFIG_SIZE (9 + CDC_ACM_DESCRIPTOR_LEN)
#else
#define USB_CONFIG_SIZE (9 + CDC_ACM_DESCRIPTOR_LEN + TUD_RPI_RESET_DESC_LEN)
#endif

#ifdef CONFIG_USB_HS
#define CDC_MAX_MPS 512
#else
#define CDC_MAX_MPS 64
#endif

#define USBD_STR_0         (0x00)
#define USBD_STR_MANUF     (0x01)
#define USBD_STR_PRODUCT   (0x02)
#define USBD_STR_SERIAL    (0x03)
#define USBD_STR_CDC       (0x04)
#define USBD_STR_RPI_RESET (0x05)

#if PICO_STDIO_CHERRYUSB_ENABLE_RESET_VIA_VENDOR_INTERFACE && PICO_STDIO_CHERRYUSB_RESET_INTERFACE_SUPPORT_MS_OS_20_DESCRIPTOR
#define USBD_WINUSB_VENDOR_CODE 0x01

#define USBD_WEBUSB_ENABLE 0
#define USBD_BULK_ENABLE   1
#define USBD_WINUSB_ENABLE 1

/* WinUSB Microsoft OS 2.0 descriptor sizes */
#define WINUSB_DESCRIPTOR_SET_HEADER_SIZE  10
#define WINUSB_FUNCTION_SUBSET_HEADER_SIZE 8
#define WINUSB_FEATURE_COMPATIBLE_ID_SIZE  20

#define FUNCTION_SUBSET_LEN                160
#define DEVICE_INTERFACE_GUIDS_FEATURE_LEN 132

#define USBD_WINUSB_DESC_SET_LEN (WINUSB_DESCRIPTOR_SET_HEADER_SIZE + USBD_WEBUSB_ENABLE * FUNCTION_SUBSET_LEN + USBD_BULK_ENABLE * FUNCTION_SUBSET_LEN)

__ALIGN_BEGIN const uint8_t USBD_WinUSBDescriptorSetDescriptor[] = {
    WBVAL(WINUSB_DESCRIPTOR_SET_HEADER_SIZE), /* wLength */
    WBVAL(WINUSB_SET_HEADER_DESCRIPTOR_TYPE), /* wDescriptorType */
    0x00, 0x00, 0x03, 0x06, /* >= Win 8.1 */  /* dwWindowsVersion*/
    WBVAL(USBD_WINUSB_DESC_SET_LEN),          /* wDescriptorSetTotalLength */
#if (USBD_WEBUSB_ENABLE)
    WBVAL(WINUSB_FUNCTION_SUBSET_HEADER_SIZE), // wLength
    WBVAL(WINUSB_SUBSET_HEADER_FUNCTION_TYPE), // wDescriptorType
    0,                                         // bFirstInterface USBD_WINUSB_IF_NUM
    0,                                         // bReserved
    WBVAL(FUNCTION_SUBSET_LEN),                // wSubsetLength
    WBVAL(WINUSB_FEATURE_COMPATIBLE_ID_SIZE),  // wLength
    WBVAL(WINUSB_FEATURE_COMPATIBLE_ID_TYPE),  // wDescriptorType
    'W', 'I', 'N', 'U', 'S', 'B', 0, 0,        // CompatibleId
    0, 0, 0, 0, 0, 0, 0, 0,                    // SubCompatibleId
    WBVAL(DEVICE_INTERFACE_GUIDS_FEATURE_LEN), // wLength
    WBVAL(WINUSB_FEATURE_REG_PROPERTY_TYPE),   // wDescriptorType
    WBVAL(WINUSB_PROP_DATA_TYPE_REG_MULTI_SZ), // wPropertyDataType
    WBVAL(42),                                 // wPropertyNameLength
    'D', 0, 'e', 0, 'v', 0, 'i', 0, 'c', 0, 'e', 0,
    'I', 0, 'n', 0, 't', 0, 'e', 0, 'r', 0, 'f', 0, 'a', 0, 'c', 0, 'e', 0,
    'G', 0, 'U', 0, 'I', 0, 'D', 0, 's', 0, 0, 0,
    WBVAL(80), // wPropertyDataLength
    '{', 0,
    '9', 0, '2', 0, 'C', 0, 'E', 0, '6', 0, '4', 0, '6', 0, '2', 0, '-', 0,
    '9', 0, 'C', 0, '7', 0, '7', 0, '-', 0,
    '4', 0, '6', 0, 'F', 0, 'E', 0, '-', 0,
    '9', 0, '3', 0, '3', 0, 'B', 0, '-',
    0, '3', 0, '1', 0, 'C', 0, 'B', 0, '9', 0, 'C', 0, '5', 0, 'A', 0, 'A', 0, '3', 0, 'B', 0, '9', 0,
    '}', 0, 0, 0, 0, 0
#endif
#if USBD_BULK_ENABLE
    WBVAL(WINUSB_FUNCTION_SUBSET_HEADER_SIZE), /* wLength */
    WBVAL(WINUSB_SUBSET_HEADER_FUNCTION_TYPE), /* wDescriptorType */
    USBD_ITF_RPI_RESET,                        /* bFirstInterface USBD_ITF_RPI_RESET*/
    0,                                         /* bReserved */
    WBVAL(FUNCTION_SUBSET_LEN),                /* wSubsetLength */
    WBVAL(WINUSB_FEATURE_COMPATIBLE_ID_SIZE),  /* wLength */
    WBVAL(WINUSB_FEATURE_COMPATIBLE_ID_TYPE),  /* wDescriptorType */
    'W', 'I', 'N', 'U', 'S', 'B', 0, 0,        /* CompatibleId*/
    0, 0, 0, 0, 0, 0, 0, 0,                    /* SubCompatibleId*/
    WBVAL(DEVICE_INTERFACE_GUIDS_FEATURE_LEN), /* wLength */
    WBVAL(WINUSB_FEATURE_REG_PROPERTY_TYPE),   /* wDescriptorType */
    WBVAL(WINUSB_PROP_DATA_TYPE_REG_MULTI_SZ), /* wPropertyDataType */
    WBVAL(42),                                 /* wPropertyNameLength */
    'D', 0, 'e', 0, 'v', 0, 'i', 0, 'c', 0, 'e', 0,
    'I', 0, 'n', 0, 't', 0, 'e', 0, 'r', 0, 'f', 0, 'a', 0, 'c', 0, 'e', 0,
    'G', 0, 'U', 0, 'I', 0, 'D', 0, 's', 0, 0, 0,
    WBVAL(80), /* wPropertyDataLength */
    '{', 0,
    'C', 0, 'D', 0, 'B', 0, '3', 0, 'B', 0, '5', 0, 'A', 0, 'D', 0, '-', 0,
    '2', 0, '9', 0, '3', 0, 'B', 0, '-', 0,
    '4', 0, '6', 0, '6', 0, '3', 0, '-', 0,
    'A', 0, 'A', 0, '3', 0, '6', 0, '-',
    0, '1', 0, 'A', 0, 'A', 0, 'E', 0, '4', 0, '6', 0, '4', 0, '6', 0, '3', 0, '7', 0, '7', 0, '6', 0,
    '}', 0, 0, 0, 0, 0
#endif
};

#define USBD_NUM_DEV_CAPABILITIES (USBD_WEBUSB_ENABLE + USBD_WINUSB_ENABLE)

#define USBD_WEBUSB_DESC_LEN 24
#define USBD_WINUSB_DESC_LEN 28

#define USBD_BOS_WTOTALLENGTH (0x05 +                                      \
                               USBD_WEBUSB_DESC_LEN * USBD_WEBUSB_ENABLE + \
                               USBD_WINUSB_DESC_LEN * USBD_WINUSB_ENABLE)

__ALIGN_BEGIN const uint8_t USBD_BinaryObjectStoreDescriptor[] = {
    0x05,                         /* bLength */
    0x0f,                         /* bDescriptorType */
    WBVAL(USBD_BOS_WTOTALLENGTH), /* wTotalLength */
    USBD_NUM_DEV_CAPABILITIES,    /* bNumDeviceCaps */
#if (USBD_WEBUSB_ENABLE)
    USBD_WEBUSB_DESC_LEN,           /* bLength */
    0x10,                           /* bDescriptorType */
    USB_DEVICE_CAPABILITY_PLATFORM, /* bDevCapabilityType */
    0x00,                           /* bReserved */
    0x38, 0xB6, 0x08, 0x34,         /* PlatformCapabilityUUID */
    0xA9, 0x09, 0xA0, 0x47,
    0x8B, 0xFD, 0xA0, 0x76,
    0x88, 0x15, 0xB6, 0x65,
    WBVAL(0x0100), /* 1.00 */ /* bcdVersion */
    USBD_WINUSB_VENDOR_CODE,  /* bVendorCode */
    0,                        /* iLandingPage */
#endif
#if (USBD_WINUSB_ENABLE)
    USBD_WINUSB_DESC_LEN,           /* bLength */
    0x10,                           /* bDescriptorType */
    USB_DEVICE_CAPABILITY_PLATFORM, /* bDevCapabilityType */
    0x00,                           /* bReserved */
    0xDF, 0x60, 0xDD, 0xD8,         /* PlatformCapabilityUUID */
    0x89, 0x45, 0xC7, 0x4C,
    0x9C, 0xD2, 0x65, 0x9D,
    0x9E, 0x64, 0x8A, 0x9F,
    0x00, 0x00, 0x03, 0x06, /* >= Win 8.1 */ /* dwWindowsVersion*/
    WBVAL(USBD_WINUSB_DESC_SET_LEN),         /* wDescriptorSetTotalLength */
    USBD_WINUSB_VENDOR_CODE,                 /* bVendorCode */
    0,                                       /* bAltEnumCode */
#endif
};

struct usb_msosv2_descriptor msosv2_desc = {
    .vendor_code = USBD_WINUSB_VENDOR_CODE,
    .compat_id = USBD_WinUSBDescriptorSetDescriptor,
    .compat_id_len = USBD_WINUSB_DESC_SET_LEN,
};

struct usb_bos_descriptor bos_desc = {
    .string = USBD_BinaryObjectStoreDescriptor,
    .string_len = USBD_BOS_WTOTALLENGTH
};

static int rp_vendor_request_handler(uint8_t busid, struct usb_setup_packet *setup, uint8_t **data, uint32_t *len) {
    if (setup->wIndex == USBD_ITF_RPI_RESET) {
#if PICO_STDIO_CHERRYUSB_RESET_INTERFACE_SUPPORT_RESET_TO_BOOTSEL
        if (setup->bRequest == RESET_REQUEST_BOOTSEL) {
#ifdef PICO_STDIO_CHERRYUSB_RESET_BOOTSEL_ACTIVITY_LED
            int gpio = PICO_STDIO_CHERRYUSB_RESET_BOOTSEL_ACTIVITY_LED;
            bool active_low = PICO_STDIO_CHERRYUSB_RESET_BOOTSEL_ACTIVITY_LED_ACTIVE_LOW;
#else
            int gpio = -1;
            bool active_low = false;
#endif
#if !PICO_STDIO_CHERRYUSB_RESET_BOOTSEL_FIXED_ACTIVITY_LED
            if (setup->wValue & 0x100) {
                gpio = setup->wValue >> 9u;
            }
            active_low = setup->wValue & 0x200;
#endif
            rom_reset_usb_boot_extra(gpio, (setup->wValue & 0x7f) | PICO_STDIO_CHERRYUSB_RESET_BOOTSEL_INTERFACE_DISABLE_MASK, active_low);
            // does not return, otherwise we'd return true
        }
#endif

#if PICO_STDIO_CHERRYUSB_RESET_INTERFACE_SUPPORT_RESET_TO_FLASH_BOOT
        if (setup->bRequest == RESET_REQUEST_FLASH) {
            watchdog_reboot(0, 0, PICO_STDIO_CHERRYUSB_RESET_RESET_TO_FLASH_DELAY_MS);
            return 0;
        }
#endif
    } else {
        return -1;
    }
}
#endif

#ifdef CONFIG_USBDEV_ADVANCE_DESC
static const uint8_t device_descriptor[] = {
#if PICO_STDIO_CHERRYUSB_ENABLE_RESET_VIA_VENDOR_INTERFACE && PICO_STDIO_CHERRYUSB_RESET_INTERFACE_SUPPORT_MS_OS_20_DESCRIPTOR
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_1, 0xEF, 0x02, 0x01, USBD_VID, USBD_PID, 0x0100, 0x01),
#else
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0xEF, 0x02, 0x01, USBD_VID, USBD_PID, 0x0100, 0x01),
#endif
};

static const uint8_t config_descriptor[] = {
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, USBD_ITF_MAX, 0x01, USBD_CONFIGURATION_DESCRIPTOR_ATTRIBUTE, USBD_MAX_POWER_MA),
    CDC_ACM_DESCRIPTOR_INIT(USBD_ITF_CDC, CDC_INT_EP, CDC_OUT_EP, CDC_IN_EP, CDC_MAX_MPS, USBD_STR_CDC),
#if PICO_STDIO_CHERRYUSB_ENABLE_RESET_VIA_VENDOR_INTERFACE
    USB_INTERFACE_DESCRIPTOR_INIT(USBD_ITF_RPI_RESET, 0x00, 0x00, 0xFF, RESET_INTERFACE_SUBCLASS, RESET_INTERFACE_PROTOCOL, USBD_STR_RPI_RESET),
#endif
};

static const uint8_t device_quality_descriptor[] = {
    ///////////////////////////////////////
    /// device qualifier descriptor
    ///////////////////////////////////////
    0x0a,
    USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER,
    0x00,
    0x02,
    0x00,
    0x00,
    0x00,
    0x40,
    0x00,
    0x00,
};

static char usbd_serial_str[PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2 + 1];

static const char *string_descriptors[] = {
    [USBD_STR_0] = (const char[]){ 0x09, 0x04 },
    [USBD_STR_MANUF] = USBD_MANUFACTURER,
    [USBD_STR_PRODUCT] = USBD_PRODUCT,
    [USBD_STR_SERIAL] = usbd_serial_str,
    [USBD_STR_CDC] = "Board CDC",
#if PICO_STDIO_CHERRYUSB_ENABLE_RESET_VIA_VENDOR_INTERFACE
    [USBD_STR_RPI_RESET] = "Reset",
#endif
};

static const uint8_t *device_descriptor_callback(uint8_t speed) {
    return device_descriptor;
}

static const uint8_t *config_descriptor_callback(uint8_t speed) {
    return config_descriptor;
}

static const uint8_t *device_quality_descriptor_callback(uint8_t speed) {
    return device_quality_descriptor;
}

static const char *string_descriptor_callback(uint8_t speed, uint8_t index) {
    if (index > 5) {
        return NULL;
    }
    // Assign the SN using the unique flash id
    if (!usbd_serial_str[0]) {
        pico_get_unique_board_id_string(usbd_serial_str, sizeof(usbd_serial_str));
    }

    return string_descriptors[index];
}

const struct usb_descriptor cdc_descriptor = {
    .device_descriptor_callback = device_descriptor_callback,
    .config_descriptor_callback = config_descriptor_callback,
    .device_quality_descriptor_callback = device_quality_descriptor_callback,
    .string_descriptor_callback = string_descriptor_callback,
#if PICO_STDIO_CHERRYUSB_ENABLE_RESET_VIA_VENDOR_INTERFACE && PICO_STDIO_CHERRYUSB_RESET_INTERFACE_SUPPORT_MS_OS_20_DESCRIPTOR
    .msosv2_descriptor = &msosv2_desc,
    .bos_descriptor = &bos_desc
#endif
};
#else
/*!< global descriptor */
static const uint8_t cdc_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0xEF, 0x02, 0x01, USBD_VID, USBD_PID, 0x0100, 0x01),
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x02, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    CDC_ACM_DESCRIPTOR_INIT(0x00, CDC_INT_EP, CDC_OUT_EP, CDC_IN_EP, CDC_MAX_MPS, 0x02),
    ///////////////////////////////////////
    /// string0 descriptor
    ///////////////////////////////////////
    USB_LANGID_INIT(USBD_LANGID_STRING),
    ///////////////////////////////////////
    /// string1 descriptor
    ///////////////////////////////////////
    0x14,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'C', 0x00,                  /* wcChar0 */
    'h', 0x00,                  /* wcChar1 */
    'e', 0x00,                  /* wcChar2 */
    'r', 0x00,                  /* wcChar3 */
    'r', 0x00,                  /* wcChar4 */
    'y', 0x00,                  /* wcChar5 */
    'U', 0x00,                  /* wcChar6 */
    'S', 0x00,                  /* wcChar7 */
    'B', 0x00,                  /* wcChar8 */
    ///////////////////////////////////////
    /// string2 descriptor
    ///////////////////////////////////////
    0x26,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'C', 0x00,                  /* wcChar0 */
    'h', 0x00,                  /* wcChar1 */
    'e', 0x00,                  /* wcChar2 */
    'r', 0x00,                  /* wcChar3 */
    'r', 0x00,                  /* wcChar4 */
    'y', 0x00,                  /* wcChar5 */
    'U', 0x00,                  /* wcChar6 */
    'S', 0x00,                  /* wcChar7 */
    'B', 0x00,                  /* wcChar8 */
    ' ', 0x00,                  /* wcChar9 */
    'C', 0x00,                  /* wcChar10 */
    'D', 0x00,                  /* wcChar11 */
    'C', 0x00,                  /* wcChar12 */
    ' ', 0x00,                  /* wcChar13 */
    'D', 0x00,                  /* wcChar14 */
    'E', 0x00,                  /* wcChar15 */
    'M', 0x00,                  /* wcChar16 */
    'O', 0x00,                  /* wcChar17 */
    ///////////////////////////////////////
    /// string3 descriptor
    ///////////////////////////////////////
    0x16,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    '2', 0x00,                  /* wcChar0 */
    '0', 0x00,                  /* wcChar1 */
    '2', 0x00,                  /* wcChar2 */
    '2', 0x00,                  /* wcChar3 */
    '1', 0x00,                  /* wcChar4 */
    '2', 0x00,                  /* wcChar5 */
    '3', 0x00,                  /* wcChar6 */
    '4', 0x00,                  /* wcChar7 */
    '5', 0x00,                  /* wcChar8 */
    '6', 0x00,                  /* wcChar9 */
#ifdef CONFIG_USB_HS
    ///////////////////////////////////////
    /// device qualifier descriptor
    ///////////////////////////////////////
    0x0a,
    USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER,
    0x00,
    0x02,
    0x00,
    0x00,
    0x00,
    0x40,
    0x00,
    0x00,
#endif
    0x00
};
#endif

USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t g_usb_read_buffer[CDC_MAX_MPS];

volatile bool g_usb_tx_busy_flag = false;
volatile uint32_t g_usb_rx_count = 0;
volatile uint32_t g_usb_rx_offset = 0;

static void usbd_event_handler(uint8_t busid, uint8_t event) {
    switch (event) {
        case USBD_EVENT_RESET:
            g_usb_tx_busy_flag = false;
            g_usb_rx_offset = 0;
            g_usb_rx_count = 0;
            break;
        case USBD_EVENT_CONNECTED:
            break;
        case USBD_EVENT_DISCONNECTED:
            break;
        case USBD_EVENT_RESUME:
            break;
        case USBD_EVENT_SUSPEND:
            break;
        case USBD_EVENT_CONFIGURED:
            g_usb_tx_busy_flag = false;
            /* setup first out ep read transfer */
            usbd_ep_start_read(busid, CDC_OUT_EP, g_usb_read_buffer, CDC_MAX_MPS);
            break;
        case USBD_EVENT_SET_REMOTE_WAKEUP:
            break;
        case USBD_EVENT_CLR_REMOTE_WAKEUP:
            break;

        default:
            break;
    }
}

void usbd_cdc_acm_bulk_out(uint8_t busid, uint8_t ep, uint32_t nbytes) {
    g_usb_rx_count = nbytes;
    g_usb_rx_offset = 0;
}

void usbd_cdc_acm_bulk_in(uint8_t busid, uint8_t ep, uint32_t nbytes) {
    if ((nbytes % usbd_get_ep_mps(busid, ep)) == 0 && nbytes) {
        /* send zlp */
        usbd_ep_start_write(busid, CDC_IN_EP, NULL, 0);
    } else {
        g_usb_tx_busy_flag = false;
    }
}

/*!< endpoint call back */
struct usbd_endpoint cdc_out_ep = {
    .ep_addr = CDC_OUT_EP,
    .ep_cb = usbd_cdc_acm_bulk_out
};

struct usbd_endpoint cdc_in_ep = {
    .ep_addr = CDC_IN_EP,
    .ep_cb = usbd_cdc_acm_bulk_in
};

static struct usbd_interface intf0;
static struct usbd_interface intf1;
static struct usbd_interface intf2;

#if PICO_STDIO_CHERRYUSB_ENABLE_RESET_VIA_BAUD_RATE
// Support for default BOOTSEL reset by changing baud rate
void usbd_cdc_acm_set_line_coding(uint8_t busid, uint8_t intf, struct cdc_line_coding *line_coding) {
    (void)busid;
    (void)intf;
    if (line_coding->dwDTERate == PICO_STDIO_CHERRYUSB_RESET_MAGIC_BAUD_RATE) {
#ifdef PICO_STDIO_CHERRYUSB_RESET_BOOTSEL_ACTIVITY_LED
        int gpio = PICO_STDIO_CHERRYUSB_RESET_BOOTSEL_ACTIVITY_LED;
        bool active_low = PICO_STDIO_CHERRYUSB_RESET_BOOTSEL_ACTIVITY_LED_ACTIVE_LOW;
#else
        int gpio = -1;
        bool active_low = false;
#endif
        rom_reset_usb_boot_extra(gpio, PICO_STDIO_CHERRYUSB_RESET_BOOTSEL_INTERFACE_DISABLE_MASK, active_low);
    }
}
#endif

static void cdc_acm_init(uint8_t busid, uintptr_t reg_base) {
#ifdef CONFIG_USBDEV_ADVANCE_DESC
    usbd_desc_register(busid, &cdc_descriptor);
#else
    usbd_desc_register(busid, cdc_descriptor);
#endif
#ifndef CONFIG_USBDEV_ADVANCE_DESC
#if PICO_STDIO_CHERRYUSB_ENABLE_RESET_VIA_VENDOR_INTERFACE && PICO_STDIO_CHERRYUSB_RESET_INTERFACE_SUPPORT_MS_OS_20_DESCRIPTOR
    usbd_bos_desc_register(busid, &bos_desc);
    usbd_msosv2_desc_register(busid, &msosv2_desc);
#endif
#endif
    usbd_add_interface(busid, usbd_cdc_acm_init_intf(busid, &intf0));
    usbd_add_interface(busid, usbd_cdc_acm_init_intf(busid, &intf1));
#if PICO_STDIO_CHERRYUSB_ENABLE_RESET_VIA_VENDOR_INTERFACE && PICO_STDIO_CHERRYUSB_RESET_INTERFACE_SUPPORT_MS_OS_20_DESCRIPTOR
    intf2.vendor_handler = rp_vendor_request_handler;
    usbd_add_interface(busid, &intf2);
#endif
    usbd_add_endpoint(busid, &cdc_out_ep);
    usbd_add_endpoint(busid, &cdc_in_ep);
    usbd_initialize(busid, reg_base, usbd_event_handler);
}

#if PICO_STDIO_CHERRYUSB_SUPPORT_CHARS_AVAILABLE_CALLBACK
static void (*chars_available_callback)(void *);
static void *chars_available_param;
#endif

static mutex_t stdio_usb_mutex;

static void stdio_usb_out_chars(const char *buf, int length) {
    uint64_t last_avail_time;
    if (!mutex_try_enter_block_until(&stdio_usb_mutex, make_timeout_time_ms(PICO_STDIO_DEADLOCK_TIMEOUT_MS))) {
        return;
    }

    last_avail_time = time_us_64();
    if (usb_device_is_configured(0)) {
        g_usb_tx_busy_flag = true;
        usbd_ep_start_write(0, CDC_IN_EP, buf, length);
        while (g_usb_tx_busy_flag) {
            if ((time_us_64() - last_avail_time) > PICO_STDIO_CHERRYUSB_STDOUT_TIMEOUT_US) {
                break;
            }
        }
    } else {
    }
    mutex_exit(&stdio_usb_mutex);
}

static void stdio_usb_out_flush(void) {
}

static int stdio_usb_in_chars(char *buf, int length) {
    int rc = PICO_ERROR_NO_DATA;
    uint32_t len;

    if (g_usb_rx_count > 0) {
        len = MIN(g_usb_rx_count - g_usb_rx_offset, length);
        memcpy(buf, &g_usb_read_buffer[g_usb_rx_offset], len);
        g_usb_rx_offset += len;
        if (len > 0) {
            return len;
        } else {
            g_usb_rx_count = 0;
            /* setup first out ep read transfer */
            usbd_ep_start_read(0, CDC_OUT_EP, g_usb_read_buffer, CDC_MAX_MPS);
            return rc;
        }
    } else {
        return rc;
    }
}

#if PICO_STDIO_CHERRYUSB_SUPPORT_CHARS_AVAILABLE_CALLBACK
void stdio_usb_set_chars_available_callback(void (*fn)(void *), void *param) {
    chars_available_callback = fn;
    chars_available_param = param;
}
#endif

stdio_driver_t stdio_usb = {
    .out_chars = stdio_usb_out_chars,
    .out_flush = stdio_usb_out_flush,
    .in_chars = stdio_usb_in_chars,
#if PICO_STDIO_CHERRYUSB_SUPPORT_CHARS_AVAILABLE_CALLBACK
    .set_chars_available_callback = stdio_usb_set_chars_available_callback,
#endif
#if PICO_STDIO_ENABLE_CRLF_SUPPORT
    .crlf_enabled = PICO_STDIO_CHERRYUSB_DEFAULT_CRLF
#endif
};

bool stdio_usb_init(void) {
    if (get_core_num() != alarm_pool_core_num(alarm_pool_get_default())) {
        // included an assertion here rather than just returning false, as this is likely
        // a coding bug, rather than anything else.
        assert(false);
        return false;
    }
#if !PICO_NO_BI_STDIO_USB
    bi_decl_if_func_used(bi_program_feature("USB stdin / stdout"));
#endif

    cdc_acm_init(0, 0);

    if (!mutex_is_initialized(&stdio_usb_mutex))
        mutex_init(&stdio_usb_mutex);
    bool rc = true;
    if (rc) {
        stdio_set_driver_enabled(&stdio_usb, true);
#if PICO_STDIO_CHERRYUSB_CONNECT_WAIT_TIMEOUT_MS
#if PICO_STDIO_CHERRYUSB_CONNECT_WAIT_TIMEOUT_MS > 0
        absolute_time_t until = make_timeout_time_ms(PICO_STDIO_CHERRYUSB_CONNECT_WAIT_TIMEOUT_MS);
#else
        absolute_time_t until = at_the_end_of_time;
#endif
        do {
            if (usb_device_is_configured(0)) {
#if PICO_STDIO_CHERRYUSB_CONNECT_WAIT_TIMEOUT_MS != 0
                sleep_ms(PICO_STDIO_CHERRYUSB_CONNECT_WAIT_TIMEOUT_MS);
#endif
                break;
            }
            sleep_ms(10);
        } while (!time_reached(until));
#endif
    }
    return rc;
}

bool stdio_usb_deinit(void) {
    if (get_core_num() != alarm_pool_core_num(alarm_pool_get_default())) {
        // included an assertion here rather than just returning false, as this is likely
        // a coding bug, rather than anything else.
        assert(false);
        return false;
    }

    usbd_deinitialize(0);

    bool rc = true;

    stdio_set_driver_enabled(&stdio_usb, false);

#if PICO_STDIO_CHERRYUSB_DEINIT_DELAY_MS != 0
    sleep_ms(PICO_STDIO_CHERRYUSB_DEINIT_DELAY_MS);
#endif
    return rc;
}
#endif