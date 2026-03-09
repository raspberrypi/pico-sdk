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
set(CMAKE_C_COMPILER   ${PICO_COMPILER_CC} CACHE FILEPATH "C compiler")
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

set(_CLANG_RUNTIMES_DIR "${PICO_COMPILER_DIR}/../lib/clang-runtimes")
cmake_path(NORMAL_PATH _CLANG_RUNTIMES_DIR)
set(PICO_CLIB_ROOT "${_CLANG_RUNTIMES_DIR}")

if(NOT PICO_CLIB  OR  PICO_CLIB STREQUAL "")
    # newlib is primary if no clib specified 
    if(EXISTS "${_CLANG_RUNTIMES_DIR}/newlib")
        set(PICO_CLIB "newlib")
    else()
        set(PICO_CLIB "picolibc")
    endif()
    set(CACHE{PICO_CLIB} TYPE STRING FORCE VALUE ${PICO_CLIB})
endif()

if(PICO_CLIB STREQUAL "newlib")
    if(EXISTS "${_CLANG_RUNTIMES_DIR}/newlib")
        set(PICO_CLIB_ROOT "${_CLANG_RUNTIMES_DIR}/newlib")
    endif()
elseif(PICO_CLIB STREQUAL "llvm_libc")
    if(EXISTS "${_CLANG_RUNTIMES_DIR}/llvmlibc")
        set(PICO_CLIB_ROOT "${_CLANG_RUNTIMES_DIR}/llvmlibc")
    endif()
elseif(PICO_CLIB STREQUAL "picolibc")
    if(EXISTS "${_CLANG_RUNTIMES_DIR}/picolibc")
        set(PICO_CLIB_ROOT "${_CLANG_RUNTIMES_DIR}/picolibc")
    endif()
else()
    message(FATAL_ERROR "PICO_CLIB must be one of newlib, picolib, llvm_libc or empty (but is '${PICO_CLIB}')")
endif()

set(PICO_COMMON_LANG_FLAGS "${PICO_COMMON_LANG_FLAGS} --sysroot ${PICO_CLIB_ROOT}")

if (PICO_CLIB STREQUAL "llvm_libc")
    # TODO: Move -nostartfiles to the appropriate library.
    foreach(TYPE IN ITEMS EXE SHARED MODULE)
        # note --unwindlib=none is only needed on recent compiler/lib versions, however just produces a
        # warning on earlier versions, so not attempting a version check for now
        set(CMAKE_${TYPE}_LINKER_FLAGS_INIT "-nostdlib++ -nostartfiles --unwindlib=none")
    endforeach()
endif()

# at least rp2040 does not work with lld.  For rp2350 it seems to work with lld, but not really sure
set(CMAKE_EXE_LINKER_FLAGS_INIT "${CMAKE_EXE_LINKER_FLAGS_INIT} -fuse-ld=ld -Wl,-z,noexecstack")
if(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
    message(FATAL_ERROR "IPO is not supported with the current configuration of the Pico SDK")
endif()

message(STATUS "Taking '${PICO_CLIB}' from '${PICO_CLIB_ROOT}'")

include(${CMAKE_CURRENT_LIST_DIR}/set_flags.cmake)
