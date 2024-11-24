include(${CMAKE_CURRENT_LIST_DIR}/find_compiler.cmake)

# include our Platform/PICO.cmake
set(CMAKE_SYSTEM_NAME PICO)

# PICO_CMAKE_CONFIG: PICO_GCC_TRIPLE, List of GCC_TRIPLES -- usually only one -- to try when searching for a compiler. This may be specified the user environment, type=int, default=PICO_DEFAULT_GCC_TRIPLE which is set based on PICO_COMPILER, group=pico_base, docref=cmake-toolchain-config
if (NOT PICO_GCC_TRIPLE)
    if (DEFINED ENV{_SAVED_PICO_GCC_TRIPLE})
        # saved within the same cmake invocation
        set(PICO_GCC_TRIPLE $ENV{_SAVED_PICO_GCC_TRIPLE})
    elseif (DEFINED ENV{PICO_GCC_TRIPLE})
        set(PICO_GCC_TRIPLE $ENV{PICO_GCC_TRIPLE})
        message("Initializing PICO_GCC_TRIPLE from environment ('${PICO_GCC_TRIPLE}')")
    elseif(DEFINED PICO_DEFAULT_GCC_TRIPLE)
        message("Defaulting PICO_GCC_TRIPLE to '${PICO_DEFAULT_GCC_TRIPLE}'")
        set(PICO_GCC_TRIPLE ${PICO_DEFAULT_GCC_TRIPLE})
    else()
        message(FATAL_ERROR "PICO_DEFAULT_GCC_TRIPLE is not defined")
    endif()
endif()
set(PICO_GCC_TRIPLE "${PICO_GCC_TRIPLE}" CACHE INTERNAL "")
set(ENV{_SAVED_PICO_GCC_TRIPLE} "${PICO_GCC_TRIPLE}")

# Find GCC
pico_find_compiler_with_triples(PICO_COMPILER_CC "${PICO_GCC_TRIPLE}" gcc)
pico_find_compiler_with_triples(PICO_COMPILER_CXX "${PICO_GCC_TRIPLE}" g++)
set(PICO_COMPILER_ASM "${PICO_COMPILER_CC}" CACHE INTERNAL "")
pico_find_compiler_with_triples(PICO_OBJCOPY "${PICO_GCC_TRIPLE}" objcopy)
pico_find_compiler_with_triples(PICO_OBJDUMP "${PICO_GCC_TRIPLE}" objdump)

# Specify the cross compiler.
set(CMAKE_C_COMPILER ${PICO_COMPILER_CC} CACHE FILEPATH "C compiler")
set(CMAKE_CXX_COMPILER ${PICO_COMPILER_CXX} CACHE FILEPATH "C++ compiler")
set(CMAKE_ASM_COMPILER ${PICO_COMPILER_ASM} CACHE FILEPATH "ASM compiler")

# workaround for projects that don't enable ASM
set(CMAKE_ASM_COMPILE_OBJECT "<CMAKE_ASM_COMPILER> <DEFINES> <INCLUDES> <FLAGS> -o <OBJECT>   -c <SOURCE>")
set(CMAKE_INCLUDE_FLAG_ASM "-I")

set(CMAKE_OBJCOPY ${PICO_OBJCOPY} CACHE FILEPATH "")
set(CMAKE_OBJDUMP ${PICO_OBJDUMP} CACHE FILEPATH "")

foreach(LANG IN ITEMS C CXX ASM)
    set(CMAKE_${LANG}_OUTPUT_EXTENSION .o)
endforeach()

# Add target system root to cmake find path.
get_filename_component(PICO_COMPILER_DIR "${PICO_COMPILER_CC}" DIRECTORY)
get_filename_component(CMAKE_FIND_ROOT_PATH "${PICO_COMPILER_DIR}" DIRECTORY)

# Look for includes and libraries only in the target system prefix.
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

include(${CMAKE_CURRENT_LIST_DIR}/set_flags.cmake)
