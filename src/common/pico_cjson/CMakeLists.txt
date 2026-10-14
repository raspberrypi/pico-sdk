# PICO_CMAKE_CONFIG: PICO_CJSON_PATH, Path to cJSON. Can be passed to CMake or set in your environment if you do not wish to use the version included with the SDK, type=string, default=<PICO_SDK_PATH>/lib/cJSON, group=pico_cjson
if (DEFINED ENV{PICO_CJSON_PATH} AND (NOT PICO_CJSON_PATH))
    set(PICO_CJSON_PATH $ENV{PICO_CJSON_PATH})
    message("Using PICO_CJSON_PATH from environment ('${PICO_CJSON_PATH}')")
endif ()

set(CJSON_TEST_PATH "cJSON.c")
if (NOT PICO_CJSON_PATH)
    set(PICO_CJSON_PATH ${PICO_SDK_PATH}/lib/cJSON)
    if (NOT EXISTS ${PICO_CJSON_PATH}/${CJSON_TEST_PATH})
        message(WARNING "cJSON submodule has not been initialized; Pi Connect support will be unavailable
hint: try 'git submodule update --init' from your SDK directory (${PICO_SDK_PATH}).")
    endif()
elseif (NOT EXISTS ${PICO_CJSON_PATH}/${CJSON_TEST_PATH})
    message(WARNING "PICO_CJSON_PATH specified but content not present.")
endif()

if (EXISTS ${PICO_CJSON_PATH}/${CJSON_TEST_PATH} AND NOT TARGET pico_cjson)
    message("cJSON available at ${PICO_CJSON_PATH}")

    pico_register_common_scope_var(PICO_CJSON_PATH)

    pico_add_library(pico_cjson)
    target_include_directories(pico_cjson_headers SYSTEM INTERFACE ${PICO_CJSON_PATH})
    target_sources(pico_cjson INTERFACE
        ${PICO_CJSON_PATH}/cJSON.c)
    target_link_libraries(pico_cjson INTERFACE pico_stdlib)

    function(suppress_cjson_warnings)
        set_source_files_properties(
            ${PICO_CJSON_PATH}/cJSON.c
            PROPERTIES
            COMPILE_OPTIONS "-Wno-float-equal"
        )
    endfunction()

    pico_promote_common_scope_vars()
endif()
