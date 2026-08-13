# note: check for an empty value too, since FreeRTOS_Kernel_import.cmake picks up a
# defined-but-empty environment variable and then hard-errors rather than skipping
if (NOT FREERTOS_KERNEL_PATH AND "$ENV{FREERTOS_KERNEL_PATH}" STREQUAL "")
    message("Skipping freertos_sync_alias_test as FREERTOS_KERNEL_PATH is not defined")
    return()
endif()

include(FreeRTOS_Kernel_import.cmake)

if (NOT TARGET FreeRTOS-Kernel-Heap4)
    message("Skipping freertos_sync_alias_test as the FreeRTOS kernel was not found")
    return()
endif()

add_executable(freertos_sync_alias_test freertos_sync_alias_test.c)

target_include_directories(freertos_sync_alias_test PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(freertos_sync_alias_test PRIVATE
        pico_stdlib
        pico_sync
        pico_time
        FreeRTOS-Kernel-Heap4
        )

# Single core keeps the scheduling deterministic; the race under test is between tasks on
# the FreeRTOS core, not between cores.
target_compile_definitions(freertos_sync_alias_test PRIVATE
        configNUMBER_OF_CORES=1
        )

pico_add_extra_outputs(freertos_sync_alias_test)

set_target_properties(freertos_sync_alias_test PROPERTIES PICO_TEST_TIMEOUT 120)
