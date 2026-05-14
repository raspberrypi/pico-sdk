if (NOT TARGET pico_clib_interface)
    pico_add_library(pico_clib_interface)

    # ---- newlib ----
    pico_add_library(pico_newlib_interface)

    target_sources(pico_newlib_interface INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/newlib_interface.c
    )

    # ---- picolibc ----

    pico_add_library(pico_picolibc_interface)

    target_sources(pico_picolibc_interface INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/picolibc_interface.c
    )

    # replacing the functions is fine it seems
    #target_compile_definitions(pico_picolibc_interface INTERFACE
    #            PICO_STDIO_SHORT_CIRCUIT_CLIB_FUNCS=0
    #)

    target_include_directories(pico_picolibc_interface SYSTEM INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/include/picolibc
    )
    
    #  ---- llvm_libc ----
    pico_add_library(pico_llvm_libc_interface)

    target_sources(pico_llvm_libc_interface INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/llvm_libc_interface.c
    )

    target_include_directories(pico_llvm_libc_interface SYSTEM INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/include/llvm_libc
    )
    target_link_libraries(pico_llvm_libc_interface INTERFACE pico_atomic)
    pico_mirrored_target_link_libraries(pico_llvm_libc_interface INTERFACE pico_stdio)

    if (NOT PICO_CLIB)
        # if no PICO_CLIB specified check for gcc existance of either newlib or picolibc 
        if(PICO_COMPILER_CC MATCHES "gcc$")
            execute_process(
                COMMAND ${PICO_COMPILER_CC} -print-file-name=nano.specs
                OUTPUT_VARIABLE PICOLIBC_SPECS_PATH
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
    
            # nano.specs exists if gcc/clang returns a complete path -> newlib exists
            if(EXISTS "${PICOLIBC_SPECS_PATH}")
                set(PICO_CLIB "newlib")
            else()
                # otherwise give picolibc a try
                set(PICO_CLIB "picolibc")
            endif()
        endif()
    endif()    
    
    target_link_libraries(pico_clib_interface INTERFACE pico_${PICO_CLIB}_interface)
endif()
