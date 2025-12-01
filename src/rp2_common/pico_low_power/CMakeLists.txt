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
        hardware_uart
        pico_stdio
        )

# pico_set_persistent_data_loc(TARGET PERSISTENT_DATA_LOC)
# \brief\ Set the persistent data location for the target
#
# This sets the target property PICO_TARGET_PERSISTENT_DATA_LOC to the given value,
# and also sets the target property PICO_TARGET_HEAP_LOC and PICO_TARGET_HEAP_LIMIT
# to the appropriate values based on the persistent data location.
#
# It also sets PICO_CRT0_PIN_XIP_SRAM=1 to pin the XIP_SRAM,
# if the persistent data is stored in XIP_SRAM on RP2350.
#
# \param\ PERSISTENT_DATA_LOC The persistent data location to set
function(pico_set_persistent_data_loc TARGET PERSISTENT_DATA_LOC)
        if (NOT PICO_RP2350)
                message(FATAL_ERROR "pico_set_persistent_data_loc is only supported on RP2350")
        endif()

        pico_set_linker_script_var(${TARGET} PERSISTENT_DATA_LOC ${PERSISTENT_DATA_LOC})

        if (PERSISTENT_DATA_LOC LESS 0x20000000)
                # XIP_SRAM, so heap should come after bss
                pico_set_linker_script_var(${TARGET} HEAP_LOC __bss_end__)
                # Also pin the XIP_SRAM
                target_compile_definitions(${TARGET} PRIVATE PICO_CRT0_PIN_XIP_SRAM=1)
        elseif (PERSISTENT_DATA_LOC LESS 0x20040000)
                # SRAM0, so heap should come after persistent data
                pico_set_linker_script_var(${TARGET} HEAP_LOC __persistent_data_end__)
        elseif(PERSISTENT_DATA_LOC LESS 0x20080000)
                # SRAM1, so heap should come before persistent data
                pico_set_linker_script_var(${TARGET} HEAP_LOC __bss_end__)
                pico_set_linker_script_var(${TARGET} HEAP_LIMIT ${PERSISTENT_DATA_LOC})
        else()
                # Not supported in scratch, as the linker script will overwrite persistent data with scratch data
                message(FATAL_ERROR "pico_set_persistent_data_loc only supports persistent data in XIP_SRAM or SRAM0-7")
        endif()
endfunction()
