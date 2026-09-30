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


    # Default header libraries
    add_library(pico_rpi_connect_ota_default_config_headers INTERFACE)
    target_include_directories(pico_rpi_connect_ota_default_config_headers SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/default_config_headers)
endif()


# pico_add_rpi_connect_ota_outputs(TARGET CONFIGURATION [WIFI_SSID <ssid>] [WIFI_PASSWORD <password>] [CONNECT_TOKEN <token>] [COMBINED_TARGET <name>] [NO_BOARD] [NO_DATE] [UPDATE_DIR <directory>] [PARTITION_TABLE <path>])
# \brief_nodesc\ Add UF2 outputs for use with rpi_connect_ota
#
# Adds UF2 outputs created from TARGET for use with rpi_connect_ota,
# including a combined UF2 for provisioning, and an update UF2 with a
# corresponding sha256sum for updating with.
#
# The update UF2s and sha256sums are copied to the UPDATE_DIR, which defaults
# to ${CMAKE_CURRENT_LIST_DIR}/updates (which is in the source tree).
# The UF2 filename will include the major, minor, and rollback versions,
# and by default also includes the date and PICO_BOARD - these can be
# disabled using NO_DATE and NO_BOARD
#
# The combined target name is COMBINED_TARGET, which defaults to
# <TARGET>_combined.
#
# The available configurations are:
# - default - Main A/B partitions, WiFi firmware A/B partitions, and FFS partition
# - no_wifi_firmware - Main A/B partitions, and FFS partition
#
# \param\ TARGET The target to add outputs for
# \param\ CONFIGURATION Pick the configuration to use - options are: default, no_wifi_firmware
# \param\ WIFI_SSID Wifi SSID to put in FFS - if not specified will be left blank
# \param\ WIFI_PASSWORD Wifi password to put in FFS - if not specified will be left blank
# \param\ CONNECT_TOKEN Raspberry Pi Connect token to put in FFS - if not specified will not be included in FFS
# \param\ COMBINED_TARGET The name to use for the combined target - defaults to <TARGET>_combined
# \param\ NO_BOARD Don't include the board name in the update filename
# \param\ NO_DATE Don't include the current date in the update filename
# \param\ UPDATE_DIR Directory to place the update UF2 in - defaults to ${CMAKE_CURRENT_LIST_DIR}/updates
# \param\ PARTITION_TABLE Specify custom path to partition table to use - must match the partition layout expected by the configuration
function(pico_add_rpi_connect_ota_outputs TARGET CONFIGURATION)
    set(options NO_DATE NO_BOARD)
    set(oneValueArgs WIFI_SSID WIFI_PASSWORD CONNECT_TOKEN COMBINED_TARGET UPDATE_DIR PARTITION_TABLE)
    cmake_parse_arguments(PARSE_ARGV 2 OPT "${options}" "${oneValueArgs}" "${multiValueArgs}")

    if (NOT OPT_COMBINED_TARGET)
        set(OPT_COMBINED_TARGET ${TARGET}_combined)
    endif()

    if (NOT OPT_UPDATE_DIR)
        set(OPT_UPDATE_DIR "${CMAKE_CURRENT_LIST_DIR}/updates")
    endif()

    if (OPT_PARTITION_TABLE)
        pico_create_partition_table_uf2(${TARGET}_pt ${OPT_PARTITION_TABLE})
    else()
        # 8k at start for 2 slots, 8k at end for btstack flash bank, plus 4k if E10 applies
        if (PICO_RP2350_A2_SUPPORTED)
            set(USED_SIZE "20")
        else()
            set(USED_SIZE "16")
        endif()

        set(FFS_SIZE "8")
        set(WIFI_FW_SIZE "256")
        math(EXPR FLASH_SIZE_K "(${PICO_FLASH_SIZE_BYTES})/1024")
        if (CONFIGURATION STREQUAL "default")
            math(EXPR MAIN_SIZE "(${FLASH_SIZE_K} - (${WIFI_FW_SIZE}*2) - ${FFS_SIZE} - ${USED_SIZE}) / 2")
            if (NOT TARGET ${TARGET}_wifi_firmware)
                # Ensure TARGET is configured to use wifi firmware from partition
                # (no embedded PT as using FFS one, and no TBYB variant as that's created below)
                pico_use_wifi_firmware_partition(${TARGET} NO_EMBEDDED_PT NO_TBYB)
            endif()
        elseif (CONFIGURATION STREQUAL "no_wifi_firmware")
            math(EXPR MAIN_SIZE "(${FLASH_SIZE_K} - ${FFS_SIZE} - ${USED_SIZE}) / 2")
        else()
            message(FATAL_ERROR "Unknown rpi_connect_ota configuration '${CONFIGURATION}'")
        endif()

        configure_file(${CMAKE_CURRENT_FUNCTION_LIST_DIR}/partition_tables/${CONFIGURATION}.template.json ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.pt.json @ONLY)

        pico_create_partition_table_uf2(${TARGET}_pt ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.pt.json)
    endif()

    pico_ffs_set_file_id_vars()

    file(CONFIGURE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.wifi_ssid CONTENT "${OPT_WIFI_SSID}")
    list(APPEND ffs_files ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.wifi_ssid)
    list(APPEND ffs_file_ids ${FFS_WIFI_SSID_FILE_ID})
    file(CONFIGURE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.wifi_password CONTENT "${OPT_WIFI_PASSWORD}")
    list(APPEND ffs_files ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.wifi_password)
    list(APPEND ffs_file_ids ${FFS_WIFI_PASSWORD_FILE_ID})

    if (OPT_CONNECT_TOKEN)
        file(CONFIGURE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.connect_token CONTENT "${OPT_CONNECT_TOKEN}")
        list(APPEND ffs_files ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.connect_token)
        list(APPEND ffs_file_ids ${FFS_AUTH_TOKEN_FILE_ID})
    endif()

    pico_create_ffs_blob_uf2(${TARGET}_ffs_blob ${ffs_files} FILE_IDS ${ffs_file_ids})

    pico_create_erase_partition_uf2(${TARGET}_erase)

    if (CONFIGURATION STREQUAL "default")
        pico_combine_uf2_outputs(${OPT_COMBINED_TARGET} ${TARGET}_pt
            ${TARGET} ${TARGET}_erase
            ${TARGET}_wifi_firmware ${TARGET}_erase
            ${TARGET}_ffs_blob
            PARTITIONS 0 1 2 3 4)
    elseif (CONFIGURATION STREQUAL "no_wifi_firmware")
        pico_combine_uf2_outputs(${OPT_COMBINED_TARGET} ${TARGET}_pt
            ${TARGET} ${TARGET}_erase
            ${TARGET}_ffs_blob
            PARTITIONS 0 1 2)
    else()
        message(FATAL_ERROR "Unknown rpi_connect_ota configuration '${CONFIGURATION}'")
    endif()

    set(update_suffix "")

    get_target_property(rollback ${TARGET} PICOTOOL_ROLLBACK_VERSION)
    if (rollback) # 0 rollback version isn't supported
        set(update_suffix "${update_suffix}-r${rollback}")
    endif()
    get_target_property(major ${TARGET} PICOTOOL_MAJOR_VERSION)
    if (NOT major)
        set(major 0)
    endif()
    get_target_property(minor ${TARGET} PICOTOOL_MINOR_VERSION)
    if (NOT minor)
        set(minor 0)
    endif()

    set(update_suffix "${update_suffix}-v${major}.${minor}")

    if (NOT OPT_NO_BOARD)
        set(update_suffix "${update_suffix}-${PICO_BOARD}")
    endif()

    if (NOT OPT_NO_DATE)
        string(TIMESTAMP current_date "%Y-%m-%d")
        set(update_suffix "${update_suffix}-${current_date}")
    endif()

    pico_add_tbyb_uf2(${TARGET} OUTTARGET ${TARGET}_update OUTFILE ${OPT_UPDATE_DIR}/${TARGET}${update_suffix}.uf2)
    pico_add_uf2_sha256sum(${TARGET}_update)
    add_dependencies(${OPT_COMBINED_TARGET} ${TARGET}_update ${TARGET}_update_sha256sum)

    if (TARGET ${TARGET}_wifi_firmware)
        # Create wifi firmware update UF2 too
        pico_add_tbyb_uf2(${TARGET}_wifi_firmware OUTTARGET ${TARGET}_wifi_firmware_update OUTFILE ${OPT_UPDATE_DIR}/${TARGET}_wifi_firmware${update_suffix}.uf2)
        pico_add_uf2_sha256sum(${TARGET}_wifi_firmware_update)
        add_dependencies(${OPT_COMBINED_TARGET} ${TARGET}_wifi_firmware_update ${TARGET}_wifi_firmware_update_sha256sum)
    endif()
endfunction()
