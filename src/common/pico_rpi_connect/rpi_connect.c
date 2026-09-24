/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/rpi_connect.h"
#include "pico/rpi_connect_util.h"
#include <cJSON.h>

#include "connect_crypto.h"
#include "request.h"
#include "rpi_connect_ca_cert.h"
#include "slist.h"

#if PICO_ON_DEVICE
#include "pico/version.h"
#include "pico/platform.h"
#endif

// Structure definition for the opaque event context type
struct RPI_CONNECT_EVENT_CONTEXT {
    char *token;                                   // Bearer token for authentication
    rpi_connect_event_callback_t callback;             // Callback function for event processing
    void *callback_arg;                            // User argument for callback
    memory_struct_t chunk;                     // Memory chunk for response data
    request_async_context_t request_async_context; // For streaming events - callbacxk invoked when data received
};

static void process_event_stream(void *arg);

static const char *g_rpi_connect_api_host = "api.connect.raspberrypi.com";
static const char * const g_rpi_connect_event_host = "ws.connect.raspberrypi.com";
static async_context_t *g_rpi_connect_async_context;
static uint32_t g_rpi_connect_http_timeout = RPI_CONNECT_HTTP_TIMEOUT_MIN;
static const char *g_empty_json = "{}";
static const int g_empty_json_len = 2;
static const char * const g_rpi_connect_default_ca_cert = RPI_CONNECT_CA_CERT;
static const char *g_rpi_connect_ca_cert = g_rpi_connect_default_ca_cert;
// Set via rpi_connect_set_serial_number() - used as the HMAC key base for
// X-Connect-Signature on requests that need signing (devices that set a
// signing secret at registration time).  NULL = no signature applied.
static const char *g_rpi_connect_serial_number;
static const char *g_rpi_connect_client_id = "96BE5607-EDF9-4403-8D49-31D17A99FAF0";

const char *rpi_connect_api_host(void) {
    return g_rpi_connect_api_host;
}

const char *rpi_connect_event_host(void) {
    return g_rpi_connect_event_host;
}

#if !PICO_ON_DEVICE
// Test hook: point the API at a local mock server. The hosts are fixed on
// device.
void rpi_connect_set_api_host(const char *host) {
    if (host) {
        g_rpi_connect_api_host = host;
    }
}
#endif

void rpi_connect_set_async_context(async_context_t *async_context) {
    g_rpi_connect_async_context = async_context;
}

void rpi_connect_set_serial_number(const char *serial_number) {
    g_rpi_connect_serial_number = serial_number;
}

const char *rpi_connect_serial_number(void) {
    return g_rpi_connect_serial_number;
}

void rpi_connect_set_client_id(const char *client_id) {
    g_rpi_connect_client_id = client_id;
}

const char *rpi_connect_client_id(void) {
    return g_rpi_connect_client_id;
}

async_context_t *rpi_connect_async_context(void) {
    return g_rpi_connect_async_context;
}

void rpi_connect_set_http_timeout(int timeout_ms) {
    if (timeout_ms > 0)
        g_rpi_connect_http_timeout = timeout_ms;
}

uint32_t rpi_connect_http_timeout(void) {
    return g_rpi_connect_http_timeout;
}

const char *rpi_connect_default_ca_cert(void) {
    return g_rpi_connect_default_ca_cert;
}

const char *rpi_connect_ca_cert(void) {
    return g_rpi_connect_ca_cert;
}

void rpi_connect_set_ca_cert(const char *ca_cert) {
    g_rpi_connect_ca_cert = ca_cert;
}

//
// Function to calculate the signature for a request
static char *rpi_calculate_signature(http_method_t method, const char *url, struct curl_slist *headers, const void *data, size_t data_size, const char *serial_number) {
    char *buf = calloc(1,1024);

    switch(method) {
        case HTTP_GET:
            strcpy(buf, "GET\n");
            break;
        case HTTP_POST:
            strcpy(buf, "POST\n");
            break;
        case HTTP_DELETE:
            strcpy(buf, "DELETE\n");
            break;
    }

    strcat(buf, url);
    strcat(buf, "\n");

    for(struct curl_slist *header = headers; header; header = header->next) {
        if(strncmp(header->data, "Authorization: Bearer ", 22) == 0) {
            strcat(buf, header->data);
            strcat(buf, "\n");
        } else if (strncmp(header->data, "Content-Type: ", 14) == 0) {
            strcat(buf, header->data);
            strcat(buf, "\n");
        } else if (strncmp(header->data, "Accept: ", 8) == 0) {
            strcat(buf, header->data);
            strcat(buf, "\n");
        } else if (strncmp(header->data, "X-Connect-Ttl: ", 15) == 0) {
            strcat(buf, header->data);
            strcat(buf, "\n");
        } else if (strncmp(header->data, "X-Connect-If-Txid: ", 19) == 0) {
            strcat(buf, header->data);
            strcat(buf, "\n");
        }
    }

    if(method == HTTP_POST && data != NULL && data_size > 0) {
        unsigned char hash[RPI_CONNECT_SHA256_SIZE];
        size_t hash_len;

        if (rpi_connect_crypto_sha256((const char *)data, data_size, hash, &hash_len) != 0) {
            RPI_CONNECT_ERROR("Error calculating SHA256 data hash\n");
            free(buf);
            return NULL;
        }

        // Convert hash to hex string
        char hash_hex[RPI_CONNECT_SHA256_SIZE*2 + 1];
        for(unsigned int i = 0; i < RPI_CONNECT_SHA256_SIZE; i++) {
            sprintf(&hash_hex[i*2], "%02x", hash[i]);
        }
        hash_hex[RPI_CONNECT_SHA256_SIZE*2] = '\0';

        // Append hash to buffer
        strcat(buf, hash_hex);
    }

    // Take a SHA256 HMAC of the buffer with the SHA256 of the serial number as the hamc key
    // Note: In a real implementation, you would use a proper key here
    unsigned char hmac_result[RPI_CONNECT_SHA256_SIZE];
    size_t hmac_len;

    // mac key should be the sha256 of the serial number
    unsigned char sha256_key[RPI_CONNECT_SHA256_SIZE];
    size_t sha256_key_len;
    char sha256_key_str[RPI_CONNECT_SHA256_SIZE*2 + 1];

    if (rpi_connect_crypto_sha256(serial_number, strlen(serial_number), sha256_key, &sha256_key_len) != 0) {
        RPI_CONNECT_ERROR("Error calculating SHA256 hash\n");
        free(buf);
        return NULL;
    }

    // Note: The MAC key is the SHA256 of the serial number but as an ascii string
    for (unsigned int i = 0; i < sha256_key_len; i++) {
        sprintf(&sha256_key_str[i*2], "%02x", sha256_key[i]);
    }
    sha256_key_str[sha256_key_len*2] = '\0';

    if (rpi_connect_crypto_hmac_sha256(buf, strlen(buf), sha256_key_str, sha256_key_len*2, hmac_result, &hmac_len) != 0) {
        RPI_CONNECT_ERROR("Error calculating HMAC\n");
        free(buf);
        return NULL;
    }

    // Convert HMAC to hex string
    char *signature = malloc(hmac_len*2 + 1);
    for(size_t i = 0; i < hmac_len; i++) {
        sprintf(&signature[i*2], "%02x", hmac_result[i]);
    }
    signature[hmac_len*2] = '\0';

    free(buf);
    return signature;
}

