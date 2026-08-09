function(add_interop_variant NAME)
    add_executable(${NAME}
            sync_interop_test.c
            interop_platform.c
            interop_harness.c
            )
    target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR})
    target_link_libraries(${NAME} PRIVATE
            pico_stdlib
            pico_sync
            pico_time
            pico_multicore
            ${INTEROP_EXTRA_LIBS}
            )
    # printf is instrumentation here, not the subject: the two disciplines are modelled
    # with our own mutexes so that no case depends on the value of PICO_STDOUT_MUTEX.
    target_compile_definitions(${NAME} PRIVATE
            PICO_STDOUT_MUTEX=0
            ${ARGN}
            )
    # D1.7/D2.5 do a blocking lock_core wait from ISR context. That is legal bare (and so
    # runs by default there), but the FreeRTOS port forbids it with
    # configASSERT(!portCHECK_IF_IN_ISR()), which aborts the run - so under FreeRTOS they
    # must be enabled deliberately, one at a time: cmake -DINTEROP_RUN_ISR_BLOCKING=1
    if (INTEROP_RUN_ISR_BLOCKING)
        target_compile_definitions(${NAME} PRIVATE INTEROP_RUN_ISR_BLOCKING=1)
    endif()
    pico_add_extra_outputs(${NAME})
    set_target_properties(${NAME} PROPERTIES PICO_TEST_TIMEOUT 120)
endfunction()

# Every variant is built twice, following the pico_time_test/_sw convention. This matters a
# great deal here: PICO_SYNC_RP2350_SPIN_LOCK_WORKAROUND only exists under software spin
# locks (a hardware unlock is a plain SIO write and does not set the event), so a run that
# does not pin this is testing whichever implementation the platform defaulted to.
# Software spin locks need Armv8-M or RISC-V atomics, so there is no _sw twin on RP2040 -
# forcing it there is a hard #error in spin_lock.h, not a fallback.
function(add_interop_variants NAME)
    add_interop_variant(${NAME} PICO_USE_SW_SPIN_LOCKS=0 ${ARGN})
    if (NOT PICO_RP2040)
        add_interop_variant(${NAME}_sw PICO_USE_SW_SPIN_LOCKS=1 ${ARGN})
    endif()
endfunction()

# The baseline needs no RTOS, so it always builds. It is the reference run: it exercises the
# SDK's own lock_core macros, so a failure here implicates the SDK (or the harness), and
# anything that fails only under FreeRTOS implicates interop.
set(INTEROP_EXTRA_LIBS "")
add_interop_variants(sync_interop_test_baseline)

# Same core assignment as the _core1 FreeRTOS build (cases on core 1, agent on core 0) but
# with no RTOS, so that a core-specific failure can be told apart from an interop one.
add_interop_variants(sync_interop_test_baseline_rev INTEROP_TESTS_ON_CORE=1)

# note: check for an empty value too, since FreeRTOS_Kernel_import.cmake picks up a
# defined-but-empty environment variable and then hard-errors rather than skipping
if (NOT FREERTOS_KERNEL_PATH AND "$ENV{FREERTOS_KERNEL_PATH}" STREQUAL "")
    message("Skipping the FreeRTOS sync_interop_test variants as FREERTOS_KERNEL_PATH is not defined")
    return()
endif()

include(FreeRTOS_Kernel_import.cmake)

if (NOT TARGET FreeRTOS-Kernel-Heap4)
    message("Skipping the FreeRTOS sync_interop_test variants as the kernel was not found")
    return()
endif()

set(INTEROP_EXTRA_LIBS FreeRTOS-Kernel-Heap4)

# FreeRTOS SMP across both cores - no bare-SDK core exists, so cross-core cases skip
add_interop_variants(sync_interop_test_smp
        INTEROP_HAVE_FREERTOS=1
        configNUMBER_OF_CORES=2
        )

# FreeRTOS on core 0, bare SDK code on core 1
add_interop_variants(sync_interop_test_core0
        INTEROP_HAVE_FREERTOS=1
        configNUMBER_OF_CORES=1
        RUN_FREE_RTOS_ON_CORE=0
        )

# FreeRTOS on core 1, bare SDK code on core 0 - catches core-0 assumptions
add_interop_variants(sync_interop_test_core1
        INTEROP_HAVE_FREERTOS=1
        configNUMBER_OF_CORES=1
        RUN_FREE_RTOS_ON_CORE=1
        )
