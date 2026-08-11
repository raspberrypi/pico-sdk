/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/test.h"
#include "hardware/pio_instructions.h"
#include "encodings.pio.h"

PICOTEST_MODULE_NAME("PIO_ENCODING", "pio encoding test");


int main() {
    stdio_init_all();

    PICOTEST_START();

    PICOTEST_START_SECTION("check pio_encodings");

    PICOTEST_CHECK(0xa0e1 == encodings_program_instructions[encodings_offset_mov_osr_x], "mov osr, x");
    PICOTEST_CHECK(0xa0e1 == pio_encode_mov(pio_osr, pio_x), "pio_encode_mov(pio_osr, pio_x)");
    PICOTEST_CHECK(0xa061 == encodings_program_instructions[encodings_offset_mov_pindirs_x], "mov pindirs, x");
    PICOTEST_CHECK(0xa061 == pio_encode_mov(pio_pindirs, pio_x), "pio_encode_mov(pio_pindirs, pio_x)");
#if PICO_PIO_VERSION > 0
    PICOTEST_CHECK(0xa061 == pio_encode_mov(pio_pindirs_mov, pio_x), "pio_encode_mov(pio_pindirs_mov, pio_x)");
#endif
    PICOTEST_CHECK(0xa081 == encodings_program_instructions[encodings_offset_mov_exec_x], "mov exec, x");
    PICOTEST_CHECK(0xa081 == pio_encode_mov(pio_exec, pio_x), "pio_encode_mov(pio_exec, pio_x)");
    PICOTEST_CHECK(0xa081 == pio_encode_mov(pio_exec_mov, pio_x), "pio_encode_mov(pio_exec_mov, pio_x)");
    PICOTEST_CHECK(0xa041 == encodings_program_instructions[encodings_offset_mov_y_x], "mov y, x");
    PICOTEST_CHECK(0xa041 == pio_encode_mov(pio_y, pio_x), "pio_encode_mov(pio_y, pio_x)");
    PICOTEST_END_SECTION();

    PICOTEST_END_TEST();
}

