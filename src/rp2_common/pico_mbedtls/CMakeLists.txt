# PICO_CMAKE_CONFIG: PICO_MBEDTLS_PATH, Path to Mbed TLS. Can be passed to CMake or set in your environment if you do not wish to use the version included with the SDK, type=string, default=<PICO_SDK_PATH>/lib/mbedtls, group=pico_mbedtls
if (DEFINED ENV{PICO_MBEDTLS_PATH} AND (NOT PICO_MBEDTLS_PATH))
    set(PICO_MBEDTLS_PATH $ENV{PICO_MBEDTLS_PATH})
    message("Using PICO_MBEDTLS_PATH from environment ('${PICO_MBEDTLS_PATH}')")
endif()

set(MBEDTLS_TEST_PATH "library/aes.c")
if (NOT PICO_MBEDTLS_PATH)
    set(PICO_MBEDTLS_PATH ${PICO_SDK_PATH}/lib/mbedtls)
elseif (NOT EXISTS ${PICO_MBEDTLS_PATH}/${MBEDTLS_TEST_PATH})
    message(WARNING "PICO_MBEDTLS_PATH specified but content not present.")
endif()

if (EXISTS ${PICO_MBEDTLS_PATH}/${MBEDTLS_TEST_PATH})
    message("mbedtls available at ${PICO_MBEDTLS_PATH}")

    pico_register_common_scope_var(PICO_MBEDTLS_PATH)

    # Support version 2.28.8 or 3.6.2
    if (NOT MBEDTLS_VERSION_MAJOR)
        if (EXISTS ${PICO_MBEDTLS_PATH}/library/ssl_cli.c)
            set(MBEDTLS_VERSION_MAJOR 2)
        elseif (EXISTS ${PICO_MBEDTLS_PATH}/library/ssl_client.c)
            set(MBEDTLS_VERSION_MAJOR 3)
        else()
            message(WARNING "Cannot determine the version of mbedtls")
        endif()
        pico_register_common_scope_var(MBEDTLS_VERSION_MAJOR)
    endif()

    function(src_crypto_list)
        set(src_crypto
            aes.c
            aesni.c
            aria.c
            asn1parse.c
            asn1write.c
            base64.c
            bignum.c
            camellia.c
            ccm.c
            chacha20.c
            chachapoly.c
            cipher.c
            cipher_wrap.c
            constant_time.c
            cmac.c
            ctr_drbg.c
            des.c
            dhm.c
            ecdh.c
            ecdsa.c
            ecjpake.c
            ecp.c
            ecp_curves.c
            entropy.c
            entropy_poll.c
            error.c
            gcm.c
            hkdf.c
            hmac_drbg.c
            md.c
            md5.c
            memory_buffer_alloc.c
            mps_reader.c
            mps_trace.c
            nist_kw.c
            oid.c
            padlock.c
            pem.c
            pk.c
            pk_wrap.c
            pkcs12.c
            pkcs5.c
            pkparse.c
            pkwrite.c
            platform.c
            platform_util.c
            poly1305.c
            psa_crypto.c
            psa_crypto_aead.c
            psa_crypto_cipher.c
            psa_crypto_client.c
            psa_crypto_ecp.c
            psa_crypto_hash.c
            psa_crypto_mac.c
            psa_crypto_rsa.c
            psa_crypto_se.c
            psa_crypto_slot_management.c
            psa_crypto_storage.c
            psa_its_file.c
            ripemd160.c
            rsa.c
            sha1.c
            sha256.c
            sha512.c
            threading.c
            timing.c
            version.c
            version_features.c
        )
        if (MBEDTLS_VERSION_MAJOR EQUAL 2)
            list(APPEND src_crypto
                arc4.c
                blowfish.c
                havege.c
                md2.c
                md4.c
                psa_crypto_driver_wrappers.c
                rsa_internal.c xtea.c
            )
        elseif (MBEDTLS_VERSION_MAJOR EQUAL 3)
            list(APPEND src_crypto
                bignum_core.c
                rsa_alt_helpers.c
                pk_ecc.c
            )
        endif()
        list(TRANSFORM src_crypto PREPEND ${PICO_MBEDTLS_PATH}/library/)
        set(src_crypto ${src_crypto} PARENT_SCOPE)
    endfunction()

    src_crypto_list()
    pico_add_library(pico_mbedtls_crypto NOFLAG)
    target_sources(pico_mbedtls_crypto INTERFACE ${src_crypto})

    function(src_x509_list)
        set(src_x509
            x509.c
            x509_create.c
            x509_crl.c
            x509_crt.c
            x509_csr.c
            x509write_crt.c
            x509write_csr.c
        )
        if (MBEDTLS_VERSION_MAJOR EQUAL 2)
            list(APPEND src_x509
                certs.c
                pkcs11.c
            )
        endif()
        list(TRANSFORM src_x509 PREPEND ${PICO_MBEDTLS_PATH}/library/)
        set(src_x509 ${src_x509} PARENT_SCOPE)
    endfunction()

    src_x509_list()
    pico_add_library(pico_mbedtls_x509 NOFLAG)
    target_sources(pico_mbedtls_x509 INTERFACE ${src_x509})

    function(src_tls_list)
        set(src_tls
            debug.c
            net_sockets.c
            ssl_cache.c
            ssl_ciphersuites.c
            ssl_cookie.c
            ssl_msg.c
            ssl_ticket.c
            ssl_tls.c
            ssl_tls13_keys.c
        )
        if (MBEDTLS_VERSION_MAJOR EQUAL 2)
            list(APPEND src_tls
                ssl_cli.c
                ssl_srv.c
            )
        elseif (MBEDTLS_VERSION_MAJOR EQUAL 3)
            list(APPEND src_tls
                ssl_client.c
                ssl_debug_helpers_generated.c
                ssl_tls12_client.c
                ssl_tls12_server.c
            )
        endif()
        list(TRANSFORM src_tls PREPEND ${PICO_MBEDTLS_PATH}/library/)
        set(src_tls ${src_tls} PARENT_SCOPE)
    endfunction()

    src_tls_list()
    pico_add_library(pico_mbedtls_tls NOFLAG)
    target_sources(pico_mbedtls_tls INTERFACE ${src_tls})

    pico_add_library(pico_mbedtls NOFLAG)
    pico_mirrored_target_link_libraries(pico_mbedtls INTERFACE pico_mbedtls_crypto pico_mbedtls_x509 pico_mbedtls_tls pico_rand)
    if (DEFINED PICO_MBEDTLS_CONFIG_FILE)
        target_compile_definitions(pico_mbedtls_headers INTERFACE MBEDTLS_CONFIG_FILE="${PICO_MBEDTLS_CONFIG_FILE}")
    else()
        if (MBEDTLS_VERSION_MAJOR EQUAL 2)
            target_compile_definitions(pico_mbedtls_headers INTERFACE MBEDTLS_CONFIG_FILE="mbedtls_config.h")
        else()
            # Avoid including mbedtls/include/mbedtls_config.h
            target_compile_definitions(pico_mbedtls_headers INTERFACE MBEDTLS_CONFIG_FILE="pico_mbedtls_config.h")
        endif()
    endif()
    if (TARGET pico_sha256)
        pico_mirrored_target_link_libraries(pico_mbedtls INTERFACE pico_sha256)
    endif()
    target_sources(pico_mbedtls INTERFACE ${CMAKE_CURRENT_LIST_DIR}/pico_mbedtls.c)
    target_include_directories(pico_mbedtls_headers SYSTEM INTERFACE ${PICO_MBEDTLS_PATH}/include/ ${PICO_MBEDTLS_PATH}/library/ ${CMAKE_CURRENT_LIST_DIR}/include/)

    function(suppress_mbedtls_warnings)
        # It seems everything needs this due to mbedtls_get_unaligned_uint64
        src_crypto_list()
        src_x509_list()
        src_tls_list()
        foreach(src_file IN LISTS src_crypto src_x509 src_tls)
            set_source_files_properties(
                ${src_file}
                PROPERTIES
                COMPILE_OPTIONS "-Wno-cast-qual"
            )
        endforeach()

        set_source_files_properties(
            ${PICO_MBEDTLS_PATH}/library/ssl_srvx.c
            ${PICO_MBEDTLS_PATH}/library/x509_crt.c
            ${PICO_MBEDTLS_PATH}/library/pk_ecc.c
            ${PICO_MBEDTLS_PATH}/library/ssl_tls12_server.c
            ${PICO_MBEDTLS_PATH}/library/ssl_tls.c
            PROPERTIES
            COMPILE_OPTIONS "-Wno-cast-qual;-Wno-null-dereference"
        )
    endfunction()

    pico_promote_common_scope_vars()
endif()
