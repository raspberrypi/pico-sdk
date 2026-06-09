/**
 * \defgroup pico_mbedtls pico_mbedtls
 * \brief pico-sdk wrapper library for <a href="https://github.com/Mbed-TLS/mbedtls.git">mbedtls</a>
 * the documentation for which is <a href="https://mbed-tls.readthedocs.io/en/latest/">here</a>.
 *
 * Builds mbedtls for pico-sdk and implements functions to take advantage of hardware support, if enabled in mbedtls_config.h
 * 
 * * \c \b MBEDTLS_ENTROPY_HARDWARE_ALT, implementation of a hardware entropy collector that uses \ref get_rand_64
 * * \c \b MBEDTLS_SHA256_ALT, use SHA256 hardware acceleration. Only valid if LIB_PICO_SHA256 is defined (i.e. not available for rp2040)
 *
 */
