set(UZLIB_TEST_PATH "tinflate.c")
if (NOT PICO_UZLIB_SRC_PATH)
    set(PICO_UZLIB_SRC_PATH ${PROJECT_SOURCE_DIR}/lib/uzlib/src)
endif()
if (NOT EXISTS ${PICO_UZLIB_SRC_PATH}/${UZLIB_TEST_PATH})
    message(WARNING "uzlib content not present.")
endif()

if (EXISTS ${PICO_UZLIB_SRC_PATH}/${UZLIB_TEST_PATH})
    message("uzlib source available at ${PICO_UZLIB_SRC_PATH}")

    pico_register_common_scope_var(PICO_UZLIB_PATH)

    pico_add_library(pico_uzlib)
    target_sources(pico_uzlib INTERFACE
        ${PICO_UZLIB_SRC_PATH}/tinflate.c
        ${PICO_UZLIB_SRC_PATH}/tinfgzip.c
        ${PICO_UZLIB_SRC_PATH}/tinfzlib.c
        ${PICO_UZLIB_SRC_PATH}/adler32.c
        ${PICO_UZLIB_SRC_PATH}/crc32.c
    )
    target_include_directories(pico_uzlib_headers INTERFACE
        ${PICO_UZLIB_SRC_PATH}
    )

    pico_promote_common_scope_vars()

    function(suppress_uzlib_warnings)
        set_source_files_properties(
            ${PICO_UZLIB_SRC_PATH}/tinflate.c
            PROPERTIES
            COMPILE_OPTIONS "-Wno-sign-compare"
            )
    endfunction()

endif()