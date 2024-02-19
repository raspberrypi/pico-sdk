if (NOT TARGET pico_tone)
    pico_add_library(pico_tone)

    target_include_directories(pico_tone_headers INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/include)

    target_sources(pico_tone INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/tone.c)

    target_link_libraries(pico_tone INTERFACE hardware_pwm)
endif()
