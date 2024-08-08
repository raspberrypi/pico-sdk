pico_simple_hardware_target(exception)
pico_mirrored_target_link_libraries(hardware_exception INTERFACE hardware_sync)

if (PICO_RISCV)
    target_compile_definitions(hardware_exception INTERFACE PICO_CRT0_NO_ISR_RISCV_MACHINE_EXCEPTION=1)
    target_sources(hardware_exception INTERFACE
                exception_table_riscv.S
    )
endif()