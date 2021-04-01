/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _BOOT_STAGE2_CONFIG_H_
#define _BOOT_STAGE2_CONFIG_H_

// NOTE THIS HEADER IS INCLUDED FROM ASSEMBLY

// PICO_CONFIG: PICO_BUILD_BOOT_STAGE2_NAME, The name of the boot stage 2 if selected by the build, group=boot_stage2
// PICO_CONFIG: PICO_BOOT_STAGE2_CHOOSE_IS25LP080, Select boot2_is25lp080 as the boot stage 2 when no boot stage2 selection is made by the CMake build, type=bool, default=false, group=boot_stage2
// PICO_CONFIG: PICO_BOOT_STAGE2_CHOOSE_W25Q080, Select boot2_w28q080 as the boot stage 2 when no boot stage2 selection is made by the CMake build, type=bool, default=false, group=boot_stage2
// PICO_CONFIG: PICO_BOOT_STAGE2_CHOOSE_W25X10CL, Select boot2_is25lp080 as the boot stage 2 when no boot stage2 selection is made by the CMake build, type=bool, default=false, group=boot_stage2
// PICO_CONFIG: PICO_BOOT_STAGE2_CHOOSE_GENERIC_03H, Select boot2_generic_03h as the boot stage 2 when no boot stage2 selection is made by the CMake build, type=bool, default=true, group=boot_stage2

#ifdef PICO_BUILD_BOOT_STAGE2_NAME
    // boot stage2 is configured by cmake, so use the name specified there
    #define PICO_BOOT_STAGE2_NAME PICO_BUILD_BOOT_STAGE2_NAME
#else
    // boot stage2 is selected by board configu header, so we have to do some work
    // NOTE: this switch is mirrored in compile_time_choice.S
    #if PICO_BOOT_STAGE2_CHOOSE_IS25LP080
        #define PICO_BOOT_STAGE2_NAME "boot2_is25lp080"
    #elif PICO_BOOT_STAGE2_CHOOSE_W25Q080
        #define PICO_BOOT_STAGE2_NAME "boot2_w28q080"
    #elif PICO_BOOT_STAGE2_CHOOSE_W25X10CL
        #define PICO_BOOT_STAGE2_NAME "boot2_w25x10cl"
    #elif PICO_BOOT_STAGE2_CHOOSE_GENERIC_03H || !defined(PICO_BOOT_STAGE2_CHOOSE_GENERIC_03H)
        #undef PICO_BOOT_STAGE2_CHOOSE_GENERIC_03H
        #define PICO_BOOT_STAGE2_CHOOSE_GENERIC_03H 1
        #define PICO_BOOT_STAGE2_NAME "boot2_generic_03h"
    #else
        #error no bootstage2 is defined by PICO_BOOT_STAGE2_CHOOSE_ macro
    #endif
#endif
#endif