//
// Callback function to add the signature to the headers
void rpi_connect_add_signature(http_method_t method, const char *url, struct curl_slist *headers, const void *data, size_t data_size, const char *serial_number) {
    // Set the X-Connect-Signature header if a serial number is provided
    if(serial_number != NULL) {
        char *signature = rpi_calculate_signature(method, url, headers, data, data_size, serial_number);
        if (signature) {
            char signature_header[256];
            snprintf(signature_header, sizeof(signature_header), "X-Connect-Signature: %s", signature);
            headers = curl_slist_append(headers, signature_header);
            free(signature);
        }
    }
}

//
// Function to fetch a signin URL from the Connect API
t_rpi_connect_signin * rpi_connect_signin(const char *client_id, const char *serial_number) {
    long http_code = 0;
    memory_struct_t chunk;

    rpi_connect_memory_struct_init(&chunk);

    // Create JSON payload
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "client_id", client_id);
    cJSON_AddStringToObject(json, "login_hint", serial_number);
    char *json_str = cJSON_Print(json);
    const char *endpoint = "/client/device";

    // Perform request
    http_code = rpi_connect_request_perform_http(
        HTTP_POST,
        rpi_connect_api_host(),
        endpoint,
        NULL,
        NULL,
        "application/json",
        json_str,
        strlen(json_str),
        &chunk,
        NULL,
        rpi_connect_async_context(),
        NULL,
        false
    );

    // Cleanup JSON
    cJSON_Delete(json);
    free(json_str);

    if (http_code != 200) {
        rpi_connect_memory_struct_free(&chunk);
        return NULL;
    }

    // Parse the response
    // Example response:
    // {"device_code":"94eb2b5a-56e6-4831-8ddf-c816d715eb95","user_code":"TKD8-F390","verification_uri_complete":"https://connect.raspberrypi.com/verify/TKD8-F390","expires_in":899,"interval":5}
    cJSON *response = cJSON_Parse(chunk.memory);
    cJSON *device_code = cJSON_GetObjectItem(response, "device_code");
    cJSON *user_code = cJSON_GetObjectItem(response, "user_code");
    cJSON *verification_uri_complete = cJSON_GetObjectItem(response, "verification_uri_complete");

    t_rpi_connect_signin *signin = malloc(sizeof(t_rpi_connect_signin));

    // Extract strings from cJSON objects
    const char *device_code_str = cJSON_GetStringValue(device_code);
    const char *user_code_str = cJSON_GetStringValue(user_code);
    const char *verification_uri_complete_str = cJSON_GetStringValue(verification_uri_complete);

    signin->device_code = strdup(device_code_str);
    signin->user_code = strdup(user_code_str);
    signin->verification_uri_complete = strdup(verification_uri_complete_str);

    // Free the JSON object
    cJSON_Delete(response);

    // Free the chunk memory
    rpi_connect_memory_struct_free(&chunk);

    return signin;
}

void rpi_connect_signin_cleanup(t_rpi_connect_signin *signin) {
    if (signin) {
        free(signin->device_code);
        free(signin->user_code);
        free(signin->verification_uri_complete);
        free(signin);
    }
}

//
// curl -s --header "Content-Type: application/json"
// --request POST --data "{\"client_id\":\"057C9305-A173-4596-BDAC-9701A92F7F62\",\"device_code\":\"Device_Code\"}" https://api.connect.raspberrypi.com/client/token
char * rpi_connect_retrieve_token_with_device_code(const char *client_id, const char *device_code, const char *serial_number) {
    long http_code = 0;
    memory_struct_t chunk;

    rpi_connect_memory_struct_init(&chunk);

    // Create JSON payload
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "client_id", client_id);
    cJSON_AddStringToObject(json, "device_code", device_code);
    char *json_str = cJSON_Print(json);
    const char *endpoint = "/client/token";

    // Perform request
    http_code = rpi_connect_request_perform_http(
        HTTP_POST,
        rpi_connect_api_host(),
        endpoint,
        NULL,
        NULL,
        "application/json",
        json_str,
        strlen(json_str),
        &chunk,
        serial_number,
        rpi_connect_async_context(),
        NULL,
        false
    );

    // Cleanup JSON
    cJSON_Delete(json);
    free(json_str);

    if (http_code != 200) {
        // Parse error response
        cJSON *response = cJSON_Parse(chunk.memory);
        cJSON *error = cJSON_GetObjectItem(response, "error");
        cJSON *error_description = cJSON_GetObjectItem(response, "error_description");

        if (error) {
            const char *error_str = cJSON_GetStringValue(error);

            // Common error: authorization_pending - user hasn't authorized yet
            if (strcmp(error_str, "authorization_pending") == 0) {
                // Do nothing, we will poll again later
            }
            // Common error: slow_down - polling too frequently
            else if (strcmp(error_str, "slow_down") == 0) {
                RPI_CONNECT_DEBUG("Slow down: Polling too frequently\n");
                // Could increase the polling interval here
            }
            // Common error: expired_token - device code has expired
            else if (strcmp(error_str, "expired_token") == 0) {
                RPI_CONNECT_DEBUG("Error: Device code has expired\n");
            }
            // Other errors
            else {
                RPI_CONNECT_DEBUG("Error: %s", error_str);

                if (error_description) {
                    const char *desc = cJSON_GetStringValue(error_description);
                    RPI_CONNECT_DEBUG(" - %s", desc);
                }

                RPI_CONNECT_DEBUG("\n");
            }
        } else {
            if (http_code != 401 && http_code != 429) {
                RPI_CONNECT_ERROR("Request failed with HTTP code %ld\n", http_code);
            } else {
                RPI_CONNECT_DEBUG("Request failed with HTTP code %ld\n", http_code);
            }
        }

        cJSON_Delete(response);
        rpi_connect_memory_struct_free(&chunk);
        return NULL;
    }

    // Parse the response
    cJSON *response = cJSON_Parse(chunk.memory);

    // Example successful response:
    // {"device_id":"5df23152-10dc-491d-9d9a-a2c264402f10","access_token":"rpdev_B4UXqsWLQQ26gVvTyEmGXkuV"}
    cJSON *access_token = cJSON_GetObjectItem(response, "access_token");
    cJSON *device_id = cJSON_GetObjectItem(response, "device_id");

    char *token = NULL;
    if (access_token) {
        const char *token_str = cJSON_GetStringValue(access_token);
        token = strdup(token_str);
        char *p = token;
        for(; *p; p++) {
            if (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t') {
                *p = '\0';
            }
        }

        if (device_id) {
            const char *device_id_str = cJSON_GetStringValue(device_id);
            RPI_CONNECT_DEBUG("Device ID: %s\n", device_id_str);
        }
    } else {
        RPI_CONNECT_ERROR("No access token in response\n");
    }

    // Free the JSON object
    cJSON_Delete(response);

    // Free the chunk memory
    rpi_connect_memory_struct_free(&chunk);

    return token;
}

