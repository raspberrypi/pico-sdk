if (NOT TARGET pico_rpi_connect)
    if (NOT PICO_ON_DEVICE)
        # Host build: the same library over libcurl/OpenSSL instead of lwip/mbedtls.
        find_package(CURL QUIET)
        find_package(OpenSSL QUIET)
        if (NOT TARGET CURL::libcurl OR NOT TARGET OpenSSL::Crypto)
            message("Skipping pico_rpi_connect as libcurl/OpenSSL are unavailable")
            return()
        endif()
    endif()
    add_library(pico_rpi_connect INTERFACE)
    target_include_directories(pico_rpi_connect SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
    target_sources(pico_rpi_connect INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/rpi_connect.c
        ${CMAKE_CURRENT_LIST_DIR}/connect_crypto.c
        ${CMAKE_CURRENT_LIST_DIR}/connect_time.c
        ${CMAKE_CURRENT_LIST_DIR}/request.c
        ${CMAKE_CURRENT_LIST_DIR}/slist.c
    )
    if (PICO_ON_DEVICE)
        target_sources(pico_rpi_connect INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/rpi_connect_http.c
            ${CMAKE_CURRENT_LIST_DIR}/request_lwip.c
        )
        # The lwIP HTTP client comes from the Raspberry Pi lwIP fork (lib/lwip),
        # which carries the http_client patches Pi Connect needs. Applications
        # must configure it in their lwipopts.h (see rpi_connect_http.c):
        #   HTTPC_SEND_ACCEPT_HEADER 0    - Pi Connect signs its request headers
        #   HTTPC_SEND_CONNECTION_CLOSE 0 - the event stream (SSE) stays open
        #   HTTPC_POLL_TIMEOUT 240        - long-lived requests (SSE, OTA
        #                                   downloads on slow links): 120 s
        target_link_libraries(pico_rpi_connect INTERFACE
            pico_cjson
            pico_lwip_http
            pico_mbedtls
            pico_stdlib
        )
    else()
        target_sources(pico_rpi_connect INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/request_curl.c
        )
        target_link_libraries(pico_rpi_connect INTERFACE
            pico_cjson
            pico_stdlib
            CURL::libcurl
            OpenSSL::Crypto
        )
    endif()
endif()

if (PICO_ON_DEVICE)
    # Ignore warnings from lwip code
    set_source_files_properties(
        ${PICO_LWIP_PATH}/src/apps/altcp_tls/altcp_tls_mbedtls.c
        PROPERTIES
        COMPILE_OPTIONS "-Wno-unused-result"
        )
endif()
