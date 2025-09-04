pico_add_library(pico_low_power)

target_sources(pico_low_power INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/low_power.c
)

target_include_directories(pico_low_power INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/include
)

pico_mirrored_target_link_libraries(pico_low_power INTERFACE
        hardware_clocks
        hardware_rosc
        hardware_xosc
        hardware_irq
        hardware_timer
        pico_aon_timer
        )

function(pico_set_persistent_data_loc TARGET PERSISTENT_DATA_LOC)
        set_target_properties(${TARGET} PROPERTIES PICO_TARGET_PERSISTENT_DATA_LOC ${PERSISTENT_DATA_LOC})

        if (PERSISTENT_DATA_LOC LESS 0x20000000)
                # XIP_SRAM, so heap should come after bss
                set_target_properties(${TARGET} PROPERTIES
                        PICO_TARGET_HEAP_LOC __bss_end__)
                # Also pin the XIP_SRAM
                target_compile_definitions(${TARGET} PRIVATE PICO_CRT0_PIN_XIP_SRAM=1)
        elseif (PERSISTENT_DATA_LOC LESS 0x20040000)
                # SRAM0, so heap should come after persistent data
                set_target_properties(${TARGET} PROPERTIES
                        PICO_TARGET_HEAP_LOC __persistent_data_end__)
        elseif(PERSISTENT_DATA_LOC LESS 0x20080000)
                # SRAM1, so heap should come before persistent data
                set_target_properties(${TARGET} PROPERTIES
                        PICO_TARGET_HEAP_LOC __bss_end__
                        PICO_TARGET_HEAP_LIMIT ${PERSISTENT_DATA_LOC})
        else()
                # Not supported in scratch, as the linker script will overwrite persistent data with scratch data
                message(FATAL_ERROR "pico_set_persistent_data_loc only supports persistent data in XIP_SRAM or SRAM0-7")
        endif()

        pico_add_linker_constants(${TARGET})
endfunction()