#if PICO_ON_DEVICE
// Add the "pico" device-information object (see POST /me in the Connect API
// docs) that accompanies a capabilities update. Values come from CMake/SDK
// build vars, the SDK platform helpers and the bootrom. On the host (POSIX)
// build this is omitted.
static void add_pico_device_info(cJSON *json) {
    cJSON *pico = cJSON_AddObjectToObject(json, "pico");

#if PICO_RP2350
    const char *platform = "rp2350";
    unsigned chip_version = rp2350_chip_version();
    unsigned rom_version = rp2350_rom_version();
#else
    const char *platform = "rp2040";
    unsigned chip_version = rp2040_chip_version();
    unsigned rom_version = rp2040_rom_version();
#endif

    cJSON_AddStringToObject(pico, "platform", platform);
#ifdef PICO_BOARD
    cJSON_AddStringToObject(pico, "board_name", PICO_BOARD);
#endif
#ifdef PICO_TARGET_NAME
    cJSON_AddStringToObject(pico, "program_name", PICO_TARGET_NAME);
#endif
    cJSON_AddStringToObject(pico, "sdk_version", PICO_SDK_VERSION_STRING);
    cJSON_AddNumberToObject(pico, "rom_version", rom_version);
    cJSON_AddNumberToObject(pico, "chip_version", chip_version);
}
#endif

// Feature names for POST /me, indexed by their byte position in the packed
// capability value (release order - see "Capabilities encoding" in the
// Connect API docs).
static const char *const g_rpi_connect_capability_names[] = { "vnc", "shell", "ota" };

// Declare the device's capabilities via POST /me. `capability` keeps one byte
// per feature, lowest index first (bit 0 = available, bit 1 = enabled). The
// server merges rather than replaces, so every known feature is sent
// explicitly - unsupported ones as "unavailable" - keeping the client
// authoritative for the whole set.
int rpi_connect_send_capability(const char *token, unsigned int capability) {
    // Signing (X-Connect-Signature) is enabled when a serial number has been
    // set via rpi_connect_set_serial_number(); NULL sends unsigned.
    const char *serial_number = rpi_connect_serial_number();
    long http_code = 0;
    memory_struct_t chunk;
    size_t feature_count = sizeof(g_rpi_connect_capability_names) / sizeof(g_rpi_connect_capability_names[0]);

    cJSON *json = cJSON_CreateObject();
    cJSON *capabilities = cJSON_AddObjectToObject(json, "capabilities");
    for (size_t i = 0; i < feature_count; i++) {
        uint8_t feature = (capability >> (8 * i)) & 0xff;
        // "enabled" implies "available" server-side
        const char *state = (feature & 0x02) ? "enabled" :
                            (feature & 0x01) ? "available" : "unavailable";
        cJSON_AddStringToObject(capabilities, g_rpi_connect_capability_names[i], state);
    }
    if (capability >> (8 * feature_count))
        RPI_CONNECT_ERROR("Capability bytes beyond the known features not sent (0x%x)\n", capability);

#if PICO_ON_DEVICE
    add_pico_device_info(json);
#endif

    char *json_str = cJSON_PrintUnformatted(json);
    const char *endpoint = "/me";

    rpi_connect_memory_struct_init(&chunk);

    // Perform request
    http_code = rpi_connect_request_perform_http(
        HTTP_POST,
        rpi_connect_api_host(),
        endpoint,
        token,
        NULL,
        "application/json",
        json_str,
        strlen(json_str),
        &chunk,
        serial_number,
        rpi_connect_async_context(),
        NULL,
        false
    );

    cJSON_Delete(json);
    free(json_str);
    rpi_connect_memory_struct_free(&chunk);

    if (http_code != 200 && http_code != 204) {
        RPI_CONNECT_ERROR("Request failed with HTTP code %ld\n", http_code);
        return -1;
    }

    return 0;
}

char * rpi_connect_handle_auth_key(const char *auth_key, const char *serial_number, const char *hostname, const char *client_id) {
    char *token = NULL;
    memory_struct_t chunk;

    rpi_connect_memory_struct_init(&chunk);

    // Perform a request to https://api.connect-staging.raspberrypi.com/client/auth-key-exchange
    // "Content-Type: application/json"
    // data: '{"auth_key":auth_key,"serial_number":serial_number,"hostname":hostname,"client_id":client_id}']

    // Create JSON payload
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "auth_key", auth_key);
    cJSON_AddStringToObject(json, "serial_number", serial_number);
    cJSON_AddStringToObject(json, "hostname", hostname);
    cJSON_AddStringToObject(json, "client_id", client_id);
    char *json_str = cJSON_Print(json);
    const char *endpoint = "/client/auth-key-exchange";

    // Perform request
    rpi_connect_request_perform_http(
        HTTP_POST,
        rpi_connect_api_host(),
        endpoint,
        NULL,
        NULL,
        "application/json",
        json_str,
        strlen(json_str),
        &chunk,
        NULL,
        rpi_connect_async_context(),
        NULL,
        false
    );

    // Cleanup JSON
    cJSON_Delete(json);
    free(json_str);

    // If the chunk.memory is empty, return NULL
    if (chunk.size == 0) {
        rpi_connect_memory_struct_free(&chunk);
        return NULL;
    }

    // Parse the response, it'll either be the token or an error
    cJSON *response = cJSON_Parse(chunk.memory);
    if (!response) {
        RPI_CONNECT_ERROR("Failed to parse JSON response\n");
        rpi_connect_memory_struct_free(&chunk);
        return NULL;
    }

    cJSON *error_obj = cJSON_GetObjectItem(response, "error");
    if (error_obj) {
        const char *error_str = cJSON_GetStringValue(error_obj);
        RPI_CONNECT_ERROR("Error: %s\n", error_str);
        cJSON_Delete(response);
        rpi_connect_memory_struct_free(&chunk);
        return NULL;
    }

    cJSON *token_obj = cJSON_GetObjectItem(response, "access_token");
    if (token_obj) {
        token = strdup(cJSON_GetStringValue(token_obj));
    }

    // Free the JSON object and chunk memory
    cJSON_Delete(response);
    rpi_connect_memory_struct_free(&chunk);

    return token;
}

