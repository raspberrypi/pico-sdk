set(CMAKE_SYSTEM_PROCESSOR cortex-m33)
set(PICO_DEFAULT_GCC_TRIPLE arm-none-eabi)

set(PICO_COMMON_LANG_FLAGS " -mcpu=cortex-m33 -mthumb -march=armv8-m.main+fp+dsp")
set(PICO_COMMON_LANG_FLAGS "${PICO_COMMON_LANG_FLAGS} -mfloat-abi=softfp")
if (NOT PICO_NO_CMSE)
    set(PICO_COMMON_LANG_FLAGS "${PICO_COMMON_LANG_FLAGS} -mcmse")
endif()

include(${CMAKE_CURRENT_LIST_DIR}/util/pico_arm_gcc_common.cmake)