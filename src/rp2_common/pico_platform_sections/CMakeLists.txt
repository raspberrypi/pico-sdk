if (NOT TARGET pico_platform_sections)
    pico_add_library(pico_platform_sections)

    target_include_directories(pico_platform_sections_headers SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
endif()

# prefix_alloc_sections_for_linker_placement(PLACEMENT_TYPE PREFIX TARGET [SOURCES])
# \brief\ Common implementation for pico_sections_time_critical() and pico_sections_not_in_flash() functions
#
# This local helper function is only available in builds using CMake 3.27 and later.
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.27)
    function(prefix_alloc_sections_for_linker_placement PLACEMENT_TYPE PREFIX TARGET)
        add_custom_command(
                TARGET ${TARGET}
                PRE_LINK
                COMMAND ${CMAKE_COMMAND} -E echo "execute_process($<LIST:TRANSFORM,$<LIST:FILTER,$<LIST:TRANSFORM,$<TARGET_OBJECTS:${TARGET}>,REPLACE,/\./,/>,INCLUDE,$<LIST:JOIN,$<LIST:TRANSFORM,$<LIST:TRANSFORM,$<LIST:TRANSFORM,$<LIST:FILTER,$<LIST:TRANSFORM,$<IF:$<STREQUAL:${ARGN},>,$<TARGET_PROPERTY:${TARGET},SOURCES>,${ARGN}>,REPLACE,^$<TARGET_PROPERTY:${TARGET},SOURCE_DIR>/,>,EXCLUDE,^/|\.h$>,PREPEND,/$<TARGET_NAME:${TARGET}>.dir/>,APPEND,.o$>,REPLACE,\\\.,\\\\.>,|>>,PREPEND,COMMAND ${CMAKE_OBJCOPY} --prefix-alloc-sections ${PREFIX} >)" > ${TARGET}_sections_${PLACEMENT_TYPE}.cmake
                COMMAND ${CMAKE_COMMAND} -P ${TARGET}_sections_${PLACEMENT_TYPE}.cmake
                COMMAND ${CMAKE_COMMAND} -E echo "$<IF:$<STREQUAL:${ARGN},>,All,Selected> \"$<TARGET_NAME:${TARGET}>\" object file alloc-section names have been updated for \"${PLACEMENT_TYPE}\" linker placement"
                VERBATIM
                COMMAND_EXPAND_LISTS
        )
    endfunction()
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
    prefix_alloc_sections_for_linker_placement(time_critical .time_critical ${TARGET} ${ARGN})
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
    prefix_alloc_sections_for_linker_placement(not_in_flash .time_critical_ram ${TARGET} ${ARGN})
endfunction()
endif()