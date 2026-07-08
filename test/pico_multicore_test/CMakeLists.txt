if (TARGET pico_multicore AND NOT PICO_TIME_NO_ALARM_SUPPORT)
    add_executable(pico_multicore_test pico_multicore_test.c)
    target_link_libraries(pico_multicore_test PRIVATE pico_stdlib pico_test pico_multicore)
    pico_add_extra_outputs(pico_multicore_test)

    add_executable(pico_multicore_vtable_test pico_multicore_test.c)
    target_link_libraries(pico_multicore_vtable_test PRIVATE pico_stdlib pico_test pico_multicore)
    target_compile_definitions(pico_multicore_vtable_test PRIVATE PICO_VTABLE_PER_CORE=1)
    pico_add_extra_outputs(pico_multicore_vtable_test)

    add_executable(pico_multicore_vtable_test_copy_to_ram pico_multicore_test.c)
    target_link_libraries(pico_multicore_vtable_test_copy_to_ram PRIVATE pico_stdlib pico_test pico_multicore)
    target_compile_definitions(pico_multicore_vtable_test_copy_to_ram PRIVATE PICO_VTABLE_PER_CORE=1)
    pico_set_binary_type(pico_multicore_vtable_test_copy_to_ram copy_to_ram)
    pico_add_extra_outputs(pico_multicore_vtable_test_copy_to_ram)

    add_executable(pico_multicore_vtable_test_no_flash pico_multicore_test.c)
    target_link_libraries(pico_multicore_vtable_test_no_flash PRIVATE pico_stdlib pico_test pico_multicore)
    target_compile_definitions(pico_multicore_vtable_test_no_flash PRIVATE PICO_VTABLE_PER_CORE=1)
    pico_set_binary_type(pico_multicore_vtable_test_no_flash no_flash)
    if (NOT PICO_RP2040)
        pico_package_uf2_output(pico_multicore_vtable_test_no_flash)
    endif()
    pico_add_extra_outputs(pico_multicore_vtable_test_no_flash)

    add_executable(pico_multicore_vtable_scratch_test pico_multicore_test.c)
    target_link_libraries(pico_multicore_vtable_scratch_test PRIVATE pico_stdlib pico_test pico_multicore)
    target_compile_definitions(pico_multicore_vtable_scratch_test PRIVATE
        PICO_VTABLE_PER_CORE=1
        PICO_CORE1_VTABLE_PLACEMENT=__in_scratch_x
    )
    pico_add_extra_outputs(pico_multicore_vtable_scratch_test)

    add_executable(pico_multicore_vtable_scratch_test_copy_to_ram pico_multicore_test.c)
    target_link_libraries(pico_multicore_vtable_scratch_test_copy_to_ram PRIVATE pico_stdlib pico_test pico_multicore)
    target_compile_definitions(pico_multicore_vtable_scratch_test_copy_to_ram PRIVATE
        PICO_VTABLE_PER_CORE=1
        PICO_CORE1_VTABLE_PLACEMENT=__in_scratch_x
    )
    pico_set_binary_type(pico_multicore_vtable_scratch_test_copy_to_ram copy_to_ram)
    pico_add_extra_outputs(pico_multicore_vtable_scratch_test_copy_to_ram)

    add_executable(pico_multicore_vtable_scratch_test_no_flash pico_multicore_test.c)
    target_link_libraries(pico_multicore_vtable_scratch_test_no_flash PRIVATE pico_stdlib pico_test pico_multicore)
    target_compile_definitions(pico_multicore_vtable_scratch_test_no_flash PRIVATE
        PICO_VTABLE_PER_CORE=1
        PICO_CORE1_VTABLE_PLACEMENT=__in_scratch_x
    )
    pico_set_binary_type(pico_multicore_vtable_scratch_test_no_flash no_flash)
    if (NOT PICO_RP2040)
        pico_package_uf2_output(pico_multicore_vtable_scratch_test_no_flash)
    endif()
    pico_add_extra_outputs(pico_multicore_vtable_scratch_test_no_flash)
endif()
