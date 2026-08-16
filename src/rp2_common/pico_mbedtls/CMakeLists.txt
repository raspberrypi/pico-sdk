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

    # Support version 2.28.8, 3.6.2, 3.6.6
    if (NOT MBEDTLS_VERSION_MAJOR)
        if (EXISTS ${PICO_MBEDTLS_PATH}/library/ssl_cli.c)
            set(MBEDTLS_VERSION_MAJOR 2)
            set(MBEDTLS_VERSION_FULL 2.28.8)
        elseif (EXISTS ${PICO_MBEDTLS_PATH}/library/ssl_client.c)
            set(MBEDTLS_VERSION_MAJOR 3)
            if (EXISTS ${PICO_MBEDTLS_PATH}/library/psa_crypto_random.c)
                set(MBEDTLS_VERSION_FULL 3.6.6)
            else()
                set(MBEDTLS_VERSION_FULL 3.6.2)
            endif()
        else()
            message(WARNING "Cannot determine the version of mbedtls")
        endif()
        pico_register_common_scope_var(MBEDTLS_VERSION_MAJOR)
        pico_register_common_scope_var(MBEDTLS_VERSION_FULL)
    endif()

    function(src_mbedtls_list)
        include(${CMAKE_CURRENT_FUNCTION_LIST_DIR}/mbedtls_library_${MBEDTLS_VERSION_FULL}.cmake)
        list(TRANSFORM src_crypto PREPEND ${PICO_MBEDTLS_PATH}/library/)
        set(src_crypto ${src_crypto} PARENT_SCOPE)
        list(TRANSFORM src_x509 PREPEND ${PICO_MBEDTLS_PATH}/library/)
        set(src_x509 ${src_x509} PARENT_SCOPE)
        list(TRANSFORM src_tls PREPEND ${PICO_MBEDTLS_PATH}/library/)
        set(src_tls ${src_tls} PARENT_SCOPE)
    endfunction()

    src_mbedtls_list()

    pico_add_library(pico_mbedtls_crypto NOFLAG)
    target_sources(pico_mbedtls_crypto INTERFACE ${src_crypto})

    pico_add_library(pico_mbedtls_x509 NOFLAG)
    target_sources(pico_mbedtls_x509 INTERFACE ${src_x509})

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
        src_mbedtls_list()
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
