# PICO_CMAKE_CONFIG: PICO_STDIO_UART, Option to globally enable stdio UART for all targets by default, type=bool, default=1, group=pico_stdlib
option(PICO_STDIO_UART "Globally enable stdio UART" 1)
# PICO_CMAKE_CONFIG: PICO_STDIO_USB, Option to globally enable stdio USB for all targets by default, type=bool, default=0, group=pico_stdlib
option(PICO_STDIO_USB "Globally enable stdio USB" 0)
# PICO_CMAKE_CONFIG: PICO_STDIO_SEMIHOSTING, Option to globally enable stdio semi-hosting for all targets by default, type=bool, default=0, group=pico_stdlib
option(PICO_STDIO_SEMIHOSTING "Globally enable stdio semi-hosting" 0)
# PICO_CMAKE_CONFIG: PICO_STDIO_RTT, Option to globally enable stdio RTT for all targets by default, type=bool, default=0, group=pico_stdlib
option(PICO_STDIO_RTT "Globally enable stdio RTT" 0)

if (NOT TARGET pico_stdio)
    pico_add_library(pico_stdio)

    target_include_directories(pico_stdio_headers SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

    target_sources(pico_stdio INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/stdio.c
    )

    pico_wrap_function(pico_stdio printf) # here not pico_printf as we do mutex
    pico_wrap_function(pico_stdio vprintf) # here not pico_printf as we do mutex
    pico_wrap_function(pico_stdio puts)
    pico_wrap_function(pico_stdio putchar)
    pico_wrap_function(pico_stdio getchar)

    if (TARGET pico_printf)
        pico_mirrored_target_link_libraries(pico_stdio INTERFACE pico_printf)
    endif()

    function(pico_enable_stdio_uart TARGET ENABLED)
        set_target_properties(${TARGET} PROPERTIES PICO_TARGET_STDIO_UART ${ENABLED})
    endfunction()

    function(pico_enable_stdio_usb TARGET ENABLED)
        set_target_properties(${TARGET} PROPERTIES PICO_TARGET_STDIO_USB ${ENABLED})
    endfunction()

    function(pico_enable_stdio_semihosting TARGET ENABLED)
        set_target_properties(${TARGET} PROPERTIES PICO_TARGET_STDIO_SEMIHOSTING ${ENABLED})
    endfunction()

    function(pico_enable_stdio_rtt TARGET ENABLED)
        set_target_properties(${TARGET} PROPERTIES PICO_TARGET_STDIO_RTT ${ENABLED})
    endfunction()

    if (TARGET pico_stdio_uart)
        target_link_libraries(pico_stdio INTERFACE $<IF:$<BOOL:$<IF:$<STREQUAL:$<TARGET_PROPERTY:PICO_TARGET_STDIO_UART>,>,${PICO_STDIO_UART},$<TARGET_PROPERTY:PICO_TARGET_STDIO_UART>>>,pico_stdio_uart,>)
        target_link_libraries(pico_stdio_headers INTERFACE $<IF:$<BOOL:$<IF:$<STREQUAL:$<TARGET_PROPERTY:PICO_TARGET_STDIO_UART>,>,${PICO_STDIO_UART},$<TARGET_PROPERTY:PICO_TARGET_STDIO_UART>>>,pico_stdio_uart_headers,>)
    endif()

    if (TARGET pico_stdio_usb)
        target_link_libraries(pico_stdio INTERFACE $<IF:$<BOOL:$<IF:$<STREQUAL:$<TARGET_PROPERTY:PICO_TARGET_STDIO_USB>,>,${PICO_STDIO_USB},$<TARGET_PROPERTY:PICO_TARGET_STDIO_USB>>>,pico_stdio_usb,>)
        target_link_libraries(pico_stdio_headers INTERFACE $<IF:$<BOOL:$<IF:$<STREQUAL:$<TARGET_PROPERTY:PICO_TARGET_STDIO_USB>,>,${PICO_STDIO_USB},$<TARGET_PROPERTY:PICO_TARGET_STDIO_USB>>>,pico_stdio_usb_headers,>)
    endif()

    if (TARGET pico_stdio_semihosting)
        target_link_libraries(pico_stdio INTERFACE $<IF:$<BOOL:$<IF:$<STREQUAL:$<TARGET_PROPERTY:PICO_TARGET_STDIO_SEMIHOSTING>,>,${PICO_STDIO_SEMIHOSTING},$<TARGET_PROPERTY:PICO_TARGET_STDIO_SEMIHOSTING>>>,pico_stdio_semihosting,>)
        target_link_libraries(pico_stdio_headers INTERFACE $<IF:$<BOOL:$<IF:$<STREQUAL:$<TARGET_PROPERTY:PICO_TARGET_STDIO_SEMIHOSTING>,>,${PICO_STDIO_SEMIHOSTING},$<TARGET_PROPERTY:PICO_TARGET_STDIO_SEMIHOSTING>>>,pico_stdio_semihosting_headers,>)
    endif()

    if (TARGET pico_stdio_rtt)
        target_link_libraries(pico_stdio INTERFACE $<IF:$<BOOL:$<IF:$<STREQUAL:$<TARGET_PROPERTY:PICO_TARGET_STDIO_RTT>,>,${PICO_STDIO_RTT},$<TARGET_PROPERTY:PICO_TARGET_STDIO_RTT>>>,pico_stdio_rtt,>)
        target_link_libraries(pico_stdio_headers INTERFACE $<IF:$<BOOL:$<IF:$<STREQUAL:$<TARGET_PROPERTY:PICO_TARGET_STDIO_RTT>,>,${PICO_STDIO_RTT},$<TARGET_PROPERTY:PICO_TARGET_STDIO_RTT>>>,pico_stdio_rtt_headers,>)
    endif()

endif()