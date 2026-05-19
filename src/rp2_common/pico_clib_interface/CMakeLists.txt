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
        # PICO_CMAKE_CONFIG: PICO_CLIB, The C library to use e.g. newlib/picolibc/llvm_libc, type=string, default=based on PICO_COMPILER, group=build, docref=cmake-toolchain-config
        set(CLIB_DETECT_CODE "
            #include <stdio.h>
            #ifdef __PICOLIBC__
            #define USING_PICOLIBC 1
            #elif defined(__NEWLIB__)
            #define USING_NEWLIB 1
            #endif
            
            #if (picolibc && !USING_PICOLIBC) || (newlib && !USING_NEWLIB)
            #error not using wanted c library
            #endif
        ")

        # Create a temporary file for the source
        set(CLIB_DETECT_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/clib_check.c")
        file(WRITE "${CLIB_DETECT_SOURCE}" "${CLIB_DETECT_CODE}")

        set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
        foreach(CLIB newlib picolibc)
            # Attempt to compile the source file
            try_compile(CLIB_MATCHED
                    "${CMAKE_CURRENT_BINARY_DIR}"              # Directory for temporary build files
                    "${CLIB_DETECT_SOURCE}"                    # Source file to compile
                    COMPILE_DEFINITIONS "-D${CLIB}=1"
            )
            if (CLIB_MATCHED)
                set(PICO_CLIB ${CLIB})
                break()
            endif()
        endforeach ()
        if (NOT PICO_CLIB)
            message("C library not detected; defaulting to newlib")
            set(PICO_CLIB newlib)
        endif()
        file(REMOVE "${CLIB_DETECT_SOURCE}")
        set(PICO_CLIB "${PICO_CLIB}" CACHE INTERNAL "")
    endif()
    message("C library type is ${PICO_CLIB}")
    target_link_libraries(pico_clib_interface INTERFACE pico_${PICO_CLIB}_interface)
endif()
