# Use XIP SRAM for time critical functions
add_executable(pico_critical_xip_sram_test pico_critical_xip_sram_test.c)
target_link_libraries(pico_critical_xip_sram_test PRIVATE pico_stdlib pico_multicore hardware_dma pico_test)
if (NOT PICO_RP2040)
    target_link_libraries(pico_critical_xip_sram_test PRIVATE hardware_riscv_platform_timer)
endif()
pico_set_binary_type(pico_critical_xip_sram_test copy_to_ram)
pico_set_time_critical_placement(pico_critical_xip_sram_test xip_ram)
pico_enable_stdio_usb(pico_critical_xip_sram_test 1) # test with USB as that invokes flash functions
pico_add_extra_outputs(pico_critical_xip_sram_test)

# Use Scratch X for time critical functions (should still be fast, as core1 function doesn't use stack)
add_executable(pico_critical_scratch_x_test pico_critical_xip_sram_test.c)
target_link_libraries(pico_critical_scratch_x_test PRIVATE pico_stdlib pico_multicore hardware_dma pico_test)
if (NOT PICO_RP2040)
    target_link_libraries(pico_critical_scratch_x_test PRIVATE hardware_riscv_platform_timer)
    target_compile_definitions(pico_critical_scratch_x_test PRIVATE
        TEXT_FUNC_XIP_BASE=SRAM_SCRATCH_X_BASE
        TEXT_FUNC_XIP_END=SRAM_SCRATCH_Y_BASE
    )
else()
    target_compile_definitions(pico_critical_scratch_x_test PRIVATE
        TEXT_FUNC_XIP_BASE=SRAM4_BASE
        TEXT_FUNC_XIP_END=SRAM5_BASE
    )
endif()
pico_set_binary_type(pico_critical_scratch_x_test copy_to_ram)
pico_set_time_critical_placement(pico_critical_scratch_x_test scratch_x)
pico_add_extra_outputs(pico_critical_scratch_x_test)

# Use Scratch Y for time critical functions (should still be fast, as this is only used by core0 stack)
add_executable(pico_critical_scratch_y_test pico_critical_xip_sram_test.c)
target_link_libraries(pico_critical_scratch_y_test PRIVATE pico_stdlib pico_multicore hardware_dma pico_test)
if (NOT PICO_RP2040)
    target_link_libraries(pico_critical_scratch_y_test PRIVATE hardware_riscv_platform_timer)
    target_compile_definitions(pico_critical_scratch_y_test PRIVATE
        TEXT_FUNC_XIP_BASE=SRAM_SCRATCH_Y_BASE
        TEXT_FUNC_XIP_END=SRAM_END
    )
else()
    target_compile_definitions(pico_critical_scratch_y_test PRIVATE
        TEXT_FUNC_XIP_BASE=SRAM5_BASE
        TEXT_FUNC_XIP_END=SRAM_END
    )
endif()
pico_set_binary_type(pico_critical_scratch_y_test copy_to_ram)
pico_set_time_critical_placement(pico_critical_scratch_y_test scratch_y)
pico_add_extra_outputs(pico_critical_scratch_y_test)

# Use XIP SRAM for time critical functions and scratch X for not in flash functions
add_executable(pico_critical_xip_sram_no_flash_scratch_x_test pico_critical_xip_sram_test.c)
target_link_libraries(pico_critical_xip_sram_no_flash_scratch_x_test PRIVATE pico_stdlib pico_multicore hardware_dma pico_test)
if (NOT PICO_RP2040)
    target_link_libraries(pico_critical_xip_sram_no_flash_scratch_x_test PRIVATE hardware_riscv_platform_timer)
    target_compile_definitions(pico_critical_xip_sram_no_flash_scratch_x_test PRIVATE
        TEXT_FUNC_SRAM_BASE=SRAM_SCRATCH_X_BASE
        TEXT_FUNC_SRAM_END=SRAM_SCRATCH_Y_BASE
    )
else()
    target_compile_definitions(pico_critical_xip_sram_no_flash_scratch_x_test PRIVATE
        TEXT_FUNC_SRAM_BASE=SRAM4_BASE
        TEXT_FUNC_SRAM_END=SRAM5_BASE
    )
endif()
target_compile_definitions(pico_critical_xip_sram_no_flash_scratch_x_test PRIVATE
    PICO_CORE1_STACK_SIZE=0x200
)
pico_set_binary_type(pico_critical_xip_sram_no_flash_scratch_x_test copy_to_ram)
pico_set_not_in_flash_placement(pico_critical_xip_sram_no_flash_scratch_x_test scratch_x)
pico_set_time_critical_placement(pico_critical_xip_sram_no_flash_scratch_x_test xip_ram)
pico_enable_stdio_usb(pico_critical_xip_sram_no_flash_scratch_x_test 1) # test with USB as that invokes flash functions
pico_add_extra_outputs(pico_critical_xip_sram_no_flash_scratch_x_test)

# Use XIP SRAM for time critical functions (no_flash build)
add_executable(pico_critical_xip_sram_test_no_flash pico_critical_xip_sram_test.c)
target_link_libraries(pico_critical_xip_sram_test_no_flash PRIVATE pico_stdlib pico_multicore hardware_dma pico_test)
if (NOT PICO_RP2040)
    target_link_libraries(pico_critical_xip_sram_test_no_flash PRIVATE hardware_riscv_platform_timer)
endif()
pico_set_binary_type(pico_critical_xip_sram_test_no_flash no_flash)
pico_set_time_critical_placement(pico_critical_xip_sram_test_no_flash xip_ram)
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
    pico_set_time_critical_placement(pico_critical_xip_sram_test_low_power xip_ram)
    pico_set_persistent_data_loc(pico_critical_xip_sram_test_low_power xip_ram)
    pico_add_extra_outputs(pico_critical_xip_sram_test_low_power)
endif()