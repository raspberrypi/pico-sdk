if (NOT TARGET cmsis_core)
    message("Skipping cmsis_test as cmsis_core is unavailable on this platform")
    return()
endif()
# todo remove check
if (NOT PICO_RISCV)
    add_executable(cmsis_test cmsis_test.c)

    target_link_libraries(cmsis_test PRIVATE pico_stdlib cmsis_core)
    pico_add_extra_outputs(cmsis_test)
endif()