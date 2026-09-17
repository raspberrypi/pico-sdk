
pico_add_library(pico_thread_local_none NOFLAG)
target_sources(pico_thread_local_none INTERFACE ${CMAKE_CURRENT_LIST_DIR}/thread_local.c)
target_include_directories(pico_thread_local_none SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
target_compile_definitions(pico_thread_local_none INTERFACE PICO_THREAD_LOCAL_MODE_NONE=1)

pico_add_library(pico_thread_local_global NOFLAG)
target_sources(pico_thread_local_global INTERFACE ${CMAKE_CURRENT_LIST_DIR}/thread_local.c)
target_include_directories(pico_thread_local_global SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
target_compile_definitions(pico_thread_local_global INTERFACE PICO_THREAD_LOCAL_MODE_GLOBAL=1)

pico_add_library(pico_thread_local_per_thread NOFLAG)
target_sources(pico_thread_local_per_thread INTERFACE ${CMAKE_CURRENT_LIST_DIR}/thread_local.c)
target_include_directories(pico_thread_local_per_thread SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
target_compile_definitions(pico_thread_local_per_thread INTERFACE PICO_THREAD_LOCAL_MODE_PER_THREAD=1)

# PICO_CMAKE_CONFIG: PICO_DEFAULT_THREAD_LOCAL_IMPL, The default implementation for pico_thread_local to link; 'per_thread' provides per thread locals while 'global' provides shared global values and 'none' makes thread local behavior undefined to save space in the binary, type=string, default=per_thread, group=pico_thread_local
pico_fixup_default_impl(PICO_DEFAULT_THREAD_LOCAL_IMPL pico_thread_local per_thread)

pico_add_library(pico_thread_local NOFLAG)
target_link_libraries(pico_thread_local INTERFACE
        $<IF:$<BOOL:$<TARGET_PROPERTY:PICO_TARGET_THREAD_LOCAL_IMPL>>,$<TARGET_PROPERTY:PICO_TARGET_THREAD_LOCAL_IMPL>,${PICO_DEFAULT_THREAD_LOCAL_IMPL}>)

macro(pico_set_thread_local_implementation TARGET IMPL)
    get_target_property(target_type ${TARGET} TYPE)
    if ("EXECUTABLE" STREQUAL "${target_type}")
        set_target_properties(${TARGET} PROPERTIES PICO_TARGET_THREAD_LOCAL_IMPL "pico_thread_local_${IMPL}")
    else()
        message(FATAL_ERROR "tls implementation must be set on executable not library")
    endif()
endmacro()

# Everything below is an optimization that uses a test compile of __thread variable into a static library
# and looks at what symbols it pulls in, which allows us to identify the actual TLS type on Arm
function(pico_detect_tls_type OUTPUT_VAR)
    include(CheckCSourceCompiles)

    # 1. Try to compile a snippet using TLS
    set(TLS_CHECK_CODE "
            __thread int test_var = 42;
            int tls_check() {
                return test_var;
            }
        ")

    # Create a temporary file for the source
    set(TLS_CHECK_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/tls_check.c")
    file(WRITE "${TLS_CHECK_SOURCE}" "${TLS_CHECK_CODE}")

    set(TLS_CHECK_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/tls_check.a")
    set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
    # Attempt to compile the source file
    try_compile(CAN_COMPILE_TLS
            "${CMAKE_CURRENT_BINARY_DIR}"              # Directory for temporary build files
            "${TLS_CHECK_SOURCE}"                      # Source file to compile
            COPY_FILE "${TLS_CHECK_LIBRARY}"
    )

    if (CAN_COMPILE_TLS)
        execute_process(
                COMMAND ${CMAKE_NM} "${TLS_CHECK_LIBRARY}"
                OUTPUT_VARIABLE NM_OUT
        )
        if(NM_OUT MATCHES "__emutls_get_address")
            message(STATUS "Detected emulated TLS (__emutls_get_address)")
            set(TLS_TYPE "emutls")
        elseif(NM_OUT MATCHES "__aeabi_read_tp")
            message(STATUS "Detected native TLS (__aeabi_read_tp)")
            set(TLS_TYPE "arm-aeabi")
        else()
            if (NOT PICO_RISCV)
                message(WARNING "Could not identify TLS mechanism")
            endif()
            set(TLS_TYPE "unknown")
        endif()
    else()
        message(STATUS "Compiler does not support __thread keyword.")
        set(TLS_TYPE "unsupported")
    endif()
    set(${OUTPUT_VAR} ${TLS_TYPE} PARENT_SCOPE)
    file(REMOVE "${TLS_CHECK_SOURCE}" "${TLS_CHECK_LIBRARY}")
endfunction()

if (NOT PICO_THREAD_LOCAL_TYPE)
    pico_detect_tls_type(_TLS_TYPE)
    set(PICO_THREAD_LOCAL_TYPE "${_TLS_TYPE}" CACHE INTERNAL "")
    unset(_TLS_TYPE)
endif()

if (PICO_THREAD_LOCAL_TYPE STREQUAL "emutls")
    target_compile_definitions(pico_thread_local INTERFACE PICO_THREAD_LOCAL_EMUTLS_CONFIRMED=1)
elseif(PICO_THREAD_LOCAL_TYPE STREQUAL "arm-aeabi")
    target_compile_definitions(pico_thread_local INTERFACE PICO_THREAD_LOCAL_THREAD_POINTER_ARM_AEABI_CONFIRMED=1)
endif()