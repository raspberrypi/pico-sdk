# Use XIP SRAM for time critical functions
add_executable(pico_critical_xip_sram_test pico_critical_xip_sram_test.c)
target_link_libraries(pico_critical_xip_sram_test PRIVATE pico_stdlib pico_multicore hardware_dma pico_test)
if (NOT PICO_RP2040)
    target_link_libraries(pico_critical_xip_sram_test PRIVATE hardware_riscv_platform_timer)
endif()
pico_set_binary_type(pico_critical_xip_sram_test copy_to_ram)
pico_use_xip_sram_for_time_critical(pico_critical_xip_sram_test)
pico_enable_stdio_usb(pico_critical_xip_sram_test 1) # test with USB as that invokes flash functions
pico_add_extra_outputs(pico_critical_xip_sram_test)

# Use XIP SRAM for time critical functions (no_flash build)
add_executable(pico_critical_xip_sram_test_no_flash pico_critical_xip_sram_test.c)
target_link_libraries(pico_critical_xip_sram_test_no_flash PRIVATE pico_stdlib pico_multicore hardware_dma pico_test)
if (NOT PICO_RP2040)
    target_link_libraries(pico_critical_xip_sram_test_no_flash PRIVATE hardware_riscv_platform_timer)
endif()
pico_set_binary_type(pico_critical_xip_sram_test_no_flash no_flash)
pico_use_xip_sram_for_time_critical(pico_critical_xip_sram_test_no_flash)
pico_add_extra_outputs(pico_critical_xip_sram_test_no_flash)

if (NOT PICO_RP2040 AND NOT PICO_C_COMPILER_IS_CLANG)
    # XIP SRAM only binary, clang version is too big
    add_executable(pico_xip_sram_test pico_xip_sram_test.c)
    target_link_libraries(pico_xip_sram_test PRIVATE pico_stdlib)
    pico_set_binary_type(pico_xip_sram_test xip_ram)
    pico_minimize_runtime(pico_xip_sram_test INCLUDE PRINTF PRINTF_MINIMAL DEFAULT_ALARM_POOL PANIC FLOAT)
    target_compile_definitions(pico_xip_sram_test PRIVATE PICO_HEAP_SIZE=0x200)
    pico_add_extra_outputs(pico_xip_sram_test)

    # Low power and critical XIP SRAM
    add_executable(pico_critical_xip_sram_test_low_power pico_critical_xip_sram_test.c)
    target_link_libraries(pico_critical_xip_sram_test_low_power PRIVATE pico_stdlib pico_multicore hardware_dma pico_test hardware_riscv_platform_timer pico_low_power)
    pico_set_binary_type(pico_critical_xip_sram_test_low_power copy_to_ram)
    pico_use_xip_sram_for_time_critical(pico_critical_xip_sram_test_low_power)
    pico_set_persistent_data_loc(pico_critical_xip_sram_test_low_power xip_ram)
    pico_add_extra_outputs(pico_critical_xip_sram_test_low_power)

    # Packaged binary and critical XIP SRAM
    add_executable(pico_critical_xip_sram_test_packaged pico_critical_xip_sram_test.c)
    target_link_libraries(pico_critical_xip_sram_test_packaged PRIVATE pico_stdlib pico_multicore hardware_dma pico_test)
    if (NOT PICO_RP2040)
        target_link_libraries(pico_critical_xip_sram_test_packaged PRIVATE hardware_riscv_platform_timer)
    endif()
    pico_set_binary_type(pico_critical_xip_sram_test_packaged no_flash)
    pico_use_xip_sram_for_time_critical(pico_critical_xip_sram_test_packaged)
    pico_package_uf2_output(pico_critical_xip_sram_test_packaged)
    pico_add_extra_outputs(pico_critical_xip_sram_test_packaged)
endif()