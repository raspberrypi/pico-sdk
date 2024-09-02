set(CMAKE_SYSTEM_PROCESSOR cortex-m0plus)

set(PICO_DEFAULT_GCC_TRIPLE arm-none-eabi)
# on ARM -mcpu should not be mixed with -march
set(PICO_COMMON_LANG_FLAGS " -mcpu=cortex-m0plus -mthumb")

include(${CMAKE_CURRENT_LIST_DIR}/util/pico_arm_gcc_common.cmake)