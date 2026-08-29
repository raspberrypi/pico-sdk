# PICO_CMAKE_CONFIG: PICO_HARD_FLOAT_ABI, use hard floating point ABI, type=bool, default=0, group=build, docref=cmake-toolchain-config
# PICO_CMAKE_CONFIG: PICO_NO_HARD_FLOAT_ABI_WARNING, disable warning when PICO_HARD_FLOAT_ABI is set when not supported, type=bool, default=0, group=build, docref=cmake-toolchain-config

# note to future self - putting the -mfloat-abi before the other -mcpu/-march/-mfpu cause GCC to
# wrongly emit .fpu softvfp regardless of any other options! go figure!!!
if(CMAKE_SYSTEM_PROCESSOR STREQUAL cortex-m0plus)
    if (PICO_HARD_FLOAT_ABI AND NOT DEFINED PICO_NO_HARD_FLOAT_ABI_WARNING AND
            NOT DEFINED ENV{PICO_NO_HARD_FLOAT_ABI_WARNING})
        set(ENV{PICO_NO_HARD_FLOAT_ABI_WARNING} 1)
        message(WARNING "PICO_HARD_FLOAT_ABI=1 is ignored for RP2040")
    endif()
    # older GCC dont accept softfp for M0+ and end up picking wrong libraries in multilib (also hard is not supported at all)
    set(PICO_COMMON_LANG_FLAGS "${PICO_COMMON_LANG_FLAGS} -mfloat-abi=soft")
elseif (PICO_HARD_FLOAT_ABI)
    set(PICO_COMMON_LANG_FLAGS "${PICO_COMMON_LANG_FLAGS} -mfloat-abi=hard")
else()
    set(PICO_COMMON_LANG_FLAGS "${PICO_COMMON_LANG_FLAGS} -mfloat-abi=softfp")
endif()

include(${CMAKE_CURRENT_LIST_DIR}/pico_gcc_common.cmake)