// GET /deployments/pending
// Authentication: Bearer rpdev_123456
// Responses
// A 204 No Content if the device does not have a pending deployment.
//
// A 200 OK if the device has a pending deployment.
// HTTP/1.1 200 OK
// Content-Type: application/json
//
// {"deployments":[{"id":"00000000-0000-0000-0000-000000000000"}]}

int rpi_connect_get_pending_deployment(const char *token, char **deployment_id) {
    long http_code = 0;
    int rc = 0;
    memory_struct_t chunk;

    if (!deployment_id)
        return -1;
    *deployment_id = NULL;

    rpi_connect_memory_struct_init(&chunk);

    struct curl_slist *additional_headers = NULL;
    additional_headers = curl_slist_append(additional_headers, "Accept: application/json");

    http_code = rpi_connect_request_perform_http(
        HTTP_GET,
        rpi_connect_api_host(),
        "/deployments/pending",
        token,
        additional_headers,
        NULL,
        NULL,
        0,
        &chunk,
        NULL,
        rpi_connect_async_context(),
        NULL,
        false
    );

    if (http_code == 204) {
        // No pending deployment
    } else if (http_code != 200) {
        RPI_CONNECT_ERROR("pending deployments failed with HTTP code %ld\n", http_code);
        rc = -http_code;
    } else if (chunk.memory) {
        cJSON *response = cJSON_Parse(chunk.memory);
        if (response) {
            cJSON *deployments = cJSON_GetObjectItem(response, "deployments");
            if (cJSON_IsArray(deployments)) {
                // The device processes one deployment at a time: take the
                // first entry; any others are picked up by later checks.
                cJSON *first = cJSON_GetArrayItem(deployments, 0);
                cJSON *id_obj = cJSON_GetObjectItem(first, "id");
                if (cJSON_IsString(id_obj))
                    *deployment_id = strdup(id_obj->valuestring);
            }
            cJSON_Delete(response);
        }
    }

    curl_slist_free_all(additional_headers);
    rpi_connect_memory_struct_free(&chunk);
    return rc;
}

//
// POST /deployments/00000000-0000-0000-0000-000000000000/start
// Authentication: Bearer rpdev_123456
// Content-Type: application/json
// Responses
// A 404 Not Found if no deployment exists.

// A 422 Unprocessable Content if the deployment cannot be transitioned
// HTTP/1.1 422 Unprocessable Content
// Content-Type: application/json

// {"message": "..."}

// A 200 OK if the deployment successfully transitioned.
// HTTP/1.1 200 OK
// Content-Type: application/json
// {"artefact":{"uri":"https://connect.raspberrypi.com/example.zip","checksum":"abc123"},"timestamp":1234567890,"signature":"..."}

int rpi_connect_start_deployment(const char *token, const char *deployment_id, char **uri, char **checksum) {
    long http_code = 0;
    int rc = 0;
    memory_struct_t chunk;

    if (uri)
        *uri = NULL;
    if (checksum)
        *checksum = NULL;

    rpi_connect_memory_struct_init(&chunk);

    // Set up request URL
    char endpoint[256];
    snprintf(endpoint, sizeof(endpoint), "/deployments/%s/start", deployment_id);

    struct curl_slist *additional_headers = NULL;
    additional_headers = curl_slist_append(additional_headers, "Accept: application/json");

    // Perform request - with empty JSON body {} insteasd of zero length because
    // the HTTP doesn't handle zero length post data correctly.
    http_code = rpi_connect_request_perform_http(
        HTTP_POST,
        rpi_connect_api_host(),
        endpoint,
        token,
        additional_headers,
        "application/json",
        g_empty_json,
        g_empty_json_len,
        &chunk,
        NULL,
        rpi_connect_async_context(),
        NULL,
        false
    );

    if (http_code != 200) {
        RPI_CONNECT_ERROR("deployment start failed with HTTP code %ld\n", http_code);
        rc = -http_code;
    } else if (chunk.memory) {
        cJSON *response = cJSON_Parse(chunk.memory);
        if (response) {
            cJSON *artefact = cJSON_GetObjectItem(response, "artefact");
            if (artefact) {
                cJSON *uri_obj = cJSON_GetObjectItem(artefact, "uri");
                cJSON *checksum_obj = cJSON_GetObjectItem(artefact, "checksum");
                if (uri && cJSON_IsString(uri_obj))
                    *uri = strdup(uri_obj->valuestring);
                if (checksum && cJSON_IsString(checksum_obj))
                    *checksum = strdup(checksum_obj->valuestring);
            }
            cJSON_Delete(response);
        }
    }

    curl_slist_free_all(additional_headers);
    rpi_connect_memory_struct_free(&chunk);
    return rc;
}

// POST /deployments/:id/fail
// POST /deployments/00000000-0000-0000-0000-000000000000/fail
// Authentication: Bearer rpdev_123456

// Attempt to transition the deployment’s state to "failed".
// Parameters
// id: unique identifier of the deployment to transition
// reason: optional reason for the failure
// Responses
// A 404 Not Found if no deployment exists.
//
// A 422 Unprocessable Content if the deployment cannot be transitioned
// HTTP/1.1 422 Unprocessable Content
// Content-Type: application/json
//
// {"message": "..."}
//
// A 204 No Content if the deployment successfully transitioned.

int rpi_connect_fail_deployment(const char *token, const char *deployment_id, const char *reason) {
    long http_code = 0;
    memory_struct_t chunk;

    rpi_connect_memory_struct_init(&chunk);

    // Create JSON payload
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "reason", reason);
    char *json_str = cJSON_Print(json);

    // Set up request URL
    char endpoint[256];
    snprintf(endpoint, sizeof(endpoint), "/deployments/%s/fail", deployment_id);

    // Perform request
    http_code = rpi_connect_request_perform_http(
        HTTP_POST,
        rpi_connect_api_host(),
        endpoint,
        token,
        NULL,
        "application/json",
        json_str,
        strlen(json_str),
        &chunk,
        NULL,
        rpi_connect_async_context(),
        NULL,
        false
    );

    // Cleanup JSON
    cJSON_Delete(json);
    free(json_str);

    if (http_code != 200 && http_code != 204) {
        RPI_CONNECT_ERROR("deployment fail failed with HTTP code %ld\n", http_code);
        rpi_connect_memory_struct_free(&chunk);
        return -http_code;
    }

    rpi_connect_memory_struct_free(&chunk);
    return 0;
}

// POST /deployments/:id/complete
// POST /deployments/00000000-0000-0000-0000-000000000000/complete
// Authentication: Bearer rpdev_123456
// Attempt to transition the deployment’s state to "succeeded".
// Parameters
// id: unique identifier of the deployment to transition
// Responses
// A 404 Not Found if no deployment exists.
// A 422 Unprocessable Content if the deployment cannot be transitioned
// HTTP/1.1 422 Unprocessable Content
// Content-Type: application/json
//
// {"message": "..."}
//
// A 204 No Content if the deployment successfully transitioned.

