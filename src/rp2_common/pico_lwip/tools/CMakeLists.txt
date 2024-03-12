# Compile the http content into a source file "pico_fsdata.inc" in a format suitable for the lwip httpd server
# Pass the target library name library type and the list of httpd content
function(pico_set_lwip_httpd_content TARGET_LIB TARGET_TYPE)
    find_package (Python3 REQUIRED COMPONENTS Interpreter)
    set(HTTPD_CONTENT_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
    set(HTTPD_CONTENT_TARGET "${TARGET_LIB}_pico_set_lwip_httpd_content")
    set(HTTPD_CONTENT_OUTPUT_NAME "pico_fsdata.inc")
    set(HTTPD_CONTENT_TOOL "${PICO_SDK_PATH}/src/rp2_common/pico_lwip/tools/makefsdata.py")
    add_custom_target(${HTTPD_CONTENT_TARGET} DEPENDS ${HTTPD_CONTENT_BINARY_DIR}/${HTTPD_CONTENT_OUTPUT_NAME})
    add_custom_command(
        OUTPUT ${HTTPD_CONTENT_BINARY_DIR}/${HTTPD_CONTENT_OUTPUT_NAME}
        DEPENDS ${HTTPD_CONTENT_TOOL} ${ARGN}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${HTTPD_CONTENT_BINARY_DIR} &&
            ${Python3_EXECUTABLE} ${HTTPD_CONTENT_TOOL} -i ${ARGN} -o ${HTTPD_CONTENT_BINARY_DIR}/${HTTPD_CONTENT_OUTPUT_NAME}
        VERBATIM)
    target_include_directories(${TARGET_LIB} ${TARGET_TYPE}
        ${HTTPD_CONTENT_BINARY_DIR}
        )
    add_dependencies(${TARGET_LIB} ${HTTPD_CONTENT_TARGET})
endfunction()
