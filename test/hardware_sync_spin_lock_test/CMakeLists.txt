if (NOT TARGET pico_multicore)
	message("Skipping hardware_sync_spin_lock_test as pico_multicore is unavailable on this platform")
	return()
endif()

add_executable(hardware_sync_spin_lock_test hardware_sync_spin_lock_test.c)

target_link_libraries(hardware_sync_spin_lock_test PRIVATE pico_test hardware_sync pico_multicore)
pico_add_extra_outputs(hardware_sync_spin_lock_test)

target_compile_definitions(hardware_sync_spin_lock_test PRIVATE
	PICO_STDOUT_MUTEX=0
	)
