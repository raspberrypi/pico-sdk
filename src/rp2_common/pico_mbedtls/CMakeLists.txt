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

    function(extract_mbedtls_file_lists FAILURE_REASON)
        set(MBEDTLS_LIBRARY_PATH "${PICO_MBEDTLS_PATH}/library/")
        set(${FAILURE_REASON} "" PARENT_SCOPE)
        # file is pretty predictable, so do a hacky parse rather than nested cmake
        if (EXISTS "${MBEDTLS_LIBRARY_PATH}/CMakeLists.txt")
            file(READ "${MBEDTLS_LIBRARY_PATH}/CMakeLists.txt" FILE_CONTENTS)

            # take multiline from set up to closing )
            if(FILE_CONTENTS MATCHES "set\\(src_crypto([^\\)]*)")
                string(REGEX REPLACE "[ \r\n]+" ";" mbedtls_src_crypto "${CMAKE_MATCH_1}")
                list(REMOVE_ITEM mbedtls_src_crypto "")
            else()
                set(${FAILURE_REASON} "src_crypto variable not extracted" PARENT_SCOPE)
                return()
            endif()
            # take multiline from set up to closing )
            if(FILE_CONTENTS MATCHES "set\\(src_x509([^\\)]*)")
                string(REGEX REPLACE "[ \r\n]+" ";" mbedtls_src_x509 "${CMAKE_MATCH_1}")
                list(REMOVE_ITEM mbedtls_src_x509 "")
            else()
                set(${FAILURE_REASON} "src_x509 variable not extracted" PARENT_SCOPE)
                return()
            endif()
            # take multiline from set up to closing )
            if(FILE_CONTENTS MATCHES "set\\(src_tls([^\\)]*)")
                string(REGEX REPLACE "[ \r\n]+" ";" mbedtls_src_tls "${CMAKE_MATCH_1}")
                list(REMOVE_ITEM mbedtls_src_tls "")
            else()
                set(${FAILURE_REASON} "src_tls variable not extracted" PARENT_SCOPE)
                return()
            endif()
        else()
            set(${FAILURE_REASON} "${MBEDTLS_LIBRARY_PATH}/CMakeLists.txt does not exist" PARENT_SCOPE)
            return()
        endif()
        list(TRANSFORM mbedtls_src_crypto PREPEND "${MBEDTLS_LIBRARY_PATH}")
        list(TRANSFORM mbedtls_src_x509 PREPEND "${MBEDTLS_LIBRARY_PATH}")
        list(TRANSFORM mbedtls_src_tls PREPEND "${MBEDTLS_LIBRARY_PATH}")
        set(mbedtls_src_crypto ${mbedtls_src_crypto} PARENT_SCOPE)
        set(mbedtls_src_x509 ${mbedtls_src_x509} PARENT_SCOPE)
        set(mbedtls_src_tls ${mbedtls_src_tls} PARENT_SCOPE)
        unset(MBEDTLS_LIBRARY_PATH)
    endfunction()

    extract_mbedtls_file_lists(FAILURE_REASON)
    if (FAILURE_REASON)
        message(WARNING "Could not configure mbedtls: ${FAILURE_REASON}")
        return()
    endif()
    pico_add_library(pico_mbedtls_crypto NOFLAG)
    target_sources(pico_mbedtls_crypto INTERFACE ${mbedtls_src_crypto})

    pico_add_library(pico_mbedtls_x509 NOFLAG)
    target_sources(pico_mbedtls_x509 INTERFACE ${mbedtls_src_x509})

    pico_add_library(pico_mbedtls_tls NOFLAG)
    target_sources(pico_mbedtls_tls INTERFACE ${mbedtls_src_tls})

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
        extract_mbedtls_file_lists(FAILURE_REASON) # this succeeded before, so should again

        foreach(src_file IN LISTS mbedtls_src_crypto mbedtls_src_x509 mbedtls_src_tls)
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
