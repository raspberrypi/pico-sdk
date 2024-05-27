if (NOT PICO_VFS_PATH)
    set(PICO_VFS_PATH ${PROJECT_SOURCE_DIR}/lib/pico-vfs)
    if (NOT EXISTS ${PICO_VFS_PATH}/tests)
        message(WARNING "pico-vfs submodule has not been initialized: File system support will be unavailable
hint: try 'get submodule update --init' from your SDK directory (${PICO_SDK_PATH}).")
    endif()
elseif (NOT EXISTS ${PICO_VFS_PATH}/tests)
    message(WARNING "PICO_VFS_PATH specified but content not present.")
endif()

if (EXISTS ${PICO_VFS_PATH}/tests)
    message("pico-vfs available at ${PICO_VFS_PATH}/tests; enabling build support for file system.")

    pico_register_common_scope_var(PICO_VFS_PATH)

    set(BOARD pico_sdk)
    set(FAMILY rp2040)

    pico_add_library(pico_filesystem_blockdevice_sd)
    target_sources(pico_filesystem_blockdevice_sd INTERFACE
        ${PICO_VFS_PATH}/src/blockdevice/sd.c)
    target_include_directories(pico_filesystem_blockdevice_sd INTERFACE
        ${PICO_VFS_PATH}/include)
    target_link_libraries(pico_filesystem_blockdevice_sd INTERFACE
        hardware_spi)

    pico_add_library(pico_filesystem_blockdevice_flash)
    target_sources(pico_filesystem_blockdevice_flash INTERFACE
        ${PICO_VFS_PATH}/src/blockdevice/flash.c)
    target_include_directories(pico_filesystem_blockdevice_flash INTERFACE
        ${PICO_VFS_PATH}/include)
    target_link_libraries(pico_filesystem_blockdevice_flash INTERFACE
        hardware_flash
        hardware_sync
        )

    pico_add_library(pico_filesystem_filesystem_littlefs)
    target_sources(pico_filesystem_filesystem_littlefs INTERFACE
        ${PICO_VFS_PATH}/src/filesystem/littlefs.c
        ${PICO_VFS_PATH}/vendor/littlefs/lfs.c
        ${PICO_VFS_PATH}/vendor/littlefs/lfs_util.c
        )
    target_include_directories(pico_filesystem_filesystem_littlefs INTERFACE
        ${PICO_VFS_PATH}/vendor/littlefs)
    target_compile_options(pico_filesystem_filesystem_littlefs INTERFACE -Wno-unused-function -Wno-null-dereference)

    pico_add_library(pico_filesystem_filesystem_fat)
    target_sources(pico_filesystem_filesystem_fat INTERFACE
        ${PICO_VFS_PATH}/src/filesystem/fat.c
        ${PICO_VFS_PATH}/vendor/ff15/source/ff.c
        ${PICO_VFS_PATH}/vendor/ff15/source/ffsystem.c
        ${PICO_VFS_PATH}/vendor/ff15/source/ffunicode.c
        )
    target_include_directories(pico_filesystem_filesystem_fat INTERFACE
        ${PICO_VFS_PATH}
        ${PICO_VFS_PATH}/include/filesystem/ChaN
        ${PICO_VFS_PATH}/vendor/ff15/source
        )

    pico_add_library(pico_filesystem)
    target_sources(pico_filesystem INTERFACE
        ${PICO_VFS_PATH}/src/filesystem/vfs.c)
    target_include_directories(pico_filesystem INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${PICO_VFS_PATH}/include)

    pico_add_library(pico_filesystem_default)
    target_sources(pico_filesystem_default INTERFACE
        ${PICO_VFS_PATH}/src/filesystem/fs_init.c)
    target_include_directories(pico_filesystem_default INTERFACE
        ${PICO_VFS_PATH}/include)
    target_link_libraries(pico_filesystem_default INTERFACE
        pico_filesystem
        pico_filesystem_blockdevice_flash
        pico_filesystem_filesystem_littlefs
        )


    pico_promote_common_scope_vars()

    function(pico_enable_filesystem TARGET)
        set(options "")
        set(oneValueArgs SIZE AUTO_INIT)
        set(multiValueArgs FS_INIT)
        cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

        if(ARG_SIZE)
            target_compile_definitions(${TARGET} PRIVATE PICO_FS_DEFAULT_SIZE=${ARG_SIZE})
        endif()

       if(ARG_FS_INIT)
           target_sources(${TARGET} PRIVATE ${ARG_FS_INIT})
       else()
           target_link_libraries(${TARGET} PRIVATE pico_filesystem_default)
       endif()

       if(ARG_AUTO_INIT)
           target_compile_definitions(${TARGET} PRIVATE PICO_FS_AUTO_INIT=1)
       endif()
    endfunction()

endif()
