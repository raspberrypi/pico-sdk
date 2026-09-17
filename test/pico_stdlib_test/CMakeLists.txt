add_executable(pico_stdlib_test pico_stdlib_test.c)
set_target_properties(pico_stdlib_test PROPERTIES PICO_TEST_TIMEOUT "15")
target_compile_definitions(pico_stdlib_test PRIVATE
        #PICO_ENTER_USB_BOOT_ON_EXIT=1
)

target_link_libraries(pico_stdlib_test PRIVATE pico_stdlib)
pico_add_extra_outputs(pico_stdlib_test)
