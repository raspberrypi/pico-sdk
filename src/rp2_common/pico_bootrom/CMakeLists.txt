pico_add_library(pico_bootrom_headers NOFLAG)

target_include_directories(pico_bootrom_headers SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

add_library(pico_bootrom INTERFACE)
target_sources(pico_bootrom INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/bootrom.c
        ${CMAKE_CURRENT_LIST_DIR}/bootrom_lock.c
        )

target_link_libraries(pico_bootrom_headers INTERFACE boot_picoboot_headers boot_bootrom_headers)
pico_mirrored_target_link_libraries(pico_bootrom INTERFACE pico_base hardware_boot_lock pico_flash)

# bootrom.c includes boot/picobin.h
# bootrom_lock.c includes pico/runtime_init.h
target_link_libraries(pico_bootrom INTERFACE boot_picobin_headers pico_runtime_init_headers)