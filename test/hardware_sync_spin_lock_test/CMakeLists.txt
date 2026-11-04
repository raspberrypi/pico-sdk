if (NOT TARGET pico_multicore)
	message("Skipping hardware_sync_spin_lock_test as pico_multicore is unavailable on this platform")
	return()
endif()

# The SEV probe measures a property only software spin locks have, so it must be pinned rather
# than inherited from the platform default; the hardware build asserts the negative.
function(add_spin_lock_test_variant NAME)
	add_executable(${NAME} hardware_sync_spin_lock_test.c)
	target_link_libraries(${NAME} PRIVATE pico_test hardware_sync pico_multicore)
	pico_add_extra_outputs(${NAME})
	target_compile_definitions(${NAME} PRIVATE
		PICO_STDOUT_MUTEX=0
		${ARGN}
		)
	set_target_properties(${NAME} PROPERTIES PICO_TEST_TIMEOUT 10)
endfunction()

add_spin_lock_test_variant(hardware_sync_spin_lock_test PICO_USE_SW_SPIN_LOCKS=0)

if (NOT PICO_RP2040)
	add_spin_lock_test_variant(hardware_sync_spin_lock_test_sw PICO_USE_SW_SPIN_LOCKS=1)
endif()
