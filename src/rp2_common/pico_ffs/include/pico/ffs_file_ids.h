#ifndef _PICO_FFS_FILE_IDS_H
#define _PICO_FFS_FILE_IDS_H

#ifdef __cplusplus
extern "C" {
#endif

// FFS file IDs used by pico_rpi_connect_ota
#define FFS_AUTH_TOKEN_FILE_ID 0x1
#define FFS_DEPLOYMENT_ID_FILE_ID 0x2
#define FFS_DEPLOYMENT_URI_FILE_ID 0x3
#define FFS_DEPLOYMENT_CHECKSUM_FILE_ID 0x4
#define FFS_DEPLOYMENT_STATUS_FILE_ID 0x5

// FFS file IDs used for WiFi credentials
#define FFS_WIFI_SSID_FILE_ID 0x10
#define FFS_WIFI_PASSWORD_FILE_ID 0x11

// Reserve all file IDs 0b0xxx for SDK use
#define FFS_RESERVED_FILE_ID_MAX 0x7f

#ifdef __cplusplus
}
#endif

#endif
