# Golden-output tests: assemble a fixture with the freshly built pioasm and
# compare the result against a checked-in expected file. Configure with
# -DPIOASM_BUILD_TESTS=ON, then run with `ctest`.
#
# To regenerate an expected file after an intentional output change, run
# pioasm by hand and check the result carefully before committing it:
#   pioasm -o c-sdk clock_div.pio clock_div.expected.h

function(pioasm_add_output_test NAME INPUT FORMAT EXPECTED)
    add_test(NAME ${NAME}
        COMMAND ${CMAKE_COMMAND}
            -DPIOASM=$<TARGET_FILE:pioasm>
            -DINPUT=${CMAKE_CURRENT_LIST_DIR}/${INPUT}
            -DFORMAT=${FORMAT}
            -DEXPECTED=${CMAKE_CURRENT_LIST_DIR}/${EXPECTED}
            -DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/${NAME}.out
            -P ${CMAKE_CURRENT_LIST_DIR}/compare_output.cmake
    )
endfunction()

pioasm_add_output_test(clock_div_c_sdk clock_div.pio c-sdk clock_div.expected.h)
pioasm_add_output_test(clock_div_json  clock_div.pio json  clock_div.expected.json)
