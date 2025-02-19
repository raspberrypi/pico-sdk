# PICO_BOARD is the root of config as it can define PICO_PLATFORM and other build vars

# PICO_CMAKE_CONFIG: PICO_BOARD, Board name being built for. This may be specified in the user environment, type=string, default=pico or pico2, group=build, docref=cmake-platform-board-config
if (DEFINED ENV{PICO_BOARD} AND NOT PICO_BOARD)
    set(PICO_BOARD $ENV{PICO_BOARD})
    message("Initializing PICO_BOARD from environment ('${PICO_BOARD}')")
endif()

# PICO_CMAKE_CONFIG: PICO_PLATFORM, Platform to build for e.g. rp2040/rp2350/rp2350-arm-s/rp2350-riscv/host. This may be specified in the user environment, type=string, default=based on PICO_BOARD or environment value, group=build, docref=cmake-platform-board-config
if (DEFINED ENV{PICO_PLATFORM} AND NOT PICO_PLATFORM)
    set(PICO_PLATFORM $ENV{PICO_PLATFORM})
    message("Initializing PICO_PLATFORM from environment ('${PICO_PLATFORM}')")
endif()
set(PICO_SAVED_PLATFORM "${PICO_PLATFORM}")

# If PICO_PLATFORM is specified but not PICO_BOARD, we'll make a stab at defaulting
if (NOT PICO_DEFAULT_BOARD_rp2040)
    set(PICO_DEFAULT_BOARD_rp2040 "pico")
endif()
if (NOT PICO_DEFAULT_BOARD_rp2350)
    set(PICO_DEFAULT_BOARD_rp2350 "pico2")
endif()
if (NOT PICO_DEFAULT_BOARD_rp2350-arm-s)
    set(PICO_DEFAULT_BOARD_rp2350-arm-s "pico2")
endif()
if (NOT PICO_DEFAULT_BOARD_rp2350-riscv)
    set(PICO_DEFAULT_BOARD_rp2350-riscv "pico2")
endif()
if (NOT PICO_DEFAULT_BOARD_host)
    set(PICO_DEFAULT_BOARD_host "none")
endif()

if (NOT PICO_DEFAULT_PLATFORM)
    set(PICO_DEFAULT_PLATFORM "rp2040")
endif()

if (NOT PICO_BOARD)
    if (NOT PICO_PLATFORM)
        # if we have neither BOARD nor PLATFORM default PLATFORM silently, so we don't end up with a board of "none"
        # on platform that does have a default board (we want default PLATFORM and BOARD in that case)
        set(PICO_PLATFORM ${PICO_DEFAULT_PLATFORM})
        # set PICO_SAVED_PLATFORM so we don't print "Defaulting" again below
        set(PICO_SAVED_PLATFORM ${PICO_DEFAULT_PLATFORM})
        pico_message("Defaulting platform (PICO_PLATFORM) to '${PICO_PLATFORM}' since not specified.")
    endif()
    if (PICO_DEFAULT_BOARD_${PICO_PLATFORM})
        set(PICO_BOARD ${PICO_DEFAULT_BOARD_${PICO_PLATFORM}})
    else()
        set(PICO_BOARD "none")
    endif()
    pico_message("Defaulting target board (PICO_BOARD) to '${PICO_BOARD}' since not specified.")
else()
    message("Target board (PICO_BOARD) is '${PICO_BOARD}'.")
endif()
set(PICO_BOARD ${PICO_BOARD} CACHE STRING "PICO target board (e.g. pico, pico2)" FORCE)

# PICO_CMAKE_CONFIG: PICO_BOARD_CMAKE_DIRS, List of directories to look for <PICO_BOARD>.cmake in. This may be specified in the user environment, type=list, group=build
if (DEFINED ENV{PICO_BOARD_CMAKE_DIRS})
    set(PICO_BOARD_CMAKE_DIRS $ENV{PICO_BOARD_CMAKE_DIRS})
    message("Using PICO_BOARD_CMAKE_DIRS from environment ('${PICO_BOARD_CMAKE_DIRS}')")
endif()

list(APPEND PICO_BOARD_CMAKE_DIRS ${CMAKE_CURRENT_LIST_DIR}/../src/boards)

pico_find_in_paths(PICO_BOARD_CMAKE_FILE PICO_BOARD_CMAKE_DIRS ${PICO_BOARD}.cmake)
if (EXISTS "${PICO_BOARD_CMAKE_FILE}")
    message("Using CMake board configuration from ${PICO_BOARD_CMAKE_FILE}")
    include(${PICO_BOARD_CMAKE_FILE} board_config)
else()
    include(generic_board)
endif()

list(APPEND PICO_INCLUDE_DIRS ${PICO_SDK_PATH}/src/boards/include) # so boards/foo.h can be explicitly included

# PICO_CMAKE_CONFIG: PICO_DEFAULT_RP2350_PLATFORM, Default actual platform to build for if rp2350 is specified for PICO_PLATFORM e.g. rp2350-arm-s/rp2350-riscv, type=string, default=rp2350-arm-s, group=build
if (DEFINED ENV{PICO_DEFAULT_RP2350_PLATFORM} AND NOT PICO_DEFAULT_RP2350_PLATFORM)
    set(PICO_DEFAULT_RP2350_PLATFORM $ENV{PICO_DEFAULT_RP2350_PLATFORM})
