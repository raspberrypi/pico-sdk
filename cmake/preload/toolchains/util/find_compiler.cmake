# Toolchain file is processed multiple times, however, it cannot access CMake cache on some runs.
# We store the search path in an environment variable so that we can always access it.
if (NOT "${PICO_TOOLCHAIN_PATH}" STREQUAL "")
    set(ENV{PICO_TOOLCHAIN_PATH} "${PICO_TOOLCHAIN_PATH}")
endif ()

# Find the compiler executable and store its path in a cache entry ${compiler_path}.
# If not found, issue a fatal message and stop processing. PICO_TOOLCHAIN_PATH can be provided from
# commandline as additional search path.
function(pico_find_compiler compiler_path compiler_exes)
    # Search user provided path first.
    find_program(
            ${compiler_path} NAMES ${compiler_exes}
            PATHS ENV PICO_TOOLCHAIN_PATH
            PATH_SUFFIXES bin
            NO_DEFAULT_PATH
    )

    # If not then search system paths.
    if ("${${compiler_path}}" STREQUAL "${compiler_path}-NOTFOUND")
        if (DEFINED ENV{PICO_TOOLCHAIN_PATH})
            message(WARNING "PICO_TOOLCHAIN_PATH specified ($ENV{PICO_TOOLCHAIN_PATH}), but ${compiler_exe} not found there")
        endif()
        find_program(${compiler_path} NAMES ${compiler_exes})
    endif ()
    if ("${${compiler_path}}" STREQUAL "${compiler_path}-NOTFOUND")
        set(PICO_TOOLCHAIN_PATH "" CACHE PATH "Path to search for compiler.")
        list(JOIN compiler_exes " / " compiler_exes)
        message(FATAL_ERROR "Compiler '${compiler_exes}' not found, you can specify search path with\
            \"PICO_TOOLCHAIN_PATH\".")
    endif ()
endfunction()

# Find the compiler executable and store its path in a cache entry ${compiler_path}.
# If not found, issue a fatal message and stop processing. PICO_TOOLCHAIN_PATH can be provided from
# commandline as additional search path.
function(pico_find_compiler_with_triples compiler_path triples compiler_suffix)
    list(TRANSFORM triples APPEND "-${compiler_suffix}")
    pico_find_compiler(${compiler_path} "${triples}")
    set(${compiler_path} ${${compiler_path}} PARENT_SCOPE)
endfunction()

# If no single compiler flags variable is set, then choose first working compiler flags from a list.
# This is done by doing a test compile via execute_process, as we can't use check_c_compiler_flag
# prior to project_setup.
# Assuming the single flag variable is not already set, each of the flag_var_list flags are tried in order.
# If the flags don't cause an error the single flag variable is set to the good flag value. If no valid flags
# are found, a fatal error is raised.
function(pico_choose_compiler_flags compiler_path common_lang_flags_var common_lang_flags_var_list test_files_var)
    # simplify logic by not complaining if both set, since CMake calls compiler setup repeatedly
    if (NOT ${common_lang_flags_var})
        if (${common_lang_flags_var_list})
            if(CMAKE_HOST_WIN32)
                set(NULL_DEVICE "NUL")
            else()
                set(NULL_DEVICE "/dev/null")
            endif()

            set(idx 0)
            foreach(flags IN LISTS ${common_lang_flags_var_list})
                # Use per-entry test file if provided, otherwise empty.c
                if (${test_files_var})
                    list(GET ${test_files_var} ${idx} test_file)
                else()
                    set(test_file "${CMAKE_CURRENT_LIST_DIR}/empty.c")
                endif()

                separate_arguments(COMPILER_CMD NATIVE_COMMAND
                    "${compiler_path} ${flags} -x c -c \"${test_file}\" -o ${NULL_DEVICE}")
                execute_process(
                        COMMAND ${COMPILER_CMD}
                        RESULT_VARIABLE COMPILE_FAILED
                        ERROR_QUIET
                )

                if (NOT COMPILE_FAILED)
                    set(${common_lang_flags_var} ${flags} PARENT_SCOPE)
                    return()
                endif()
                math(EXPR idx "${idx} + 1")
            endforeach()
            message(FATAL_ERROR "No compiler found that supports required compiler flags")
        else()
            if (${common_lang_flags_var_list})
                message(FATAL_ERROR "Either ${common_lang_flags_var} or ${common_lang_flags_var_list} must be set")
            endif()
        endif()
    endif()
endfunction()
