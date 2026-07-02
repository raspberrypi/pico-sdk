add_executable(pico_sync_test_hw pico_sync_test.c)
target_compile_definitions(pico_sync_test_hw PRIVATE PICO_USE_SW_SPIN_LOCKS=0)
target_link_libraries(pico_sync_test_hw PRIVATE pico_test pico_sync)
pico_add_extra_outputs(pico_sync_test_hw)

if (NOT PICO_RP2040)
    add_executable(pico_sync_test_sw pico_sync_test.c)
    target_link_libraries(pico_sync_test_sw PRIVATE pico_test pico_sync)
    pico_add_extra_outputs(pico_sync_test_sw)
endif()

