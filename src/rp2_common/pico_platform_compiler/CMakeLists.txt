if (NOT TARGET pico_platform_compiler)
    pico_add_library(pico_platform_compiler)

    target_include_directories(pico_platform_compiler_headers SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

    target_link_libraries(pico_platform_compiler_headers INTERFACE hardware_regs)

    if (NOT PICO_HARD_FLOAT_ABI AND PICO_C_COMPILER_IS_GNU AND CMAKE_C_COMPILER_VERSION VERSION_LESS "8.0.0")
        # not defined by GCC < 8 - doing in the build instead of platform/compiler.h as the same
        # problem breaks CMSIS headers
        target_compile_definitions(pico_platform_compiler_headers INTERFACE __SOFTFP__=1)
    endif()
endif()
