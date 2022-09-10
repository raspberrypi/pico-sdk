if (NOT TARGET pico_tls)
  # library to be depended on - we make this depend on particular implementations using per target generator expressions
  pico_add_impl_library(pico_tls)

  # no custom implementation; falls thru to compiler
  pico_add_impl_library(pico_tls_compiler)

  # add alias "default" which is just core_thread.
  add_library(pico_tls_default INTERFACE)
  target_link_libraries(pico_tls_default INTERFACE pico_tls_core_thread)

  set(PICO_DEFAULT_TLS_IMPL pico_tls_default)

  target_link_libraries(pico_tls INTERFACE
          $<IF:$<BOOL:$<TARGET_PROPERTY:PICO_TARGET_TLS_IMPL>>,$<TARGET_PROPERTY:PICO_TARGET_TLS_IMPL>,${PICO_DEFAULT_TLS_IMPL}>)

  add_library(pico_tls_core_thread_explicit INTERFACE)
  target_sources(pico_tls_core_thread_explicit INTERFACE
          ${CMAKE_CURRENT_LIST_DIR}/tls.c
          ${CMAKE_CURRENT_LIST_DIR}/tls.S
          )

  pico_add_impl_library(pico_tls_core_thread)

  target_link_libraries(pico_tls_core_thread INTERFACE pico_tls_core_thread_explicit)

  pico_wrap_function(pico_tls_core_thread __emutls_get_address)

  # Call this to substitute an alternate implementation, e.g. if using an RTOS.
  macro(pico_set_tls_implementation TARGET IMPL)
      get_target_property(target_type ${TARGET} TYPE)
      if ("EXECUTABLE" STREQUAL "${target_type}")
          set_target_properties(${TARGET} PROPERTIES PICO_TARGET_TLS_IMPL "pico_tls_${IMPL}")
      else()
          message(FATAL_ERROR "tls implementation must be set on executable not library")
      endif()
  endmacro()
endif()
