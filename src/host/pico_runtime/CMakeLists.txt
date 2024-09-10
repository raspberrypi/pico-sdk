pico_add_library(pico_runtime)

target_include_directories(pico_runtime_headers SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

pico_mirrored_target_link_libraries(pico_runtime INTERFACE
        pico_base
)

function(pico_minimize_runtime TARGET)
endfunction()