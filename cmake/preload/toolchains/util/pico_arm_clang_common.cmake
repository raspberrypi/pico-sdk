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

if (NOT PICO_COMPILER_SYSROOT)
    # Note we explicitly look for the correct libraries, because we don't necessarily trust multilib (even
    # in newer Arm LLVM ATfE to correctly pick the right one
    foreach(PICO_CLANG_RUNTIME IN LISTS PICO_CLANG_RUNTIMES)
        # LLVM embedded-toolchain for ARM style
        find_path(PICO_COMPILER_SYSROOT NAMES lib
                HINTS
                ${PICO_COMPILER_DIR}/../lib/clang-runtimes/arm-none-eabi/${PICO_CLANG_RUNTIME}
                ${PICO_COMPILER_DIR}/../lib/clang-runtimes/${PICO_CLANG_RUNTIME}
        )

        if (PICO_COMPILER_SYSROOT)
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
endif()

# moving this here as a reminder from pico_standard_link; it was commented out theee, but if ever needed,
# it belongs here as part of LINKER_FLAGS_INIT
#target_link_options(pico_standard_link INTERFACE "LINKER:-fuse-ld=lld")

if (PICO_CLIB STREQUAL "llvm_libc")
    # TODO: Remove -nostdlib++ once we include libc++ in the toolchain.
    # TODO: Move -nostartfiles to the appropriate library.
    foreach(TYPE IN ITEMS EXE SHARED MODULE)
        # note --unwindlib=none is only needed on recent compiler/lib versions, however just produces a
        # warning on earlier versions, so not attempting a version check for now
        set(CMAKE_${TYPE}_LINKER_FLAGS_INIT "-nostdlib++ -nostartfiles --unwindlib=none")
    endforeach()
else()
    if (NOT PICO_COMPILER_SYSROOT)
        message(FATAL_ERROR "Could not find an llvm runtime")
    endif()
    set(PICO_COMMON_LANG_FLAGS "${PICO_COMMON_LANG_FLAGS} --sysroot ${PICO_COMPILER_SYSROOT}")

    find_path(_CLANG_HEADERS_DIR NAMES include/stdio.h
            HINTS
            ${PICO_COMPILER_SYSROOT}
            ${PICO_COMPILER_SYSROOT}/.. # new style LLVM ATfE (21+)
    )
    if (NOT _CLANG_HEADERS_DIR)
        message(FATAL_ERROR "Could not find llvm headers (searched ${PICO_COMPILER_SYSROOT}/include and ${PICO_COMPILER_SYSROOT}/../include)")
    else()
        if (NOT _CLANG_HEADERS_DIR STREQUAL PICO_COMPILER_SYSROOT)
            # LLVM ATfE 21+ moves the headers out of the multilib so we need to explicitly add C++/C headers (in that order)
            set(PICO_COMMON_LANG_FLAGS "${PICO_COMMON_LANG_FLAGS} -isystem ${_CLANG_HEADERS_DIR}/include/c++/v1 -isystem ${_CLANG_HEADERS_DIR}/include")
        endif()
    endif()
endif()
include(${CMAKE_CURRENT_LIST_DIR}/set_flags.cmake)
