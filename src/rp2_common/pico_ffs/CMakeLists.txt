pico_add_library(pico_ffs)

target_sources(pico_ffs INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/ffs.c
        ${CMAKE_CURRENT_LIST_DIR}/ffs_internal.c
)

target_include_directories(pico_ffs SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

target_link_libraries(pico_ffs INTERFACE boot_picobin_headers hardware_flash pico_bootrom pico_stdlib)

function(pico_ffs_set_file_id_vars)
        set(FFS_AUTH_TOKEN_FILE_ID 0x1 PARENT_SCOPE)
        set(FFS_DEPLOYMENT_ID_FILE_ID 0x2 PARENT_SCOPE)
        set(FFS_DEPLOYMENT_URI_FILE_ID 0x3 PARENT_SCOPE)
        set(FFS_DEPLOYMENT_CHECKSUM_FILE_ID 0x4 PARENT_SCOPE)
        set(FFS_DEPLOYMENT_STATUS_FILE_ID 0x5 PARENT_SCOPE)
        set(FFS_WIFI_SSID_FILE_ID 0x10 PARENT_SCOPE)
        set(FFS_WIFI_PASSWORD_FILE_ID 0x11 PARENT_SCOPE)
endfunction()

function(pico_ffs_check_cmake_matches_header)
        pico_ffs_set_file_id_vars()
        message("Checking FFS file IDs in CMake match values in ffs_file_ids.h")
        file(STRINGS ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/include/pico/ffs_file_ids.h file_ids_h)
        foreach(line IN LISTS file_ids_h)
                if (${line} MATCHES "#define (.*_FILE_ID) (0x..?)")
                        set(header_var ${CMAKE_MATCH_1})
                        set(header_val ${CMAKE_MATCH_2})
                        if (NOT "${${header_var}}" STREQUAL "${header_val}")
                                message(FATAL_ERROR "Conflicting definitions for ${header_var}: header ${header_val}, cmake ${${header_var}}")
                        endif()
                endif()
        endforeach()
endfunction()
