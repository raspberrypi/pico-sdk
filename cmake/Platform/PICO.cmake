# this is included because toolchain file sets SYSTEM_NAME=PICO

set_property(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS FALSE)
set(CMAKE_EXECUTABLE_SUFFIX .elf)

# include paths to find installed tools
if(CMAKE_HOST_WIN32)
    include(Platform/WindowsPaths)
else()
    include(Platform/UnixPaths)
endif()