int rpi_connect_complete_deployment(const char *token, const char *deployment_id) {
    long http_code = 0;
    memory_struct_t chunk;

    rpi_connect_memory_struct_init(&chunk);

    // Set up request URL
    char endpoint[256];
    snprintf(endpoint, sizeof(endpoint), "/deployments/%s/complete", deployment_id);

    // Perform request
    http_code = rpi_connect_request_perform_http(
        HTTP_POST,
        rpi_connect_api_host(),
        endpoint,
        token,
        NULL,
        "application/json",
        g_empty_json,
        g_empty_json_len,
        &chunk,
        NULL,
        rpi_connect_async_context(),
        NULL,
        false
    );

    if (http_code != 204) {
        RPI_CONNECT_ERROR("deployment complete failed with HTTP code %ld\n", http_code);
        rpi_connect_memory_struct_free(&chunk);
        return -http_code;
    }

    rpi_connect_memory_struct_free(&chunk);
    return 0;
}

static const char g_b64_table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static char *base64_encode(const unsigned char *data, size_t len) {
    size_t out_len = 4 * ((len + 2) / 3);
    char *out = malloc(out_len + 1);
    if (!out) return NULL;

    size_t i, j;
    for (i = 0, j = 0; i < len; i += 3, j += 4) {
        uint32_t val = (uint32_t)data[i] << 16;
        if (i + 1 < len) val |= (uint32_t)data[i + 1] << 8;
        if (i + 2 < len) val |= (uint32_t)data[i + 2];

        out[j]     = g_b64_table[(val >> 18) & 0x3F];
        out[j + 1] = g_b64_table[(val >> 12) & 0x3F];
        out[j + 2] = (i + 1 < len) ? g_b64_table[(val >> 6) & 0x3F] : '=';
        out[j + 3] = (i + 2 < len) ? g_b64_table[val & 0x3F] : '=';
    }
    out[out_len] = '\0';
    return out;
}

static char *rpi_calculate_identity_signature(
    http_method_t method, const char *url, struct curl_slist *headers,
    const void *data, size_t data_size,
    const unsigned char *private_key) {
    char *buf = calloc(1, 4096);
    if (!buf) return NULL;

    switch (method) {
        case HTTP_GET:    strcpy(buf, "GET\n"); break;
        case HTTP_POST:   strcpy(buf, "POST\n"); break;
        case HTTP_DELETE: strcpy(buf, "DELETE\n"); break;
    }

    strcat(buf, url);
    strcat(buf, "\n");

    for (struct curl_slist *header = headers; header; header = header->next) {
        if (strncmp(header->data, "Authorization: Bearer ", 22) == 0 ||
            strncmp(header->data, "Content-Type: ", 14) == 0 ||
            strncmp(header->data, "Accept: ", 8) == 0 ||
            strncmp(header->data, "X-Connect-Ttl: ", 15) == 0 ||
            strncmp(header->data, "X-Connect-Timestamp: ", 21) == 0) {
            strcat(buf, header->data);
            strcat(buf, "\n");
        }
    }

    if (method == HTTP_POST && data != NULL && data_size > 0) {
        unsigned char hash[RPI_CONNECT_SHA256_SIZE];
        size_t hash_len;
        if (rpi_connect_crypto_sha256((const char *)data, data_size, hash, &hash_len) != 0) {
            free(buf);
            return NULL;
        }
        char hash_hex[RPI_CONNECT_SHA256_SIZE * 2 + 1];
        for (unsigned int i = 0; i < RPI_CONNECT_SHA256_SIZE; i++)
            sprintf(&hash_hex[i * 2], "%02x", hash[i]);
        hash_hex[RPI_CONNECT_SHA256_SIZE * 2] = '\0';
        strcat(buf, hash_hex);
    }

    unsigned char payload_hash[RPI_CONNECT_SHA256_SIZE];
    size_t payload_hash_len;
    if (rpi_connect_crypto_sha256(buf, strlen(buf), payload_hash, &payload_hash_len) != 0) {
        free(buf);
        return NULL;
    }
    free(buf);

    unsigned char sig[RPI_CONNECT_CRYPTO_ECDSA_P256_SIG_MAX_SIZE];
    size_t sig_len = sizeof(sig);
    if (rpi_connect_crypto_ecdsa_p256_sign(payload_hash, private_key, sig, &sig_len) != 0)
        return NULL;

    return base64_encode(sig, sig_len);
}

char *rpi_connect_create_device_identity(
    const char *org_token,
    const unsigned char *private_key,
    const char *public_key_pem,
    const char *description,
    const char *device_name) {
    char *id = NULL;
    memory_struct_t chunk;
    rpi_connect_memory_struct_init(&chunk);

    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "public_key", public_key_pem);
    cJSON_AddStringToObject(json, "description", description);
    if (device_name)
        cJSON_AddStringToObject(json, "device_name", device_name);
    char *json_str = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);
    if (!json_str) {
        rpi_connect_memory_struct_free(&chunk);
        return NULL;
    }

    const char *endpoint = "/organisation/device-identities";

    char url[512];
    snprintf(url, sizeof(url), "https://%s%s", rpi_connect_api_host(), endpoint);

    struct curl_slist *sig_headers = NULL;
    char auth_hdr[256];
    snprintf(auth_hdr, sizeof(auth_hdr), "Authorization: Bearer %s", org_token);
    sig_headers = curl_slist_append(sig_headers, auth_hdr);
    sig_headers = curl_slist_append(sig_headers, "Content-Type: application/json");
    sig_headers = curl_slist_append(sig_headers, "Accept: */*");

    char *signature = rpi_calculate_identity_signature(
        HTTP_POST, url, sig_headers, json_str, strlen(json_str), private_key);
    curl_slist_free_all(sig_headers);

    if (!signature) {
        RPI_CONNECT_ERROR("Failed to calculate identity signature\n");
        free(json_str);
        rpi_connect_memory_struct_free(&chunk);
        return NULL;
    }

    struct curl_slist *additional_headers = NULL;
    char sig_header[512];
    snprintf(sig_header, sizeof(sig_header),
             "X-Connect-Identity-Signature: %s", signature);
    additional_headers = curl_slist_append(additional_headers, sig_header);
    free(signature);

    long http_code = rpi_connect_request_perform_http(
        HTTP_POST,
        rpi_connect_api_host(),
        endpoint,
        org_token,
        additional_headers,
        "application/json",
        json_str,
        strlen(json_str),
        &chunk,
        NULL,
        rpi_connect_async_context(),
        NULL,
        false
    );

    curl_slist_free_all(additional_headers);
    free(json_str);

    if (http_code != 201) {
        RPI_CONNECT_ERROR("create device identity failed with HTTP code %ld\n",
                          http_code);
        if (chunk.memory && chunk.size > 0) {
            cJSON *response = cJSON_Parse(chunk.memory);
            if (response) {
                cJSON *msg = cJSON_GetObjectItem(response, "message");
                if (cJSON_IsString(msg))
                    RPI_CONNECT_ERROR("Error: %s\n", msg->valuestring);
                cJSON_Delete(response);
            }
        }
        rpi_connect_memory_struct_free(&chunk);
        return NULL;
    }

    if (chunk.memory) {
        cJSON *response = cJSON_Parse(chunk.memory);
        if (response) {
            cJSON *id_obj = cJSON_GetObjectItem(response, "id");
            cJSON *desc_obj = cJSON_GetObjectItem(response, "description");
            cJSON *name_obj = cJSON_GetObjectItem(response, "device_name");
            cJSON *created_obj = cJSON_GetObjectItem(response, "created_at");

            if (cJSON_IsString(id_obj))
                id = strdup(id_obj->valuestring);

            RPI_CONNECT_INFO("Device identity created:\n");
            if (cJSON_IsString(id_obj))
                RPI_CONNECT_INFO("  id: %s\n", id_obj->valuestring);
            if (cJSON_IsString(desc_obj))
                RPI_CONNECT_INFO("  description: %s\n", desc_obj->valuestring);
            if (cJSON_IsString(name_obj))
                RPI_CONNECT_INFO("  device_name: %s\n", name_obj->valuestring);
            if (cJSON_IsString(created_obj))
                RPI_CONNECT_INFO("  created_at: %s\n", created_obj->valuestring);

            cJSON_Delete(response);
        }
    }

    rpi_connect_memory_struct_free(&chunk);
    return id;
}

