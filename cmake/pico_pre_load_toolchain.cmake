# PICO_CMAKE_CONFIG: PICO_TOOLCHAIN_PATH, Path to search for compiler, type=string, default=none (i.e. search system paths), group=build, docref=cmake-toolchain-config
set(PICO_TOOLCHAIN_PATH "${PICO_TOOLCHAIN_PATH}" CACHE INTERNAL "")

# Set a default build type if none was specified
set(default_build_type "Release")

list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES CMAKE_PREFIX_PATH)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Defaulting build type to '${default_build_type}' since not specified.")
    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build, options are: 'Debug', 'Release', 'MinSizeRel', 'RelWithDebInfo'." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
            "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Default")
    error("Default build type is NOT supported")
endif()

if (NOT (DEFINED PICO_COMPILER OR DEFINED CMAKE_TOOLCHAIN_FILE))
    if (DEFINED PICO_DEFAULT_COMPILER)
        pico_message("Defaulting compiler (PICO_COMPILER) to '${PICO_DEFAULT_COMPILER}' since not specified.")
        set(PICO_COMPILER ${PICO_DEFAULT_COMPILER})
    endif()
endif ()

# PICO_CMAKE_CONFIG: PICO_COMPILER, Specifies the compiler family to use, type=string, group=build, default=PICO_DEFAULT_COMPILER which is set based on PICO_PLATFORM, docref=cmake-toolchain-config
# If PICO_COMPILER is specified, set toolchain file to ${PICO_COMPILER}.cmake.
if (DEFINED PICO_COMPILER)
    # maintain backwards compatibility with RP2040 SDK compilers
    set(ORIG_PICO_COMPILER "${PICO_COMPILER}")
    if (PICO_COMPILER STREQUAL "pico_arm_gcc")
        if (PICO_PLATFORM STREQUAL "rp2040")
            set(PICO_COMPILER "pico_arm_cortex_m0plus_gcc")
        elseif(PICO_PLATFORM STREQUAL "rp2350")
            set(PICO_COMPILER "pico_arm_cortex_m33_gcc")
        endif()
    elseif(PICO_COMPILER STREQUAL "pico_arm_clang")
        if (PICO_PLATFORM STREQUAL "rp2040")
            set(PICO_COMPILER "pico_arm_cortex_m0plus_clang")
        elseif(PICO_PLATFORM STREQUAL "rp2350-arm-s")
            set(PICO_COMPILER "pico_arm_cortex_m33_clang")
        elseif(PICO_PLATFORM STREQUAL "rp2350-arm-ns")
            set(PICO_COMPILER "pico_arm_cortex_m33_clang")
        endif()
    endif()
    if (NOT PICO_COMPILER STREQUAL ORIG_PICO_COMPILER)
        message("Accepting PICO_COMPILER value '${ORIG_PICO_COMPILER}' for compatibility, but using '${PICO_COMPILER}' instead")
    endif()
    if (NOT DEFINED PICO_TOOLCHAIN_DIR)
        set(PICO_TOOLCHAIN_DIR "${CMAKE_CURRENT_LIST_DIR}/preload/toolchains")
    endif()
    set(toolchain_file "${PICO_TOOLCHAIN_DIR}/${PICO_COMPILER}.cmake")
    if (EXISTS "${toolchain_file}")
        set(CMAKE_TOOLCHAIN_FILE "${toolchain_file}")
    else ()
        # todo improve message
        message(FATAL_ERROR "Toolchain file \"${PICO_COMPILER}.cmake\" does not exist, please\
            select one from \"cmake/toolchains\" folder.")
    endif ()
    message("Configuring toolchain based on PICO_COMPILER '${PICO_COMPILER}'")
endif ()

if (PICO_PREVIOUS_CMAKE_TOOLCHAIN_FILE)
    if (NOT "${PICO_PREVIOUS_CMAKE_TOOLCHAIN_FILE}" STREQUAL "${CMAKE_TOOLCHAIN_FILE}")
        message(FATAL_ERROR "CMAKE_TOOLCHAIN_FILE was previously defined to ${PICO_PREVIOUS_CMAKE_TOOLCHAIN_FILE}, and now\
                is being changed to ${CMAKE_TOOLCHAIN_FILE}. You\
                need to delete the CMake cache and reconfigure if you want to switch compiler.\
                The best practice is to use separate build directories for different platforms or compilers.")
    endif ()
endif ()

set(PICO_PREVIOUS_CMAKE_TOOLCHAIN_FILE ${CMAKE_TOOLCHAIN_FILE} CACHE INTERNAL "Saved CMAKE_TOOLCHAIN_FILE" FORCE)
unset(PICO_COMPILER CACHE)