endif()
if (NOT PICO_DEFAULT_RP2350_PLATFORM)
    set(PICO_DEFAULT_RP2350_PLATFORM "rp2350-arm-s")
endif()

if (NOT COMMAND pico_expand_pico_platform)
    function(pico_expand_pico_platform FUNC DO_MESSAGE)
        if (${FUNC} STREQUAL "rp2350")
            if (DO_MESSAGE)
                message("Auto-converting non-specific PICO_PLATFORM='rp2350' to '${PICO_DEFAULT_RP2350_PLATFORM}'")
            endif()
            set(${FUNC} "${PICO_DEFAULT_RP2350_PLATFORM}" PARENT_SCOPE)
        endif()
    endfunction()
endif()

if (NOT PICO_PLATFORM)
    set(PICO_PLATFORM ${PICO_DEFAULT_PLATFORM})
    pico_message("Defaulting platform (PICO_PLATFORM) to '${PICO_PLATFORM}' since not specified.")
else()
    if (NOT PICO_SAVED_PLATFORM)
        pico_expand_pico_platform(PICO_PLATFORM 1)
        pico_message("Defaulting platform (PICO_PLATFORM) to '${PICO_PLATFORM}' based on PICO_BOARD setting.")
    else()
        string(REGEX REPLACE "-.*" "" PICO_PLATFORM_PREFIX ${PICO_PLATFORM})
        string(REGEX REPLACE "-.*" "" PICO_SAVED_PLATFORM_PREFIX ${PICO_SAVED_PLATFORM})
        if (PICO_PLATFORM_PREFIX STREQUAL PICO_SAVED_PLATFORM_PREFIX)
            # the PICO_PLATFORM specified based on the board is compatible based on the one we were
            # already using, so use that
            pico_expand_pico_platform(PICO_SAVED_PLATFORM 0)
            set(PICO_PLATFORM ${PICO_SAVED_PLATFORM})
            message("Pico Platform (PICO_PLATFORM) is '${PICO_PLATFORM}'.")
        else()
            message(FATAL_ERROR "PICO_PLATFORM is specified to be '${PICO_SAVED_PLATFORM}', but PICO_BOARD='${PICO_BOARD}' uses \
                '${PICO_PLATFORM}' which is incompatible. You need to delete the CMake cache or build directory and reconfigure to proceed. \
                The best practice is to use separate build directories for different platforms.")
        endif()
    endif()
endif()
unset(PICO_SAVED_PLATFORM)

if (PICO_PREVIOUS_PLATFORM AND NOT PICO_PREVIOUS_PLATFORM STREQUAL PICO_PLATFORM)
    message(FATAL_ERROR "PICO_PLATFORM has been modified from '${PICO_SAVED_PLATFORM}' to '${PICO_PLATFORM}.\
                You need to delete the CMake cache or build directory and reconfigure to proceed.\
                The best practice is to use separate build directories for different platforms.")
endif()
set(PICO_PLATFORM ${PICO_PLATFORM} CACHE STRING "PICO Build platform (e.g. rp2040, rp2350, rp2350-riscv,  host)" FORCE)
set(PICO_PREVIOUS_PLATFORM ${PICO_PLATFORM} CACHE STRING "Saved PICO Build platform (e.g. rp2040, rp2350, rp2350-riscv,  host)" INTERNAL)

# PICO_CMAKE_CONFIG: PICO_CMAKE_PRELOAD_PLATFORM_FILE, Custom CMake file to use to set up the platform environment, type=string, group=build
set(PICO_CMAKE_PRELOAD_PLATFORM_FILE ${PICO_CMAKE_PRELOAD_PLATFORM_FILE} CACHE INTERNAL "")
if (NOT PICO_CMAKE_PRELOAD_PLATFORM_DIR)
    set(PICO_CMAKE_PRELOAD_PLATFORM_DIR "${CMAKE_CURRENT_LIST_DIR}/preload/platforms")
endif()
set(PICO_CMAKE_PRELOAD_PLATFORM_DIR "${PICO_CMAKE_PRELOAD_PLATFORM_DIR}" CACHE INTERNAL "")

if (NOT PICO_CMAKE_PRELOAD_PLATFORM_FILE)
    set(PICO_CMAKE_PRELOAD_PLATFORM_FILE ${PICO_CMAKE_PRELOAD_PLATFORM_DIR}/${PICO_PLATFORM}.cmake CACHE INTERNAL "")
endif ()

if (NOT EXISTS "${PICO_CMAKE_PRELOAD_PLATFORM_FILE}")
    message(FATAL_ERROR "${PICO_CMAKE_PRELOAD_PLATFORM_FILE} does not exist. \
    Either specify a valid PICO_PLATFORM (or PICO_CMAKE_PRELOAD_PLATFORM_FILE).")
endif ()

include(${PICO_CMAKE_PRELOAD_PLATFORM_FILE})