char *rpi_connect_device_identity_exchange(
    const char *client_id,
    const unsigned char *private_key,
    const char *public_key_pem,
    const char *hostname,
    const char *serial_number,
    char **out_device_id) {
    char *token = NULL;
    memory_struct_t chunk;
    rpi_connect_memory_struct_init(&chunk);

    if (out_device_id)
        *out_device_id = NULL;

    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "client_id", client_id);
    cJSON_AddStringToObject(json, "public_key", public_key_pem);
    cJSON_AddStringToObject(json, "hostname", hostname);
    cJSON_AddStringToObject(json, "serial_number", serial_number);
    char *json_str = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);
    if (!json_str) {
        rpi_connect_memory_struct_free(&chunk);
        return NULL;
    }

    const char *endpoint = "/client/device-identity-exchange";

    char url[512];
    snprintf(url, sizeof(url), "https://%s%s", rpi_connect_api_host(), endpoint);

    // Optional replay protection: date the request when wall-clock time
    // is known (rpi_connect_update_time()). The server rejects a
    // timestamp more than a few minutes from its own clock with a 403.
    char ts_header[48];
    int64_t now = rpi_connect_time();
    if (now > 0)
        snprintf(ts_header, sizeof(ts_header),
                 "X-Connect-Timestamp: %lld", (long long)now);

    // Headers used to compute the X-Connect-Identity-Signature. The set must
    // mirror what the underlying HTTP client actually transmits so that the
    // server can recreate and verify the same signed payload.
    struct curl_slist *sig_headers = NULL;
    sig_headers = curl_slist_append(sig_headers, "Content-Type: application/json");
    sig_headers = curl_slist_append(sig_headers, "Accept: */*");
    if (now > 0)
        sig_headers = curl_slist_append(sig_headers, ts_header);

    char *signature = rpi_calculate_identity_signature(
        HTTP_POST, url, sig_headers, json_str, strlen(json_str), private_key);
    curl_slist_free_all(sig_headers);

    if (!signature) {
        RPI_CONNECT_ERROR("Failed to calculate identity signature\n");
        free(json_str);
        rpi_connect_memory_struct_free(&chunk);
        return NULL;
    }

    struct curl_slist *additional_headers = NULL;
    char sig_header[512];
    snprintf(sig_header, sizeof(sig_header),
             "X-Connect-Identity-Signature: %s", signature);
    additional_headers = curl_slist_append(additional_headers, sig_header);
    additional_headers = curl_slist_append(additional_headers, "Accept: */*");
    if (now > 0)
        additional_headers = curl_slist_append(additional_headers, ts_header);
    free(signature);

    long http_code = rpi_connect_request_perform_http(
        HTTP_POST,
        rpi_connect_api_host(),
        endpoint,
        NULL,
        additional_headers,
        "application/json",
        json_str,
        strlen(json_str),
        &chunk,
        NULL,
        rpi_connect_async_context(),
        NULL,
        false
    );

    curl_slist_free_all(additional_headers);
    free(json_str);

    if (http_code != 200) {
        RPI_CONNECT_ERROR("device identity exchange failed with HTTP code %ld\n",
                          http_code);
        if (chunk.memory && chunk.size > 0) {
            cJSON *response = cJSON_Parse(chunk.memory);
            if (response) {
                cJSON *err = cJSON_GetObjectItem(response, "error");
                cJSON *msg = cJSON_GetObjectItem(response, "message");
                if (cJSON_IsString(err))
                    RPI_CONNECT_ERROR("Error: %s\n", err->valuestring);
                if (cJSON_IsString(msg))
                    RPI_CONNECT_ERROR("Message: %s\n", msg->valuestring);
                cJSON_Delete(response);
            }
        }
        rpi_connect_memory_struct_free(&chunk);
        return NULL;
    }

    if (chunk.memory) {
        cJSON *response = cJSON_Parse(chunk.memory);
        if (response) {
            cJSON *id_obj = cJSON_GetObjectItem(response, "device_id");
            cJSON *tok_obj = cJSON_GetObjectItem(response, "access_token");
            if (cJSON_IsString(id_obj)) {
                RPI_CONNECT_INFO("Device identity exchanged: device_id=%s\n",
                                 id_obj->valuestring);
                if (out_device_id)
                    *out_device_id = strdup(id_obj->valuestring);
            }
            if (cJSON_IsString(tok_obj))
                token = strdup(tok_obj->valuestring);
            cJSON_Delete(response);
        }
    }

    rpi_connect_memory_struct_free(&chunk);
    return token;
}

// Async download implementation

struct rpi_connect_download_context {
    rpi_connect_download_callback_t callback;
    rpi_connect_download_error_callback_t error_callback;
    void *callback_arg;
    memory_struct_t chunk;
    request_async_context_t request_async_context;
    char *hostname;
    int completed;
    size_t content_len; // response Content-Length, 0 if unknown (POSIX path)
};

