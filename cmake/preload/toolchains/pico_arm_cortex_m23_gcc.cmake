set(CMAKE_SYSTEM_PROCESSOR cortex-m23)
set(PICO_DEFAULT_GCC_TRIPLE arm-none-eabi)

set(PICO_COMMON_LANG_FLAGS " -mcpu=cortex-m23 -mthumb -march=armv8-m.base")

if (NOT PICO_NO_CMSE)
    set(PICO_COMMON_LANG_FLAGS "${PICO_COMMON_LANG_FLAGS} -mcmse")
endif()

include(${CMAKE_CURRENT_LIST_DIR}/util/pico_arm_gcc_common.cmake)