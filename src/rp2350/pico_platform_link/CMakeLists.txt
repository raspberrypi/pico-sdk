pico_register_common_scope_var(PICO_LINKER_SCRIPT_PATH)
if (NOT PICO_LINKER_SCRIPT_PATH)
    set(PICO_LINKER_SCRIPT_PATH ${CMAKE_CURRENT_LIST_DIR})
endif()

pico_promote_common_scope_vars()