// Parse scheme://hostname/path into hostname (allocated) and path (pointer into uri).
// For http:// the scheme is kept on the hostname. The scheme dictates transport
// selection.
static int parse_url(const char *uri, char **out_hostname, const char **out_path) {
    const char *p = uri;
    size_t scheme_len;

    if (strncmp(p, "https://", 8) == 0) {
        p += 8;
        scheme_len = 0;
    } else if (strncmp(p, "http://", 7) == 0) {
        scheme_len = 7;
    } else {
        return -1;
    }

    const char *slash = strchr(p + scheme_len, '/');
    if (!slash)
        return -1;

    size_t host_len = slash - p;
    char *hostname = malloc(host_len + 1);
    if (!hostname)
        return -1;

    memcpy(hostname, p, host_len);
    hostname[host_len] = '\0';

    *out_hostname = hostname;
    *out_path = slash;
    return 0;
}

#if PICO_ON_DEVICE
// Device (lwip) path: truly async download

static void process_download_data(void *arg) {
    rpi_connect_download_context_t *ctx = (rpi_connect_download_context_t *)arg;

    // Forward buffered data to user callback
    if (ctx->chunk.size > 0 && !ctx->completed) {
        RPI_CONNECT_VERBOSE_DEBUG("async-download %zu completed %d\n", ctx->chunk.size, ctx->completed);
        int rc = ctx->callback(ctx->chunk.memory, ctx->chunk.size, ctx->callback_arg);
        ctx->chunk.size = 0;
        if (rc != 0) {
            // User requested abort
            ctx->completed = 1;
        }
    }
}

// Fires once from the async worker after the request completes (or errors).
// Translates the final http_code into the existing data/error callback contract
// so callers don't need to poll rpi_connect_download_poll to learn the outcome.
static void process_download_complete(long http_code, void *arg) {
    rpi_connect_download_context_t *ctx = (rpi_connect_download_context_t *)arg;
    if (ctx->completed)
        return;
    ctx->completed = 1;
    if (http_code == 200 || http_code == 206) {
        ctx->callback(NULL, 0, ctx->callback_arg);
    } else {
        int error_code = (int)(http_code > 0 ? http_code : -1);
        if (ctx->error_callback)
            ctx->error_callback(error_code, ctx->callback_arg);
    }
}

rpi_connect_download_context_t *rpi_connect_async_download(
    async_context_t *context, const char *uri,
    rpi_connect_download_callback_t callback,
    rpi_connect_download_error_callback_t error_callback,
    void *callback_arg, bool tls_no_verify) {
    if (!context || !uri || !callback)
        return NULL;

    char *hostname = NULL;
    const char *path = NULL;

    if (parse_url(uri, &hostname, &path) != 0)
        return NULL;

    RPI_CONNECT_INFO("%s host: %s path: %s\n", __func__, hostname, path);
    if (tls_no_verify)
        RPI_CONNECT_INFO("%s: TLS certificate verification disabled for this download\n", __func__);

    rpi_connect_download_context_t *ctx = calloc(1, sizeof(rpi_connect_download_context_t));
    if (!ctx) {
        free(hostname);
        return NULL;
    }

    ctx->callback = callback;
    ctx->error_callback = error_callback;
    ctx->callback_arg = callback_arg;
    ctx->hostname = hostname;
    ctx->completed = 0;
    ctx->request_async_context.process_data = process_download_data;
    ctx->request_async_context.process_data_arg = ctx;
    ctx->request_async_context.process_complete = process_download_complete;
    ctx->request_async_context.process_complete_arg = ctx;
    rpi_connect_memory_struct_init(&ctx->chunk);

    long result = rpi_connect_request_perform_http(
        HTTP_GET,
        hostname,
        path,
        NULL,   // no token
        NULL,   // no additional headers
        NULL,   // no content type
        NULL,   // no data
        0,
        &ctx->chunk,
        NULL,   // no serial number
        context,
        &ctx->request_async_context,
        tls_no_verify
    );

    if (result != 0) {
        rpi_connect_memory_struct_free(&ctx->chunk);
        free(hostname);
        free(ctx);
        return NULL;
    }

    return ctx;
}

int rpi_connect_download_poll(rpi_connect_download_context_t *context) {
    // Completion (data_cb(NULL,0) + error_cb) is fired from process_download_complete,
    // which runs from the async worker. Pumping the worker here keeps the polling-mode
    // API working for callers that don't drive async_context_poll() themselves.
    if (!context->completed)
        rpi_connect_request_http_async_poll(&context->request_async_context);
    return context->completed;
}

void rpi_connect_download_stop(rpi_connect_download_context_t *context) {
    if (!context)
        return;

    rpi_connect_request_http_async_cleanup(&context->request_async_context);
    rpi_connect_memory_struct_free(&context->chunk);
    free(context->hostname);
    free(context);
}

size_t rpi_connect_download_content_length(rpi_connect_download_context_t *context) {
    if (!context)
        return 0;
    return rpi_connect_request_http_content_length(&context->request_async_context);
}

#else
// POSIX (curl) path: synchronous download, streamed to callback

rpi_connect_download_context_t *rpi_connect_async_download(
    __unused async_context_t *context, const char *uri,
    rpi_connect_download_callback_t callback,
    rpi_connect_download_error_callback_t error_callback,
    void *callback_arg, bool tls_no_verify) {

    if (!uri || !callback)
        return NULL;

    if (tls_no_verify)
        RPI_CONNECT_INFO("%s: TLS certificate verification disabled for this download\n", __func__);

    char *hostname = NULL;
    const char *path = NULL;

    if (parse_url(uri, &hostname, &path) != 0)
        return NULL;

    rpi_connect_download_context_t *ctx = calloc(1, sizeof(rpi_connect_download_context_t));
    if (!ctx) {
        free(hostname);
        return NULL;
    }

    ctx->callback = callback;
    ctx->error_callback = error_callback;
    ctx->callback_arg = callback_arg;
    ctx->hostname = hostname;
    ctx->completed = 0;
    rpi_connect_memory_struct_init(&ctx->chunk);

    // Synchronous download - blocks until complete
    long http_code = rpi_connect_request_perform_http(
        HTTP_GET,
        hostname,
        path,
        NULL,   // no token
        NULL,   // no additional headers
        NULL,   // no content type
        NULL,   // no data
        0,
        &ctx->chunk,
        NULL,   // no serial number
        NULL,   // async_context (unused on POSIX)
        NULL,   // sync mode
        tls_no_verify
    );

    if (http_code == 200 || http_code == 206) {
        // Forward the downloaded data to the callback
        RPI_CONNECT_VERBOSE_DEBUG("%s http-code %ld\n", __func__, http_code);
        // Synchronous download: the full body is buffered, so the received
        // size is the content length.
        ctx->content_len = ctx->chunk.size;
        if (ctx->chunk.size > 0)
            ctx->callback(ctx->chunk.memory, ctx->chunk.size, ctx->callback_arg);
        // Signal completion
        ctx->callback(NULL, 0, ctx->callback_arg);
    } else {
        int error_code = (int)(http_code > 0 ? http_code : -1);
        if (ctx->error_callback)
            ctx->error_callback(error_code, ctx->callback_arg);
    }
    ctx->completed = 1;

    return ctx;
}

