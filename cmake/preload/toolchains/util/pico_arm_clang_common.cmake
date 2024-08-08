include(${CMAKE_CURRENT_LIST_DIR}/find_compiler.cmake)

# include our Platform/PICO.cmake
set(CMAKE_SYSTEM_NAME PICO)

# Find Clang
pico_find_compiler(PICO_COMPILER_CC clang)
pico_find_compiler(PICO_COMPILER_CXX clang++)
set(PICO_COMPILER_ASM "${PICO_COMPILER_CC}" CACHE INTERNAL "")
pico_find_compiler(PICO_OBJCOPY llvm-objcopy)
pico_find_compiler(PICO_OBJDUMP llvm-objdump)

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

# Oz is preferred for Clang (verses CMake default -Os) see also https://gitlab.kitware.com/cmake/cmake/-/issues/22458
foreach(LANG IN ITEMS C CXX ASM)
    set(CMAKE_${LANG}_FLAGS_MINSIZEREL_INIT "-Oz -DNDEBUG")
endforeach()

list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES PICO_CLIB)

foreach(PICO_CLANG_RUNTIME IN LISTS PICO_CLANG_RUNTIMES)
    # LLVM embedded-toolchain for ARM style
    find_path(PICO_COMPILER_SYSROOT NAMES include/stdio.h
            HINTS
            ${PICO_COMPILER_DIR}/../lib/clang-runtimes/arm-none-eabi/${PICO_CLANG_RUNTIME}
            ${PICO_COMPILER_DIR}/../lib/clang-runtimes/${PICO_CLANG_RUNTIME}
    )

    if (PICO_COMPILER_SYSROOT)
        if (NOT PICO_CLIB)
            # this is a bit of a hack; to try to autodetect the C library used:
            # `picolibc.h` seems to exist on the newer versions of LLVM embedded toolchain for ARM using picolibc whereas
            # `newlib.h` appears in all versions, so isn't very useful
            if (EXISTS "${PICO_COMPILER_SYSROOT}/include/picolibc.h")
                message("Setting default C library to picolibc as LLVM appears to be using it")
                set(PICO_CLIB "picolibc" CACHE INTERNAL "")
            endif()
        endif()
        break()
    endif()
    # llvm_libc style
    find_path(PICO_COMPILER_SYSROOT NAMES stdio.h
            HINTS
            ${PICO_COMPILER_DIR}/../include/${PICO_CLANG_RUNTIME}
    )
    if (PICO_COMPILER_SYSROOT)
        if (NOT PICO_CLIB)
            message("Setting default C library to llvm_libc as LLVM appears to be using it")
            set(PICO_CLIB "llvm_libc" CACHE INTERNAL "")
        endif()
        break()
    endif()
endforeach()

# moving this here as a reminder from pico_standard_link; it was commented out theee, but if ever needed,
# it belongs here as part of LINKER_FLAGS_INIT
#target_link_options(pico_standard_link INTERFACE "LINKER:-fuse-ld=lld")

if (PICO_CLIB STREQUAL "llvm_libc")
    # TODO: Remove -nostdlib++ once we include libc++ in the toolchain.
    # TODO: Move -nostartfiles to the appropriate library.
    foreach(TYPE IN ITEMS EXE SHARED MODULE)
        set(CMAKE_${TYPE}_LINKER_FLAGS_INIT "-nostdlib++ -nostartfiles")
    endforeach()
else()
    if (NOT PICO_COMPILER_SYSROOT)
        message(FATAL_ERROR "Could not find an llvm runtime for '${PICO_CLANG_RUNTIME}'")
    endif()

    set(PICO_COMMON_LANG_FLAGS "${PICO_COMMON_LANG_FLAGS} --sysroot ${PICO_COMPILER_SYSROOT}")
endif()
include(${CMAKE_CURRENT_LIST_DIR}/set_flags.cmake)
