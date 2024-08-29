PROJECT(pico_float_test)


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

else ()
    add_executable(pico_float_test
            pico_float_test.c
            llvm/call_apsr.S
            )

    add_executable(pico_double_test
            pico_double_test.c
            llvm/call_apsr.S
            )


    #todo split out variants with different flags
    target_compile_definitions(pico_float_test PRIVATE
    #        PICO_FLOAT_PROPAGATE_NANS=1
    #        PICO_DIVIDER_DISABLE_INTERRUPTS=1
    )

    #todo split out variants with different flags
    target_compile_definitions(pico_double_test PRIVATE
            PICO_USE_CRT_PRINTF=1 # want full precision output
            PICO_FLOAT_PROPAGATE_NANS=1
            PICO_DOUBLE_PROPAGATE_NANS=1
                    #PICO_DIVIDER_DISABLE_INTERRUPTS=1
            )
    if (NOT PICO_CLIB STREQUAL "llvm_libc")
        # raw compiler printf on llvm_libc doesn't currently have floating point
        pico_set_printf_implementation(pico_double_test compiler) # want full precision output
    endif()

    # handy for testing we aren't pulling in extra stuff
    #target_link_options(pico_float_test PRIVATE -nodefaultlibs)

    target_include_directories(pico_float_test PRIVATE ${CMAKE_CURRENT_LIST_DIR}/llvm)
    target_link_libraries(pico_float_test pico_float pico_stdlib)
    pico_add_extra_outputs(pico_float_test)
    #pico_set_float_implementation(pico_float_test compiler)
    #pico_set_float_implementation(pico_float_test pico_vfp)
    #pico_set_double_implementation(pico_float_test compiler)

    target_include_directories(pico_double_test PRIVATE ${CMAKE_CURRENT_LIST_DIR}/llvm)
    target_link_libraries(pico_double_test pico_double pico_stdlib)
    pico_add_extra_outputs(pico_double_test)
    #pico_set_float_implementation(pico_double_test compiler)
    #pico_set_double_implementation(pico_double_test compiler)

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
endif()
