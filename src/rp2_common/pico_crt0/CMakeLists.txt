if (NOT TARGET pico_crt0)
    pico_add_library(pico_crt0)

    if (PICO_RISCV)
        target_sources(pico_crt0 INTERFACE ${CMAKE_CURRENT_LIST_DIR}/crt0_riscv.S)
    else()
        target_sources(pico_crt0 INTERFACE ${CMAKE_CURRENT_LIST_DIR}/crt0.S)
    endif()

    pico_mirrored_target_link_libraries(pico_crt0 INTERFACE hardware_structs hardware_regs)
    target_link_libraries(pico_crt0 INTERFACE boot_picobin_headers pico_bootrom_headers)
endif()

pico_register_common_scope_var(PICO_LINKER_SCRIPT_PATH)
if (NOT PICO_LINKER_SCRIPT_PATH)
    set(PICO_LINKER_SCRIPT_PATH ${CMAKE_CURRENT_LIST_DIR}/${PICO_CHIP})
endif()

pico_promote_common_scope_vars()