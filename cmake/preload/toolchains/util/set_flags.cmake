option(PICO_DEOPTIMIZED_DEBUG "Build debug builds with -O0" 0)
option(PICO_DEBUG_INFO_IN_RELEASE "Include debug info in release builds" 1)

get_property(IS_IN_TRY_COMPILE GLOBAL PROPERTY IN_TRY_COMPILE)
foreach(LANG IN ITEMS C CXX ASM)
    set(CMAKE_${LANG}_FLAGS_INIT "${PICO_COMMON_LANG_FLAGS}")
    unset(CMAKE_${LANG}_FLAGS_DEBUG CACHE)
    if (PICO_DEOPTIMIZED_DEBUG)
        set(CMAKE_${LANG}_FLAGS_DEBUG_INIT "-O0")
    else()
        set(CMAKE_${LANG}_FLAGS_DEBUG_INIT "-Og")
    endif()
    if (PICO_DEBUG_INFO_IN_RELEASE)
        set(CMAKE_${LANG}_FLAGS_RELEASE_INIT "-g")
        set(CMAKE_${LANG}_FLAGS_MINSIZEREL_INIT "-g")
    endif()
    set(CMAKE_${LANG}_LINK_FLAGS "-Wl,--build-id=none")

    # try_compile is where the feature testing is done, and at that point,
    # pico_standard_link is not ready to be linked in to provide essential
    # functions like _exit. So pass -nostdlib so it doesn't link in an exit()
    # function at all.
    if(IS_IN_TRY_COMPILE)
        set(CMAKE_${LANG}_LINK_FLAGS "${CMAKE_${LANG}_LINK_FLAGS} -nostdlib")
    endif()
endforeach()
