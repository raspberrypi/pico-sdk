pico_add_library(pico_runtime)

target_sources(pico_runtime INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/runtime.c
)

target_include_directories(pico_runtime_headers SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

pico_mirrored_target_link_libraries(pico_runtime INTERFACE
        pico_base
        pico_runtime_init
)

if (TARGET hardware_gpio_headers)
    target_link_libraries(pico_runtime INTERFACE hardware_gpio_headers) # to determine if we should init GPIO copro
endif()
if (TARGET hardware_riscv_headers)
    target_link_libraries(pico_runtime INTERFACE hardware_riscv_headers)
endif()

set(PICO_RUNTIME_LIBRARIES
        hardware_uart
        pico_bit_ops
        pico_divider
        pico_double
        pico_int64_ops
        pico_float
        pico_malloc
        pico_mem_ops
        pico_atomic
        pico_cxx_options
        pico_standard_binary_info
        pico_standard_link
        pico_sync
        pico_printf
        pico_crt0
        pico_clib_interface
        pico_stdio
)

foreach(LIB IN LISTS PICO_RUNTIME_LIBRARIES)
    if (TARGET ${LIB})
        pico_mirrored_target_link_libraries(pico_runtime INTERFACE ${LIB})
    endif()
endforeach()

# todo is this correct/needed?
if (PICO_C_COMPILER_IS_GNU)
    target_link_options(pico_runtime INTERFACE "--specs=nosys.specs")
elseif (PICO_C_COMPILER_IS_CLANG)
   # target_link_options(pico_runtime INTERFACE "-nostdlib")
endif()

# pico_minimize_runtime((INCLUDE ...) (EXCLUDE ...))
#
# INCLUDE/EXCLUDE can contain any of the following (all defaulting to not included)
#
# DEFAULT_ALARM_POOL    - default alarm pool setup
# PRINTF                - full printf support
# PRINTF_MINIMAL        - printf support without the following
# PRINTF_FLOAT          - to control float support if printf is enabled
# PRINTF_EXPONENTIAL
# PRINTF_LONG_LONG
# PRINTF_PTRDIFF_T
# FLOAT                 - support for single-precision floating point
# DOUBLE                - support for double-precision floating point
# FPGA_CHECK            - checks for FPGA which allows Raspberry Pi to run your binary on FPGA
# PANIC                 - default panic impl which brings in stdio
# AUTO_INIT_MUTEX       - auto init mutexes; without this you get no printf mutex either               -
function(pico_minimize_runtime TARGET)
    set(ALL_ITEMS
            DEFAULT_ALARM_POOL
            PRINTF
            FLOAT
            DOUBLE
            FPGA_CHECK
            PANIC
            AUTO_INIT_MUTEX)
    cmake_parse_arguments(RUNTIME "" ""
            "INCLUDE;EXCLUDE" ${ARGN} )
    foreach (INCL_EXCL IN ITEMS INCLUDE EXCLUDE)
        if (INCL_EXCL STREQUAL "INCLUDE")
            set(VAL 1)
        else()
            set(VAL 0)
        endif()
        foreach(VAR IN LISTS RUNTIME_${INCL_EXCL})
            if (VAR STREQUAL "ALL")
                foreach(ITEM IN LISTS ALL_ITEMS)
                    set(RUNTIME_INCLUDE_${ITEM} ${VAL})
                endforeach ()
            else()
                set(RUNTIME_INCLUDE_${VAR} ${VAL})
            endif()
        endforeach ()
    endforeach ()
    if (NOT RUNTIME_INCLUDE_DEFAULT_ALARM_POOL)
        target_compile_definitions(${TARGET} PRIVATE PICO_TIME_DEFAULT_ALARM_POOL_DISABLED=1)
    endif()
    if (NOT RUNTIME_INCLUDE_FLOAT)
        pico_set_float_implementation(${TARGET} none)
    endif()
    if (NOT RUNTIME_INCLUDE_DOUBLE)
        pico_set_double_implementation(${TARGET} none)
    endif()
    if (NOT RUNTIME_INCLUDE_AUTO_INIT_MUTEX)
        target_compile_definitions(${TARGET} PRIVATE PICO_RUNTIME_SKIP_INIT_MUTEX=1)
    endif()

    if (RUNTIME_INCLUDE_PRINTF)
        if (NOT RUNTIME_INCLUDE_PRINTF_MINIMAL)
            set( RUNTIME_INCLUDE_PRINTF_FLOAT 1)
            set( RUNTIME_INCLUDE_PRINTF_EXPONENTIAL 1)
            set( RUNTIME_INCLUDE_PRINTF_LONG_LONG 1)
            set( RUNTIME_INCLUDE_PRINTF_PTRDIFF_T 1)
        endif()
        if (NOT RUNTIME_INCLUDE_PRINTF_FLOAT)
            target_compile_definitions(${TARGET} PRIVATE PICO_PRINTF_SUPPORT_FLOAT=0)
        endif()
        if (NOT RUNTIME_INCLUDE_PRINTF_EXPONENTIAL)
            target_compile_definitions(${TARGET} PRIVATE PICO_PRINTF_SUPPORT_EXPONENTIAL=0)
        endif()
        if (NOT RUNTIME_INCLUDE_PRINTF_LONG_LONG)
            target_compile_definitions(${TARGET} PRIVATE PICO_PRINTF_SUPPORT_LONG_LONG=0)
        endif()
        if (NOT RUNTIME_INCLUDE_PRINTF_PTRDIFF_T)
            target_compile_definitions(${TARGET} PRIVATE PICO_PRINTF_SUPPORT_PTRDIFF_T=0)
        endif()
    else()
        pico_set_printf_implementation(${TARGET} none)
    endif()

    if (NOT RUNTIME_INCLUDE_PANIC)
        target_compile_definitions(${TARGET} PRIVATE
                PICO_PANIC_FUNCTION= #the default uses puts
        )
    endif()
    if (NOT RUNTIME_INCLUDE_AUTO_INIT_MUTEX)
        target_compile_definitions(${TARGET} PRIVATE
                PICO_RUNTIME_NO_INIT_MUTEX=1
                PICO_STDOUT_MUTEX=0 # currently pulls in mutex code otherwise
        )
    endif()
    if (NOT RUNTIME_INCLUDE_FPGA_CHECK)
        target_compile_definitions(${TARGET} PRIVATE PICO_NO_FPGA_CHECK=1)
    endif()
endfunction()