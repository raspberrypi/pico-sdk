if (NOT TARGET pico_secure)
    message("Skipping pico_secure_test as pico_secure is unavailable on this platform")
    return()
endif()

# Common partition table UF2
pico_create_partition_table_uf2(pico_secure_test_pt ${CMAKE_CURRENT_LIST_DIR}/pt.json)

# Common partition erase UF2, to erase the other partitions and ensure a clean start
pico_create_erase_partition_uf2(pico_secure_test_erase)

# Add secure executable
add_executable(pico_secure_test_s pico_secure_test_s.c)
target_link_libraries(pico_secure_test_s
        pico_stdlib
        hardware_watchdog
        pico_secure
        pico_test
        hardware_flash
)

# Skip in CI, as requires UF2 loading
set_target_properties(pico_secure_test_s PROPERTIES PICO_TEST_SKIP_IN_CI TRUE)

# Packaged no_flash binary
pico_set_binary_type(pico_secure_test_s no_flash)
pico_package_uf2_output(pico_secure_test_s 0x10000000)
pico_add_extra_outputs(pico_secure_test_s)

# Add non-secure executable
add_executable(pico_secure_test_ns pico_secure_test_ns.c)
target_link_libraries(pico_secure_test_ns
        pico_stdlib
        pico_secure
        pico_test
        hardware_flash
        hardware_pio
)

pico_generate_pio_header(pico_secure_test_ns ${CMAKE_CURRENT_LIST_DIR}/addition.pio)

# Skip in CI, as requires UF2 loading
set_target_properties(pico_secure_test_ns PROPERTIES PICO_TEST_SKIP_IN_CI TRUE)

# Set UF2 family manually
pico_set_uf2_family(pico_secure_test_ns "rp2350-arm-ns")

# Use stdio_usb
pico_enable_stdio_uart(pico_secure_test_ns 0)
pico_enable_stdio_usb(pico_secure_test_ns 1)

# Default flash binary
pico_add_extra_outputs(pico_secure_test_ns)

# Set security options
pico_set_security_options(pico_secure_test_s pico_secure_test_ns
    STDIO RAND DMA USER_IRQ PIO GPIO USB
    NONSECURE_TIMER 1
)

# Set security ram split
pico_set_security_ram_split(pico_secure_test_s pico_secure_test_ns SIMPLE 64k 2k)

# Create concatenated UF2, for loading into the partition table using drag & drop (non-secure first)
pico_concatenate_uf2_outputs(pico_secure_test_drag_drop pico_secure_test_ns pico_secure_test_s)

# Create combined UF2 containing partition table, for loading using picotool or drag & drop
pico_combine_uf2_outputs(pico_secure_test_combined pico_secure_test_pt
        pico_secure_test_erase pico_secure_test_s pico_secure_test_erase pico_secure_test_ns
        PARTITIONS 0 1 2 3)

# Create combined CMake target, for easy building
add_custom_target(pico_secure_test ALL
        DEPENDS pico_secure_test_combined pico_secure_test_drag_drop)
