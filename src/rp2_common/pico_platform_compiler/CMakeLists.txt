if (NOT TARGET pico_platform_compiler)
    pico_add_library(pico_platform_compiler)

    target_include_directories(pico_platform_compiler_headers SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

    target_link_libraries(pico_platform_compiler_headers INTERFACE hardware_regs)
endif()
