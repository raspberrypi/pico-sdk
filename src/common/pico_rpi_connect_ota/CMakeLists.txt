if (NOT PICO_ON_DEVICE AND NOT TARGET pico_rpi_connect)
    # pico_rpi_connect was skipped (no libcurl/OpenSSL on this host).
    return()
endif()
if (PICO_ON_DEVICE AND PICO_RP2040)
    # OTA relies on the RP2350 bootrom flash-update mechanism, A/B partitions
    # and OTP (via pico_flash_image, which is likewise RP2350-only), so it is
    # not available on RP2040.
    return()
endif()

if (NOT TARGET pico_rpi_connect_ota)
    # OTP row holding the 32-byte P-256 device identity private key.
    # Override on the cmake command line (-DRPI_CONNECT_IDENTITY_OTP_ROW=0xd0)
    # or by adding RPI_CONNECT_IDENTITY_OTP_ROW=... to the consuming target's
    # PRIVATE compile definitions (later -D wins over the INTERFACE default).
    set(RPI_CONNECT_IDENTITY_OTP_ROW "0xc0" CACHE STRING
        "OTP row where the device identity P-256 private key is stored")

    add_library(pico_rpi_connect_ota INTERFACE)
    target_include_directories(pico_rpi_connect_ota SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
    # connect_crypto.h lives in the pico_rpi_connect source dir (not its public include).
    # The OTA library uses it for deriving the public key during device-identity-exchange.
    target_include_directories(pico_rpi_connect_ota INTERFACE ${CMAKE_CURRENT_LIST_DIR}/../pico_rpi_connect)
    target_sources(pico_rpi_connect_ota INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/rpi_connect_ota.c
    )
    target_compile_definitions(pico_rpi_connect_ota INTERFACE
        RPI_CONNECT_IDENTITY_OTP_ROW=${RPI_CONNECT_IDENTITY_OTP_ROW}
    )
    if (PICO_ON_DEVICE)
        target_link_libraries(pico_rpi_connect_ota INTERFACE
            pico_ffs
            pico_flash_image
            pico_rpi_connect
            pico_stdlib
        )
    else()
        # Host stand-ins for the bootrom/flash-image parts of the API; the FFS
        # store comes from the host pico_ffs.
        target_sources(pico_rpi_connect_ota INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/rpi_connect_ota_host.c
        )
        target_link_libraries(pico_rpi_connect_ota INTERFACE
            pico_ffs
            pico_rpi_connect
            pico_stdlib
        )
    endif()
endif()