int rpi_connect_download_poll(rpi_connect_download_context_t *context) {
    return context->completed;
}

void rpi_connect_download_stop(rpi_connect_download_context_t *context) {
    if (!context)
        return;

    rpi_connect_memory_struct_free(&context->chunk);
    free(context->hostname);
    free(context);
}

size_t rpi_connect_download_content_length(rpi_connect_download_context_t *context) {
    // POSIX downloads are synchronous; content_len is set once the transfer
    // completes (equal to the buffered body size).
    return context ? context->content_len : 0;
}

#endif

// Connect event handling implementation
// Example stream
//
// event: welcome
// data: {"type":"welcome","sid":"GnG0i7paML5ysUzHd26NA"}
//
// data: {"command":"deploy","data":{"id":"d3f194e3-3e76-4bdd-8b83-9fa1297f1ac5"}}
//
// event: confirm_subscription
// data: {"type":"confirm_subscription","identifier":"{\"channel\":\"DeviceChannel\"}"}

rpi_connect_event_context_t *rpi_connect_event_listen(async_context_t *context, const char *token, rpi_connect_event_callback_t callback, void *callback_arg) {
    rpi_connect_event_context_t *ctx = calloc(1, sizeof(rpi_connect_event_context_t));
    if (!ctx)
        return NULL;

    ctx->token = strdup(token);
    ctx->callback = callback;
    ctx->callback_arg = callback_arg;
    ctx->request_async_context.process_data = process_event_stream;
    ctx->request_async_context.process_data_arg = ctx;
    rpi_connect_memory_struct_init(&ctx->chunk);

    // Set up async request for events endpoint. A GET has no body, so ask
    // for the SSE stream with Accept rather than a Content-Type header.
    struct curl_slist *additional_headers = curl_slist_append(NULL, "Accept: text/event-stream");
    const char *endpoint = "/events?channel=DeviceChannel";
    long result = rpi_connect_request_perform_http(
        HTTP_GET,
        rpi_connect_event_host(),
        endpoint,
        token,
        additional_headers,
        NULL,
        NULL,
        0,
        &ctx->chunk,
        NULL,  // serial_number
        context,
        &ctx->request_async_context,
        false   // tls_no_verify
    );
    curl_slist_free_all(additional_headers);

    if (result != 0) {
        free(ctx->token);
        rpi_connect_memory_struct_free(&ctx->chunk);
        free(ctx);
        return NULL;
    }

    return ctx;
}

static void parse_event(rpi_connect_event_context_t *ctx, const char *event_type, const char *data) {
    rpi_connect_event_t event = {0};
    cJSON *json = cJSON_Parse(data);
    if (!json) {
        return;
    }

    if (strcmp(event_type, "welcome") == 0) {
        event.type = RPI_CONNECT_EVENT_TYPE_WELCOME;
        cJSON *sid = cJSON_GetObjectItem(json, "sid");
        if (cJSON_IsString(sid)) {
            event.data.welcome.sid = sid->valuestring;
        }
    } else if (strcmp(event_type, "confirm_subscription") == 0) {
        event.type = RPI_CONNECT_EVENT_TYPE_CONFIRM_SUBSCRIPTION;
        cJSON *identifier = cJSON_GetObjectItem(json, "identifier");
        if (cJSON_IsString(identifier)) {
            event.data.confirm_subscription.identifier = identifier->valuestring;
        }
    } else if (strcmp(event_type, "ping") == 0) {
        event.type = RPI_CONNECT_EVENT_TYPE_PING;
        cJSON *message = cJSON_GetObjectItem(json, "message");
        if (cJSON_IsNumber(message)) {
            event.data.ping.timestamp = message->valuedouble;
        }
    } else if (strcmp(event_type, "message") == 0) {
        cJSON *command = cJSON_GetObjectItem(json, "command");
        if (cJSON_IsString(command) && strcmp(command->valuestring, "deploy") == 0) {
            event.type = RPI_CONNECT_EVENT_TYPE_DEPLOY;
            cJSON *deploy_data = cJSON_GetObjectItem(json, "data");
            if (deploy_data) {
                cJSON *id = cJSON_GetObjectItem(deploy_data, "id");

                // Point directly to the JSON strings
                event.data.deploy.id = cJSON_IsString(id) ? id->valuestring : NULL;
            }
        }
    } else {
        // Handle unknown event types
        event.type = RPI_CONNECT_EVENT_TYPE_UNKNOWN;
        event.data.unknown.event_type = event_type;
        event.data.unknown.data = data;
    }

    // Call the callback with the event while JSON is still valid
    if (ctx->callback) {
        ctx->callback(&event, ctx->callback_arg);
    }

    // Now we can delete the JSON since we're done with the string pointers
    cJSON_Delete(json);
}

// The event stream is implemented using HTTP Server-Sent Events (SSE)
// This function processes the received data chunk and extracts events and
// handles partial data.
static void process_event_stream(void *arg) {
    rpi_connect_event_context_t *ctx = (rpi_connect_event_context_t *)arg;
    char *data = ctx->chunk.memory;
    size_t data_len = ctx->chunk.size;
    char *line_start = data;
    char *line_end;
    char *event_type = NULL;
    char *event_data = NULL;

    while ((line_end = strstr(line_start, "\n")) != NULL) {
        *line_end = '\0';

        // Skip empty lines - they mark the end of an event
        if (line_start[0] == '\0') {
            if (event_data) {
                // If we have data but no event type, treat it as a "message" event
                parse_event(ctx, event_type ? event_type : "message", event_data);
                event_type = NULL;
                event_data = NULL;
            }
        } else if (strncmp(line_start, "event:", 6) == 0) {
            event_type = line_start + 7;
            while (*event_type == ' ') event_type++;
        } else if (strncmp(line_start, "data:", 5) == 0) {
            event_data = line_start + 6;
            while (*event_data == ' ') event_data++;
        }

        line_start = line_end + 1;
    }

    // Keep any partial data
    if (line_start < data + data_len) {
        size_t remaining = data + data_len - line_start;
        memmove(data, line_start, remaining);
        ctx->chunk.size = remaining;
    } else {
        ctx->chunk.size = 0;
    }
}

// Polls for events, a callback is dispatched for each outstanding event before returning
int rpi_connect_event_poll(rpi_connect_event_context_t *context) {
    return rpi_connect_request_http_async_poll(&context->request_async_context);
}

// Stop the event listener and close the connection
void rpi_connect_event_stop(rpi_connect_event_context_t *context) {
    if (!context)
        return;

    rpi_connect_request_http_async_cleanup(&context->request_async_context);
    rpi_connect_memory_struct_free(&context->chunk);
    free(context->token);
    free(context);
}

