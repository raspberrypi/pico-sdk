macro(pico_set_float_implementation TARGET IMPL)
    # ignore
endmacro()

macro(pico_set_double_implementation TARGET IMPL)
    # ignore
endmacro()

macro(pico_set_binary_type TARGET IMPL)
    # ignore
endmacro()

macro(pico_set_boot_stage2 TARGET IMPL)
    # ignore
endmacro()

set(PICO_HOST_DIR "${CMAKE_CURRENT_LIST_DIR}/host" CACHE INTERNAL "")
function(pico_define_boot_stage2 NAME)
    add_executable(${NAME} ${PICO_HOST_DIR}/boot_stage2.c)
endfunction()

function(pico_add_extra_outputs TARGET)
endfunction()

set(PICO_NO_HARDWARE "1" CACHE INTERNAL "")
set(PICO_ON_DEVICE "0" CACHE INTERNAL "")