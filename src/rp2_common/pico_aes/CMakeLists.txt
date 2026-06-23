pico_add_library(pico_aes)

target_sources(pico_aes INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/aes.c
    ${CMAKE_CURRENT_LIST_DIR}/aes.S
)

target_include_directories(pico_aes_headers SYSTEM INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

pico_mirrored_target_link_libraries(pico_aes INTERFACE
    pico_bootrom
)

function(pico_aes_configure_binary TARGET)
    pico_include_in_generated_section(${TARGET} post_data section_aes_scratch.incl)
    pico_include_in_generated_section(${TARGET} post_end section_aes_end.incl)
    target_compile_definitions(${TARGET} PRIVATE PICO_AES_CONFIGURED=1 PICO_AES_SINGLE_USE=0)
endfunction()

function(pico_aes_configure_single_use_binary TARGET)
    target_compile_definitions(${TARGET} PRIVATE PICO_AES_CONFIGURED=1 PICO_AES_SINGLE_USE=1)
endfunction()
