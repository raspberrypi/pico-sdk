if (NOT TARGET pico_platform)
    pico_add_library(pico_platform)

    target_sources(pico_platform INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/platform.c
    )

    target_compile_definitions(pico_platform_headers INTERFACE
            PICO_NO_HARDWARE=0
            PICO_ON_DEVICE=1
            PICO_BUILD=1
            PICO_RP2350=1
            PICO_32BIT=1
    )

    if (PICO_RISCV)
        target_compile_definitions(pico_platform_headers INTERFACE PICO_RISCV=1)
    endif()

    target_include_directories(pico_platform_headers SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

    target_link_libraries(pico_platform_headers INTERFACE
            hardware_regs
    )
    pico_mirrored_target_link_libraries(pico_platform INTERFACE
            pico_platform_compiler
            pico_platform_panic
            pico_platform_sections
            hardware_dcp
            hardware_rcp
    )
    if (PICO_RISCV)
        pico_mirrored_target_link_libraries(pico_platform INTERFACE hardware_hazard3)
    endif()
endif()

function(pico_add_platform_library TARGET)
    target_link_libraries(pico_platform INTERFACE ${TARGET})
endfunction()