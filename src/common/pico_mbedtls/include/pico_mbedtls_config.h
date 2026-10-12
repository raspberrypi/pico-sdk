// Latest versions of mbedtls include mbedtls/include/mbedtls_config.h and we used to set MBEDTLS_CONFIG_FILE=mbedtls_config.h
// To maintain compatibility with this and avoid including the mbedtls version of mbedtls_config.h we include pico_mbedtls_config.h first
#include "mbedtls_config.h"