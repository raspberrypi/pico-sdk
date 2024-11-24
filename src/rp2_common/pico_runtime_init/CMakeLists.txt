pico_add_library(pico_runtime_init)

target_sources(pico_runtime_init INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/runtime_init.c
        ${CMAKE_CURRENT_LIST_DIR}/runtime_init_clocks.c
        ${CMAKE_CURRENT_LIST_DIR}/runtime_init_stack_guard.c
)

target_include_directories(pico_runtime_init_headers SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

pico_mirrored_target_link_libraries(pico_runtime_init INTERFACE
        pico_base
)

if (TARGET hardware_clocks)
    pico_mirrored_target_link_libraries(pico_runtime_init INTERFACE hardware_clocks)
endif()

# pico/runtime_init.h includes pico/runtime.h
target_link_libraries(pico_runtime_init_headers INTERFACE pico_runtime_headers)