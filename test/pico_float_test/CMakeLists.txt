if (NOT TARGET pico_float)
    message("Skipping pico_float_test as pico_float is unavailable on this platform")
    return()
endif()

PROJECT(pico_float_test)

set(FLOAT_TYPES "")
set(DOUBLE_TYPES "")
foreach (TYPE IN ITEMS compiler pico pico_vfp pico_dcp)
    if (TARGET pico_float_${TYPE})
        list(APPEND FLOAT_TYPES ${TYPE})
    endif()
    if (TARGET pico_double_${TYPE})
        list(APPEND DOUBLE_TYPES ${TYPE})
    endif()
endforeach()

if (PICO_RISCV)
    # Separate, simpler test: currently we only have a few single-precision
    # routines for RISC-V soft float (and the other tests are a bit
    # AEABI-dependent)
    add_executable(pico_float_test
            pico_float_test_hazard3.c
    )
    target_link_libraries(pico_float_test PRIVATE pico_float pico_stdlib)
    target_include_directories(pico_float_test PRIVATE ${CMAKE_CURRENT_LIST_DIR})
    pico_add_extra_outputs(pico_float_test)

    # pico_enable_stdio_usb(pico_float_test 1)
    # pico_enable_stdio_uart(pico_float_test 0)
endif()

foreach (FLOAT_TYPE IN LISTS FLOAT_TYPES)
    if (NOT PICO_RISCV)
        # keep default as pico_float_test for backwards compatibility
        if (FLOAT_TYPE STREQUAL "pico")
            set(PICO_FLOAT_TEST "pico_float_test")
        else()
            set(PICO_FLOAT_TEST "pico_float_test_${FLOAT_TYPE}")
        endif()
        add_executable(${PICO_FLOAT_TEST}
                pico_float_test.c
                llvm/call_apsr.S
        )

        #todo split out variants with different flags
        target_compile_definitions(${PICO_FLOAT_TEST} PRIVATE
                #        PICO_FLOAT_PROPAGATE_NANS=1
                #        PICO_DIVIDER_DISABLE_INTERRUPTS=1
        )

        # handy for testing we aren't pulling in extra stuff
        #target_link_options(${PICO_FLOAT_TEST} PRIVATE -nodefaultlibs)
        target_include_directories(${PICO_FLOAT_TEST} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/llvm)
        target_link_libraries(${PICO_FLOAT_TEST} pico_float pico_stdlib m)
        pico_add_extra_outputs(${PICO_FLOAT_TEST})
        pico_set_float_implementation(${PICO_FLOAT_TEST} ${FLOAT_TYPE})
    endif()

    add_executable(custom_float_funcs_test_${FLOAT_TYPE} custom_float_funcs_test.c)
    pico_set_float_implementation(custom_float_funcs_test_${FLOAT_TYPE} ${FLOAT_TYPE})
    target_link_libraries(custom_float_funcs_test_${FLOAT_TYPE} PRIVATE pico_stdlib)
    pico_add_extra_outputs(custom_float_funcs_test_${FLOAT_TYPE})
    if (NOT PICO_CLIB STREQUAL "llvm_libc")
        # raw compiler printf on llvm_libc doesn't currently have floating point
        pico_set_printf_implementation(custom_float_funcs_test_${FLOAT_TYPE} compiler)
    endif()
    if (PICO_C_COMPILER_IS_GNU)
        target_compile_options(custom_float_funcs_test_${FLOAT_TYPE} PRIVATE -Wno-analyzer-use-of-uninitialized-value)
    endif()
    if (PICO_C_COMPILER_IS_CLANG)
        # llvm by default will treat our out of range float->int conversions as undefined behavior
        # and may just decide the function is too broken to bother even with the function epilogue, so we
        # force it to accept out of range values (since we support them in our versions today)
        target_compile_options(${PICO_FLOAT_TEST} PRIVATE -fno-strict-float-cast-overflow)
        target_compile_options(custom_float_funcs_test_${FLOAT_TYPE} PRIVATE -fno-strict-float-cast-overflow)
    endif()

    add_executable(float_benchmark_${FLOAT_TYPE} float_benchmark.c)
    pico_set_float_implementation(float_benchmark_${FLOAT_TYPE} ${FLOAT_TYPE})
    target_link_libraries(float_benchmark_${FLOAT_TYPE} PRIVATE pico_stdlib m)
    pico_add_extra_outputs(float_benchmark_${FLOAT_TYPE})
    target_compile_definitions(float_benchmark_${FLOAT_TYPE} PRIVATE PICO_FLOAT_IN_RAM=1)
    if (NOT PICO_CLIB STREQUAL "llvm_libc")
        # raw compiler printf on llvm_libc doesn't currently have floating point
        pico_set_printf_implementation(float_benchmark_${FLOAT_TYPE} compiler)
    endif()
    pico_set_binary_type(float_benchmark_${FLOAT_TYPE} copy_to_ram)
