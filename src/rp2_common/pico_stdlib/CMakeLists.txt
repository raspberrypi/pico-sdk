if (NOT TARGET pico_stdlib)
    pico_add_impl_library(pico_stdlib)
    target_sources(pico_stdlib INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/stdlib.c
    )

    set(PICO_STDLIB_LIBRARIES
            hardware_gpio
            hardware_uart
            hardware_divider
            pico_time
            pico_util
            pico_platform
            pico_runtime
            pico_stdio
    )
    foreach(LIB IN LISTS PICO_STDLIB_LIBRARIES)
        if (TARGET ${LIB})
            pico_mirrored_target_link_libraries(pico_stdlib INTERFACE ${LIB})
        endif()
    endforeach()
endif()
