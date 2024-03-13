if (NOT TARGET pico_stdio)
    # library to be depended on - we make this depend on particular implementations using per target generator expressions
    pico_add_library(pico_stdio)

    # no custom implementation; falls thru to compiler
    pico_add_library(pico_stdio_compiler)
    target_sources(pico_stdio_compiler INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/stdio.c
    )

    target_include_directories(pico_stdio_headers INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

    # add alias "default" which is just pico
    add_library(pico_stdio_default INTERFACE)
    target_link_libraries(pico_stdio_default INTERFACE pico_stdio_pico)

    set(PICO_DEFAULT_STDIO_IMPL pico_stdio_default)

    target_link_libraries(pico_stdio INTERFACE
            $<IF:$<BOOL:$<TARGET_PROPERTY:PICO_TARGET_STDIO_IMPL>>,$<TARGET_PROPERTY:PICO_TARGET_STDIO_IMPL>,${PICO_DEFAULT_STDIO_IMPL}>)

    pico_add_library(pico_stdio_pico)
    target_sources(pico_stdio_pico INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/stdio.c
    )
    target_link_libraries(pico_stdio_pico INTERFACE pico_stdio_headers)

    function(wrap_stdio_functions TARGET)
        pico_wrap_function(${TARGET} printf)
        pico_wrap_function(${TARGET} vprintf)
        pico_wrap_function(${TARGET} puts)
        pico_wrap_function(${TARGET} putchar)
        pico_wrap_function(${TARGET} getchar)
    endfunction()

    wrap_stdio_functions(pico_stdio_pico)

    macro(pico_set_stdio_implementation TARGET IMPL)
        get_target_property(target_type ${TARGET} TYPE)
        if ("EXECUTABLE" STREQUAL "${target_type}")
            set_target_properties(${TARGET} PROPERTIES PICO_TARGET_STDIO_IMPL "pico_stdio_${IMPL}")
        else()
            message(FATAL_ERROR "stdio implementation must be set on executable not library")
        endif()
    endmacro()

    if (TARGET pico_printf)
        pico_mirrored_target_link_libraries(pico_stdio INTERFACE pico_printf)
    endif()
endif()
