# PICO_CMAKE_CONFIG: PICO_HARD_FLOAT_ABI, use hard floating point ABI, type=bool, default=0, group=build, docref=cmake-toolchain-config

# note to future self - putting the -mfloat-abi before the other -mcpu/-march/-mfpu cause GCC to
# wrongly emit .fpu softvfp regardless of any other options! go figure!!!
if (PICO_HARD_FLOAT_ABI)
    set(PICO_COMMON_LANG_FLAGS "${PICO_COMMON_LANG_FLAGS} -mfloat-abi=hard")
else()
    set(PICO_COMMON_LANG_FLAGS "${PICO_COMMON_LANG_FLAGS} -mfloat-abi=softfp")
endif()

include(${CMAKE_CURRENT_LIST_DIR}/pico_gcc_common.cmake)
