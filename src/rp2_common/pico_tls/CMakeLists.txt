
pico_add_library(pico_tls_none NOFLAG)
target_sources(pico_tls_none INTERFACE ${CMAKE_CURRENT_LIST_DIR}/tls.c)
target_include_directories(pico_tls_none SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
target_compile_definitions(pico_tls_none INTERFACE PICO_TLS_MODE_NONE=1)

pico_add_library(pico_tls_global NOFLAG)
target_sources(pico_tls_global INTERFACE ${CMAKE_CURRENT_LIST_DIR}/tls.c)
target_include_directories(pico_tls_global SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
target_compile_definitions(pico_tls_global INTERFACE PICO_TLS_MODE_GLOBAL=1)

pico_add_library(pico_tls_per_thread NOFLAG)
target_sources(pico_tls_per_thread INTERFACE ${CMAKE_CURRENT_LIST_DIR}/tls.c)
target_include_directories(pico_tls_per_thread SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
target_compile_definitions(pico_tls_per_thread INTERFACE PICO_TLS_MODE_PER_THREAD=1)

set(PICO_DEFAULT_TLS_IMPL pico_tls_per_thread)

pico_add_library(pico_tls NOFLAG)
target_link_libraries(pico_tls INTERFACE
        $<IF:$<BOOL:$<TARGET_PROPERTY:PICO_TARGET_TLS_IMPL>>,$<TARGET_PROPERTY:PICO_TARGET_TLS_IMPL>,${PICO_DEFAULT_TLS_IMPL}>)

macro(pico_set_tls_implementation TARGET IMPL)
    get_target_property(target_type ${TARGET} TYPE)
    if ("EXECUTABLE" STREQUAL "${target_type}")
        set_target_properties(${TARGET} PROPERTIES PICO_TARGET_TLS_IMPL "pico_tls_${IMPL}")
    else()
        message(FATAL_ERROR "tls implementation must be set on executable not library")
    endif()
endmacro()

# Everything below is an optimization that uses a test compile of __thread variable into a static library
# and looks at what symbols it pulls in, which allows us to identify the actual TLS type on Arm
function(pico_get_tls_type OUTPUT_VAR)
    include(CheckCSourceCompiles)

    # 1. Try to compile a snippet using TLS
    set(TLS_CHECK_SOURCE "
            __thread int test_var = 42;
            int tls_check() {
                return test_var;
            }
        ")

    # Create a temporary file for the source
    file(WRITE "${CMAKE_BINARY_DIR}/tls_check.c" "${TLS_CHECK_SOURCE}")

    set(TLS_CHECK_LIBRARY "${CMAKE_BINARY_DIR}/tls_check.a")
    set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
    # Attempt to compile the source file
    try_compile(CAN_COMPILE_TLS
            "${CMAKE_BINARY_DIR}"                     # Directory for temporary build files
            "${CMAKE_BINARY_DIR}/tls_check.c"        # Source file to compile
            COPY_FILE ${TLS_CHECK_LIBRARY}
    )

    if (CAN_COMPILE_TLS)
        execute_process(
                COMMAND ${CMAKE_NM} ${TLS_CHECK_LIBRARY}
                OUTPUT_VARIABLE NM_OUT
        )
        if(NM_OUT MATCHES "__emutls_get_address")
            message(STATUS "Detected emulated TLS (__emutls_get_address)")
            set(TLS_TYPE "emutls" CACHE INTERNAL "TLS Type")
        elseif(NM_OUT MATCHES "__aeabi_read_tp")
            message(STATUS "Detected native TLS (__aeabi_read_tp)")
            set(TLS_TYPE "arm-aeabi" CACHE INTERNAL "TLS Type")
        else()
            if (NOT PICO_RISCV)
                message(WARNING "Could not identify TLS mechanism")
            endif()
            set(TLS_TYPE "unknown" CACHE INTERNAL "TLS Type")
        endif()
    else()
        message(STATUS "Compiler does not support __thread keyword.")
        set(TLS_TYPE "unsupported" CACHE INTERNAL "TLS Type")
    endif()
    set(${OUTPUT_VAR} ${TLS_TYPE} PARENT_SCOPE)
endfunction()

if (NOT PICO_TLS_TYPE)
    pico_get_tls_type(_TLS_TYPE)
    set(PICO_TLS_TYPE "${_TLS_TYPE}" CACHE INTERNAL "")
    unset(_TLS_TYPE)
endif()

if (PICO_TLS_TYPE STREQUAL "emutls")
    target_compile_definitions(pico_tls INTERFACE PICO_TLS_EMUTLS_CONFIRMED=1)
elseif(PICO_TLS_TYPE STREQUAL "arm-aeabi")
    target_compile_definitions(pico_tls INTERFACE PICO_TLS_THREAD_POINTER_ARM_AEABI_CONFIRMED=1)
endif()