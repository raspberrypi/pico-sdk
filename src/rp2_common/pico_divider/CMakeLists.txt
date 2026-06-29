if (NOT TARGET pico_divider)
    # library to be depended on - we make this depend on particular implementations using per target generator expressions
    pico_add_impl_library(pico_divider)

    # no custom implementation; falls thru to compiler
    pico_add_library(pico_divider_compiler)
    target_sources(pico_divider_compiler INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/divider_compiler.c
            )
    target_link_libraries(pico_divider_compiler_headers INTERFACE pico_divider_headers)

    # add alias "pico" which is hardware for RP2040 and compiler for RP2350
    add_library(pico_divider_pico INTERFACE)
    if (PICO_RP2040)
        target_link_libraries(pico_divider_pico INTERFACE pico_divider_hardware)
    else()
        target_link_libraries(pico_divider_pico INTERFACE pico_divider_compiler)
    endif()
    add_library(pico_divider_default INTERFACE)
    target_link_libraries(pico_divider_default INTERFACE pico_divider_pico)

    # PICO_CMAKE_CONFIG: PICO_DEFAULT_DIVIDER_IMPL, The default implementation for pico_divider to link. 'pico' selects the fastest implementation for the current platform; 'hardware' selects the hardware divider on RP2040; 'compiler' uses the default compiler/C library versions, type=string, default=pico, group=pico_divider
    pico_fixup_default_impl(PICO_DEFAULT_DIVIDER_IMPL pico_divider default)

    target_link_libraries(pico_divider INTERFACE
            $<IF:$<BOOL:$<TARGET_PROPERTY:PICO_TARGET_DIVIDER_IMPL>>,$<TARGET_PROPERTY:PICO_TARGET_DIVIDER_IMPL>,${PICO_DEFAULT_DIVIDER_IMPL}>)

    pico_add_library(pico_divider_hardware_explicit NOFLAG)
    target_sources(pico_divider_hardware_explicit INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/divider_hardware.S
            )

    target_link_libraries(pico_divider_hardware_explicit_headers INTERFACE
            pico_divider_headers
            hardware_divider_headers
            hardware_regs
            )

    pico_add_library(pico_divider_hardware)

    target_link_libraries(pico_divider_hardware INTERFACE pico_divider_hardware_explicit)

    pico_wrap_function(pico_divider_hardware __aeabi_idiv)
    pico_wrap_function(pico_divider_hardware __aeabi_idivmod)
    pico_wrap_function(pico_divider_hardware __aeabi_ldivmod)
    pico_wrap_function(pico_divider_hardware __aeabi_uidiv)
    pico_wrap_function(pico_divider_hardware __aeabi_uidivmod)
    pico_wrap_function(pico_divider_hardware __aeabi_uldivmod)

    macro(pico_set_divider_implementation TARGET IMPL)
        get_target_property(target_type ${TARGET} TYPE)
        if ("EXECUTABLE" STREQUAL "${target_type}")
            set_target_properties(${TARGET} PROPERTIES PICO_TARGET_DIVIDER_IMPL "pico_divider_${IMPL}")
        else()
            message(FATAL_ERROR "divider implementation must be set on executable not library")
        endif()
    endmacro()
endif()
