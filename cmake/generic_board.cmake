# For boards without their own cmake file, we look for a header file
cmake_minimum_required(VERSION 3.15)

# PICO_CMAKE_CONFIG: PICO_BOARD_HEADER_DIRS, List of directories to look for <PICO_BOARD>.h in. This may be specified the user environment, type=list, group=build
if (DEFINED ENV{PICO_BOARD_HEADER_DIRS})
    set(PICO_BOARD_HEADER_DIRS $ENV{PICO_BOARD_HEADER_DIRS})
    message("Using PICO_BOARD_HEADER_DIRS from environment ('${PICO_BOARD_HEADER_DIRS}')")
endif()
set(PICO_BOARD_HEADER_DIRS ${PICO_BOARD_HEADER_DIRS} CACHE STRING "PICO board header directories" FORCE)

list(APPEND PICO_BOARD_HEADER_DIRS ${CMAKE_CURRENT_LIST_DIR}/../src/boards/include/boards)
pico_find_in_paths(PICO_BOARD_HEADER_FILE PICO_BOARD_HEADER_DIRS ${PICO_BOARD}.h)

if (EXISTS ${PICO_BOARD_HEADER_FILE})
    message("Using board configuration from ${PICO_BOARD_HEADER_FILE}")
    list(APPEND PICO_CONFIG_HEADER_FILES ${PICO_BOARD_HEADER_FILE})

    # we parse the header file to configure the defaults
    file(STRINGS ${PICO_BOARD_HEADER_FILE} HEADER_FILE_CONTENTS)

    while(HEADER_FILE_CONTENTS)
        list(POP_FRONT HEADER_FILE_CONTENTS LINE)
        if (LINE MATCHES "^[ \t\]*//[ \t\]*pico_cmake_set[ \t\]*([a-zA-Z_][a-zA-Z0-9_]*)[ \t\]*=[ \t\]*(.*)")
            set("${CMAKE_MATCH_1}" "${CMAKE_MATCH_2}")
        endif()
        if (LINE MATCHES "^[ \t\]*//[ \t\]*pico_cmake_set_default[ \t\]*([a-zA-Z_][a-zA-Z0-9_]*)[ \t\]*=[ \t\]*(.*)")
            if (NOT DEFINED "${CMAKE_MATCH_1}")
                set("${CMAKE_MATCH_1}" "${CMAKE_MATCH_2}")
            else()
                list(APPEND PICO_BOARD_CMAKE_OVERRIDES ${CMAKE_MATCH_1})
            endif()
        endif()
    endwhile()
else()
    set(msg "Unable to find definition of board '${PICO_BOARD}' (specified by PICO_BOARD):\n")
    list(JOIN PICO_BOARD_HEADER_DIRS ", " DIRS)
    string(CONCAT msg ${msg} "   Looked for ${PICO_BOARD}.h in ${DIRS} (additional paths specified by PICO_BOARD_HEADER_DIRS)\n")
    list(JOIN PICO_BOARD_CMAKE_DIRS ", " DIRS)
    string(CONCAT msg ${msg} "   Looked for ${PICO_BOARD}.cmake in ${DIRS} (additional paths specified by PICO_BOARD_CMAKE_DIRS)")
    message(FATAL_ERROR ${msg})
endif()
