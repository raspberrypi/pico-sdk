# PICO_BUILD_DEFINE: PICO_SDK_VERSION_MAJOR, SDK major version number, type=int, default=Current SDK major version, group=pico_base
# PICO_CMAKE_CONFIG: PICO_SDK_VERSION_MAJOR, SDK major version number, type=int, default=Current SDK major version, group=pico_base
set(PICO_SDK_VERSION_MAJOR 2)
# PICO_BUILD_DEFINE: PICO_SDK_VERSION_MINOR, SDK minor version number, type=int, default=Current SDK minor version, group=pico_base
# PICO_CMAKE_CONFIG: PICO_SDK_VERSION_MINOR, SDK minor version number, type=int, default=Current SDK minor version, group=pico_base
set(PICO_SDK_VERSION_MINOR 1)
# PICO_BUILD_DEFINE: PICO_SDK_VERSION_REVISION, SDK version revision, type=int, default=Current SDK revision, group=pico_base
# PICO_CMAKE_CONFIG: PICO_SDK_VERSION_REVISION, SDK version revision, type=int, default=Current SDK revision, group=pico_base
set(PICO_SDK_VERSION_REVISION 0)
# PICO_BUILD_DEFINE: PICO_SDK_VERSION_PRE_RELEASE_ID, Optional SDK pre-release version identifier, default=Current SDK pre-release identifier, type=string, group=pico_base
# PICO_CMAKE_CONFIG: PICO_SDK_VERSION_PRE_RELEASE_ID, Optional SDK pre-release version identifier, default=Current SDK pre-release identifier, type=string, group=pico_base
#set(PICO_SDK_VERSION_PRE_RELEASE_ID develop)

# PICO_BUILD_DEFINE: PICO_SDK_VERSION_STRING, SDK version string, type=string, default=Current SDK version string, group=pico_base
# PICO_CMAKE_CONFIG: PICO_SDK_VERSION_STRING, SDK version string, type=string, default=Current SDK version string, group=pico_base
set(PICO_SDK_VERSION_STRING "${PICO_SDK_VERSION_MAJOR}.${PICO_SDK_VERSION_MINOR}.${PICO_SDK_VERSION_REVISION}")

if (PICO_SDK_VERSION_PRE_RELEASE_ID)
    set(PICO_SDK_VERSION_STRING "${PICO_SDK_VERSION_STRING}-${PICO_SDK_VERSION_PRE_RELEASE_ID}")
endif()
