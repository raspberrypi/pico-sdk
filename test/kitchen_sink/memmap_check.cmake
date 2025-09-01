execute_process(COMMAND ${CMAKE_COMMAND}
            -E compare_files 
            ${DEFAULT_LINKER_SCRIPT}
            ${MODIFIED_LINKER_SCRIPT} RESULT_VARIABLE compare_result)

if (NOT compare_result EQUAL 0)
    message(FATAL_ERROR "Generated linker script ${MODIFIED_LINKER_SCRIPT} does not match default linker script ${DEFAULT_LINKER_SCRIPT}")
endif()
