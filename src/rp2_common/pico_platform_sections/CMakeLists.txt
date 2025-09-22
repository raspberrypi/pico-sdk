if (NOT TARGET pico_platform_sections)
    pico_add_library(pico_platform_sections)

    target_include_directories(pico_platform_sections_headers SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
endif()

# pico_sections_time_critical(TARGET [SOURCES])
# \brief\ Prefix target's object file sections with ".time_critical"
#
# This function will apply "objcopy --prefix-alloc-sections .time_critical" to all the object files of
# TARGET that match either an optionally specified list of source files or all the target's "SOURCES" found
# within in the target's "SOURCE_DIR".
#
# This utility function is only available in builds using CMake 3.27 and later.
#
# Examples:
# pico_sections_time_critical(MyTarget)
#
# pico_sections_time_critical(MyTarget
#         some_time_critical_code.c
#         other_time_critical_code.c
# )
#
# \param\ TARGET The build target
# \param\ SOURCES Optional, source files of the object files to be modified. If not specified, uses the build
# target's "SOURCES" list.
#
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.27)
function(pico_sections_time_critical TARGET)
    add_custom_command(
            TARGET ${TARGET}
            PRE_LINK
            COMMAND ${CMAKE_COMMAND} -E echo "execute_process($<LIST:TRANSFORM,$<LIST:FILTER,$<LIST:TRANSFORM,$<TARGET_OBJECTS:${TARGET}>,REPLACE,/\./,/>,INCLUDE,$<LIST:JOIN,$<LIST:TRANSFORM,$<LIST:TRANSFORM,$<LIST:TRANSFORM,$<LIST:FILTER,$<LIST:TRANSFORM,$<IF:$<STREQUAL:${ARGN},>,$<TARGET_PROPERTY:${TARGET},SOURCES>,${ARGN}>,REPLACE,^$<TARGET_PROPERTY:${TARGET},SOURCE_DIR>/,>,EXCLUDE,^/|\.h$>,PREPEND,/$<TARGET_NAME:${TARGET}>.dir/>,APPEND,.o$>,REPLACE,\\\.,\\\\.>,|>>,PREPEND,COMMAND ${CMAKE_OBJCOPY} --prefix-alloc-sections .time_critical >)" > ${TARGET}_sections_time_critical.cmake
            COMMAND ${CMAKE_COMMAND} -P ${TARGET}_sections_time_critical.cmake
            COMMAND ${CMAKE_COMMAND} -E echo "$<IF:$<STREQUAL:${ARGN},>,All,Selected> \"$<TARGET_NAME:${TARGET}>\" object file alloc-section names have been updated for \"time_critical\" linker placement"
            VERBATIM
            COMMAND_EXPAND_LISTS
    )
endfunction()
endif()

# pico_sections_not_in_flash(TARGET [SOURCES])
# \brief\ Prefix target's object file sections with ".time_critical_ram"
#
# This function will apply "objcopy --prefix-alloc-sections .time_critical_ram" to all the object files of
# TARGET that match either an optionally specified list of source files or all the target's "SOURCES" found
# within in the target's "SOURCE_DIR".
#
# This utility function is only available in builds using CMake 3.27 and later.
#
# Examples:
# pico_sections_not_in_flash(MyTarget)
#
# pico_sections_not_in_flash(MyTarget
#         some_code.c
#         other_code.c
# )
#
# \param\ TARGET The build target
# \param\ SOURCES Optional, source files of the object files to be modified. If not specified, uses the build
# target's "SOURCES" list.
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.27)
function(pico_sections_not_in_flash TARGET)
    add_custom_command(
            TARGET ${TARGET}
            PRE_LINK
            COMMAND ${CMAKE_COMMAND} -E echo "execute_process($<LIST:TRANSFORM,$<LIST:FILTER,$<LIST:TRANSFORM,$<TARGET_OBJECTS:${TARGET}>,REPLACE,/\./,/>,INCLUDE,$<LIST:JOIN,$<LIST:TRANSFORM,$<LIST:TRANSFORM,$<LIST:TRANSFORM,$<LIST:FILTER,$<LIST:TRANSFORM,$<IF:$<STREQUAL:${ARGN},>,$<TARGET_PROPERTY:${TARGET},SOURCES>,${ARGN}>,REPLACE,^$<TARGET_PROPERTY:${TARGET},SOURCE_DIR>/,>,EXCLUDE,^/|\.h$>,PREPEND,/$<TARGET_NAME:${TARGET}>.dir/>,APPEND,.o$>,REPLACE,\\\.,\\\\.>,|>>,PREPEND,COMMAND ${CMAKE_OBJCOPY} --prefix-alloc-sections .time_critical_ram >)" > ${TARGET}_sections_not_in_flash.cmake
            COMMAND ${CMAKE_COMMAND} -P ${TARGET}_sections_not_in_flash.cmake
            COMMAND ${CMAKE_COMMAND} -E echo "$<IF:$<STREQUAL:${ARGN},>,All,Selected> \"$<TARGET_NAME:${TARGET}>\" object file section names have been updated for \"not_in_flash\" linker placement"
            VERBATIM
            COMMAND_EXPAND_LISTS
    )
endfunction()
endif()