endforeach ()

foreach (DOUBLE_TYPE IN LISTS DOUBLE_TYPES)
    if (NOT PICO_RISCV)
        # keep default as pico_float_test for backwards compatibility
        if (DOUBLE_TYPE STREQUAL "pico")
            set(PICO_DOUBLE_TEST "pico_double_test")
        else()
            set(PICO_DOUBLE_TEST "pico_double_test_${DOUBLE_TYPE}")
        endif()

        add_executable(${PICO_DOUBLE_TEST}
                pico_double_test.c
                llvm/call_apsr.S
        )

        target_include_directories(${PICO_DOUBLE_TEST} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/llvm)
        target_link_libraries(${PICO_DOUBLE_TEST} pico_double pico_stdlib m)
        pico_add_extra_outputs(${PICO_DOUBLE_TEST})
        pico_set_double_implementation(${PICO_DOUBLE_TEST} ${DOUBLE_TYPE})
        #todo split out variants with different flags
        target_compile_definitions(${PICO_DOUBLE_TEST} PRIVATE
                PICO_USE_CRT_PRINTF=1 # want full precision output
                PICO_FLOAT_PROPAGATE_NANS=1
                PICO_DOUBLE_PROPAGATE_NANS=1
                #PICO_DIVIDER_DISABLE_INTERRUPTS=1
        )
        if (NOT PICO_CLIB STREQUAL "llvm_libc")
            # raw compiler printf on llvm_libc doesn't currently have floating point
            pico_set_printf_implementation(${PICO_DOUBLE_TEST} compiler) # want full precision output
        endif()

        add_executable(custom_double_funcs_test_${DOUBLE_TYPE} custom_double_funcs_test.c)
        pico_set_double_implementation(custom_double_funcs_test_${DOUBLE_TYPE} ${DOUBLE_TYPE})
        target_link_libraries(custom_double_funcs_test_${DOUBLE_TYPE} PRIVATE pico_stdlib)
        pico_add_extra_outputs(custom_double_funcs_test_${DOUBLE_TYPE})
        if (NOT PICO_CLIB STREQUAL "llvm_libc")
            # raw compiler printf on llvm_libc doesn't currently have floating point
            pico_set_printf_implementation(custom_double_funcs_test_${DOUBLE_TYPE} compiler)
        endif()

        if (PICO_C_COMPILER_IS_CLANG)
            # llvm by default will treat our out of range float->int conversions as undefined behavior
            # and may just decide the function is too broken to bother even with the function epilogue, so we
            # force it to accept out of range values (since we support them in our versions today)
            target_compile_options(${PICO_DOUBLE_TEST} PRIVATE -fno-strict-float-cast-overflow)
            target_compile_options(custom_double_funcs_test_${DOUBLE_TYPE} PRIVATE -fno-strict-float-cast-overflow)
        endif()
    endif()

    add_executable(double_benchmark_${DOUBLE_TYPE} double_benchmark.c)
    pico_set_double_implementation(double_benchmark_${DOUBLE_TYPE} ${DOUBLE_TYPE})
    target_link_libraries(double_benchmark_${DOUBLE_TYPE} PRIVATE pico_stdlib m)
    pico_add_extra_outputs(double_benchmark_${DOUBLE_TYPE})
    target_compile_definitions(double_benchmark_${DOUBLE_TYPE} PRIVATE PICO_DOUBLE_IN_RAM=1)
    if (NOT PICO_CLIB STREQUAL "llvm_libc")
        # raw compiler printf on llvm_libc doesn't currently have floating point
        pico_set_printf_implementation(double_benchmark_${DOUBLE_TYPE} compiler)
    endif()
    pico_set_binary_type(double_benchmark_${DOUBLE_TYPE} copy_to_ram)
endforeach ()

if (PICO_RP2350 AND NOT PICO_RISCV)
    add_executable(m33
            m33.c
    )
    target_compile_definitions(m33 PRIVATE
            PICO_USE_CRT_PRINTF=1 # want full precision output
            PICO_FLOAT_PROPAGATE_NANS=1
            PICO_DOUBLE_PROPAGATE_NANS=1
            #PICO_DIVIDER_DISABLE_INTERRUPTS=1
    )
    pico_set_printf_implementation(m33 compiler) # want full precision output
    pico_set_float_implementation(m33 pico)
    pico_set_double_implementation(m33 pico)
    target_link_libraries(m33 pico_double pico_stdlib)
    pico_add_extra_outputs(m33)
endif()
