add_executable(pico_sync_test pico_sync_test.c)
target_compile_definitions(pico_sync_test PRIVATE PICO_USE_SW_SPIN_LOCKS=0)
target_include_directories(pico_sync_test PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_compile_definitions(pico_sync_test PRIVATE
        PICO_PLATFORM_TEST_HEADER=pico_sync_test_hook.h)
target_link_libraries(pico_sync_test PRIVATE pico_test pico_sync pico_multicore)
pico_add_extra_outputs(pico_sync_test)

if (NOT PICO_RP2040)
    add_executable(pico_sync_test_sw pico_sync_test.c)
    target_compile_definitions(pico_sync_test_sw PRIVATE PICO_USE_SW_SPIN_LOCKS=1)
    target_include_directories(pico_sync_test_sw PRIVATE ${CMAKE_CURRENT_LIST_DIR})
    target_compile_definitions(pico_sync_test_sw PRIVATE PICO_PLATFORM_TEST_HEADER=pico_sync_test_hook.h)
    target_link_libraries(pico_sync_test_sw PRIVATE pico_test pico_sync pico_multicore)
    pico_add_extra_outputs(pico_sync_test_sw)
endif()

