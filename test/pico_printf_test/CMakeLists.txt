add_executable(pico_printf_test pico_printf_test.c)
target_link_libraries(pico_printf_test pico_stdlib)
# the test is of pico_printf, not the compiler's printf
pico_set_printf_implementation(pico_printf_test pico)
# otherwise the compiler formats some of these calls itself
target_compile_options(pico_printf_test PRIVATE -fno-builtin)
pico_add_extra_outputs(pico_printf_test)
