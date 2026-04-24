if (TARGET pico_multicore)
    # Note the global versions don't actually need multicore, but it didn't seem worth adding #ifdef to them all
    add_executable(pico_tls_test pico_tls_test.c)
    target_link_libraries(pico_tls_test PRIVATE pico_stdlib pico_multicore)
    pico_add_extra_outputs(pico_tls_test)

    add_executable(pico_tls_test_global pico_tls_test.c)
    target_link_libraries(pico_tls_test_global PRIVATE pico_stdlib pico_multicore)
    pico_add_extra_outputs(pico_tls_test_global)
    pico_set_tls_implementation(pico_tls_test_global global)

    add_executable(pico_tls_test_cpp pico_tls_test_cpp.cpp)
    target_link_libraries(pico_tls_test_cpp PRIVATE pico_stdlib pico_multicore)
    pico_add_extra_outputs(pico_tls_test_cpp)

    add_executable(pico_tls_test_cpp_global pico_tls_test_cpp.cpp)
    target_link_libraries(pico_tls_test_cpp_global PRIVATE pico_stdlib pico_multicore)
    pico_add_extra_outputs(pico_tls_test_cpp_global)
    pico_set_tls_implementation(pico_tls_test_cpp_global global)
endif()