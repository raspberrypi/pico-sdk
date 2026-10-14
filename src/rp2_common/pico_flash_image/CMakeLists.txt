pico_add_library(pico_flash_image)

target_sources(pico_flash_image INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/flash_image.c
        ${CMAKE_CURRENT_LIST_DIR}/flashlog.c
)

target_include_directories(pico_flash_image SYSTEM INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

pico_generate_pio_header(pico_flash_image ${CMAKE_CURRENT_LIST_DIR}/debug_uart_tx.pio)

target_link_libraries(pico_flash_image INTERFACE
    boot_picobin_headers
    boot_picoboot_headers
    boot_uf2_headers
    hardware_flash
    pico_bootrom
    pico_stdlib
)
