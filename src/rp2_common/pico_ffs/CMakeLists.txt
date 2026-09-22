pico_add_library(pico_ffs)

target_sources(pico_ffs INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/ffs.c
        ${CMAKE_CURRENT_LIST_DIR}/ffs_internal.c
)

target_include_directories(pico_ffs SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

target_link_libraries(pico_ffs INTERFACE boot_picobin_headers hardware_flash pico_bootrom pico_stdlib)
