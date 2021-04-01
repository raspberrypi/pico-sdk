// ----------------------------------------------------------------------------
// Second stage boot code
// Copyright (c) 2019-2021 Raspberry Pi (Trading) Ltd.
// SPDX-License-Identifier: BSD-3-Clause
// ----------------------------------------------------------------------------
//
// This implementation uses the PICO_BOOT_STAGE2_CHOOSE_ preprocessor defines to pick
// amongst a menu of known boot stage 2 implementations, allowing the board
// configuration header to be able to specify the boot stage 2

#include "boot_stage2/config.h"

#ifdef PICO_BUILD_BOOT_STAGE2_NAME
    // boot stage2 is configured by cmake, so use the name specified there
    #error PICO_BUILD_BOOT_STAGE2_NAME should not be defined for compile_time_choice builds
#else
    // boot stage2 is selected by board configu header, so we have to do some work
    #if PICO_BOOT_STAGE2_CHOOSE_IS25LP080
        #include "boot2_is25lp080.S"
    #elif PICO_BOOT_STAGE2_CHOOSE_W25Q080
        #include "boot2_w28q080.S"
    #elif PICO_BOOT_STAGE2_CHOOSE_W25X10CL
        #include "boot2_w25x10cl.S"
    #elif PICO_BOOT_STAGE2_CHOOSE_GENERIC_03H
        #include "boot2_generic_03h.S"
    #else
        #error unknown boot stage2 choice
    #endif
#endif
