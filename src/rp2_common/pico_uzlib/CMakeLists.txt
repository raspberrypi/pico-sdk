set(UZLIB_TEST_PATH "src/tinflate.c")
if (NOT PICO_UZLIB_PATH)
    set(PICO_UZLIB_PATH ${PROJECT_SOURCE_DIR}/lib/uzlib)
endif()
if (NOT EXISTS ${PICO_UZLIB_PATH}/${UZLIB_TEST_PATH})
    message(WARNING "uzlib content not present.")
endif()

if (EXISTS ${PICO_UZLIB_PATH}/${UZLIB_TEST_PATH})
    message("uzlib available at ${PICO_UZLIB_PATH}")

    pico_register_common_scope_var(PICO_UZLIB_PATH)

    add_library(pico_uzlib INTERFACE)
    target_sources(pico_uzlib INTERFACE
        ${PICO_UZLIB_PATH}/src/tinflate.c
        ${PICO_UZLIB_PATH}/src/tinfgzip.c
        ${PICO_UZLIB_PATH}/src/tinfzlib.c
        ${PICO_UZLIB_PATH}/src/adler32.c
        ${PICO_UZLIB_PATH}/src/crc32.c
        ${PICO_UZLIB_PATH}/src/defl_static.c
    )
    target_include_directories(pico_uzlib INTERFACE
        ${PICO_UZLIB_PATH}/src
    )

    pico_promote_common_scope_vars()

    function(suppress_uzlib_warnings)
        set_source_files_properties(
            ${PICO_UZLIB_PATH}/src/defl_static.c
            PROPERTIES
            COMPILE_OPTIONS "-Wno-redundant-decls"
            )
        set_source_files_properties(
            ${PICO_UZLIB_PATH}/src/tinflate.c
            PROPERTIES
            COMPILE_OPTIONS "-Wno-sign-compare"
            )
    endfunction()

endif()