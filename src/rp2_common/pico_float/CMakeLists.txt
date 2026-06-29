 if (NOT TARGET pico_float)
    # library to be depended on - we make this depend on particular implementations using per target generator expressions
    pico_add_library(pico_float)

    # no custom implementation; falls thru to compiler
    pico_add_library(pico_float_compiler)

    target_include_directories(pico_float_headers SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)


    # add alias "default" which is just pico.
    add_library(pico_float_default INTERFACE)
    target_link_libraries(pico_float_default INTERFACE pico_float_pico)

    # PICO_CMAKE_CONFIG: PICO_DEFAULT_FLOAT_IMPL, The default implementation for pico_float to link. 'pico' selects the fastest implementation for the current platform; 'compiler' uses the default C library versions but omits additional functions provided when using 'pico'/'pico_vfp'/'pico_dcp'; 'none' removes all single-precision floating point functions altogether; on RP2350 'pico_vfp' and 'pico_dcp' can also be used to select between using the Arm hardware floating point instructions or the RP2350 double coprocessor for single-precision floating point operations, type=string, default=pico, group=pico_float
    pico_fixup_default_impl(PICO_DEFAULT_FLOAT_IMPL pico_float default)

    target_link_libraries(pico_float INTERFACE
            $<IF:$<BOOL:$<TARGET_PROPERTY:PICO_TARGET_FLOAT_IMPL>>,$<TARGET_PROPERTY:PICO_TARGET_FLOAT_IMPL>,${PICO_DEFAULT_FLOAT_IMPL}>)

    function(wrap_float_functions TARGET)
        cmake_parse_arguments(WRAP_FLOAT "NO_AEABI_ARITHMETIC;NO_AEABI_CMP;NO_AEABI_CONV_32;NO_AEABI_CONV_64;NO_AEABI_CONV_DOUBLE;NO_SQRTF;NO_SCI;NO_SCI_EXTRA;NO_FMAF" "" "" ${ARGN} )
        if (NOT WRAP_FLOAT_NO_AEABI_ARITHMETIC)
            pico_wrap_function(${TARGET} __aeabi_fadd)
            pico_wrap_function(${TARGET} __aeabi_fdiv)
            pico_wrap_function(${TARGET} __aeabi_fmul)
            pico_wrap_function(${TARGET} __aeabi_frsub)
            pico_wrap_function(${TARGET} __aeabi_fsub)
        endif()
        if (NOT WRAP_FLOAT_NO_AEABI_CMP)
            pico_wrap_function(${TARGET} __aeabi_cfcmpeq)
            pico_wrap_function(${TARGET} __aeabi_cfrcmple)
            pico_wrap_function(${TARGET} __aeabi_cfcmple)
            pico_wrap_function(${TARGET} __aeabi_fcmpeq)
            pico_wrap_function(${TARGET} __aeabi_fcmplt)
            pico_wrap_function(${TARGET} __aeabi_fcmple)
            pico_wrap_function(${TARGET} __aeabi_fcmpge)
            pico_wrap_function(${TARGET} __aeabi_fcmpgt)
            pico_wrap_function(${TARGET} __aeabi_fcmpun)
        endif()
        if (NOT WRAP_FLOAT_NO_AEABI_CONV_32)
            pico_wrap_function(${TARGET} __aeabi_i2f)
            pico_wrap_function(${TARGET} __aeabi_ui2f)
            pico_wrap_function(${TARGET} __aeabi_f2iz)
            pico_wrap_function(${TARGET} __aeabi_f2uiz)
        endif()
        if (NOT WRAP_FLOAT_NO_AEABI_CONV_64)
            pico_wrap_function(${TARGET} __aeabi_l2f)
            pico_wrap_function(${TARGET} __aeabi_ul2f)
            pico_wrap_function(${TARGET} __aeabi_f2lz)
            pico_wrap_function(${TARGET} __aeabi_f2ulz)
        endif()
        if (NOT WRAP_FLOAT_NO_AEABI_CONV_DOUBLE)
            pico_wrap_function(${TARGET} __aeabi_f2d)
        endif()
        # separate as we have a direct DCP version, and VFP has an instruction
        if (NOT WRAP_FLOAT_NO_SQRTF)
            pico_wrap_function(${TARGET} sqrtf)
        endif()
        if (NOT WRAP_FLOAT_NO_SCI)
            pico_wrap_function(${TARGET} cosf)
            pico_wrap_function(${TARGET} sinf)
            pico_wrap_function(${TARGET} tanf)
            pico_wrap_function(${TARGET} atan2f)
            pico_wrap_function(${TARGET} expf)
            pico_wrap_function(${TARGET} logf)
            pico_wrap_function(${TARGET} sincosf) # gnu
        endif()
        if (NOT WRAP_FLOAT_NO_SCI_EXTRA)
            pico_wrap_function(${TARGET} ldexpf)
            pico_wrap_function(${TARGET} copysignf)
            pico_wrap_function(${TARGET} truncf)
            pico_wrap_function(${TARGET} floorf)
            pico_wrap_function(${TARGET} ceilf)
            pico_wrap_function(${TARGET} roundf)
            pico_wrap_function(${TARGET} asinf)
            pico_wrap_function(${TARGET} acosf)
            pico_wrap_function(${TARGET} atanf)
            pico_wrap_function(${TARGET} sinhf)
            pico_wrap_function(${TARGET} coshf)
            pico_wrap_function(${TARGET} tanhf)
            pico_wrap_function(${TARGET} asinhf)
            pico_wrap_function(${TARGET} acoshf)
            pico_wrap_function(${TARGET} atanhf)
            pico_wrap_function(${TARGET} exp2f)
            pico_wrap_function(${TARGET} log2f)
            pico_wrap_function(${TARGET} exp10f)
            pico_wrap_function(${TARGET} log10f)
            pico_wrap_function(${TARGET} powf)
            pico_wrap_function(${TARGET} powintf) #gnu
            pico_wrap_function(${TARGET} hypotf)
            pico_wrap_function(${TARGET} cbrtf)
            pico_wrap_function(${TARGET} fmodf)
            pico_wrap_function(${TARGET} dremf)
            pico_wrap_function(${TARGET} remainderf)
            pico_wrap_function(${TARGET} remquof)
            pico_wrap_function(${TARGET} expm1f)
            pico_wrap_function(${TARGET} log1pf)
        endif()
        if (NOT WRAP_FLOAT_NO_FMAF)
            pico_wrap_function(${TARGET} fmaf)
        endif()
    endfunction()

    pico_add_library(pico_float_none)
    target_sources(pico_float_none INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/float_none.S
    )

    # float_none doesn't make much sense with hard float
    if (NOT PICO_HARD_FLOAT_ABI)
        target_link_libraries(pico_float_none INTERFACE pico_float_headers)
        wrap_float_functions(pico_float_none) # we wrap all functions
        if (NOT PICO_RISCV)
            # be explicit that there should be no floating point instructions
            target_compile_options(pico_float_none INTERFACE -msoft-float)
        endif()
    endif()
    pico_add_library(pico_float_pico)
    if (PICO_RP2040)
        target_sources(pico_float_pico INTERFACE
                ${CMAKE_CURRENT_LIST_DIR}/float_aeabi_rp2040.S
                ${CMAKE_CURRENT_LIST_DIR}/float_init_rom_rp2040.c
                ${CMAKE_CURRENT_LIST_DIR}/float_math.c
                ${CMAKE_CURRENT_LIST_DIR}/float_v1_rom_shim_rp2040.S
        )
        wrap_float_functions(pico_float_pico)
        target_link_libraries(pico_float_pico INTERFACE pico_bootrom pico_float_headers hardware_divider)
    elseif(NOT PICO_RISCV)
        # dcp library not supported with hard float
        if (NOT PICO_HARD_FLOAT_ABI)
            pico_add_library(pico_float_pico_dcp)
            # todo what functions from float_math belong in each case; should some be left to GCC on RP2350?
            target_sources(pico_float_pico_dcp INTERFACE
                    ${CMAKE_CURRENT_LIST_DIR}/float_math.c
                    ${CMAKE_CURRENT_LIST_DIR}/float_aeabi_dcp.S
                    ${CMAKE_CURRENT_LIST_DIR}/float_common_m33.S
                    ${CMAKE_CURRENT_LIST_DIR}/float_sci_m33.S
                    )

            # NOTE the main reason for using pico_float_pico_dcp is presumably that you
            # don't want to use VFP at all, so turn off compiler support, otherwise, it will inline usages
            target_compile_options(pico_float_pico_dcp INTERFACE -msoft-float)

            wrap_float_functions(pico_float_pico_dcp
                    # we wrap all functions as we don't want to use VFP (or compiler versions) at all
                    #NO_AEABI_ARITHMETIC
                    #NO_AEABI_CMP
                    #NO_AEABI_CONV_32
                    #NO_AEABI_CONV_64
                    #NO_AEABI_CONV_DOUBLE
                    #NO_SQRTF
                    #NO_SCI
                    #NO_SCI_EXTRA
            )
        endif()

        pico_add_library(pico_float_pico_vfp)
        target_sources(pico_float_pico_vfp INTERFACE
                ${CMAKE_CURRENT_LIST_DIR}/float_math.c
                ${CMAKE_CURRENT_LIST_DIR}/float_conv32_vfp.S
                ${CMAKE_CURRENT_LIST_DIR}/float_common_m33.S
                ${CMAKE_CURRENT_LIST_DIR}/float_sci_m33_vfp.S
        )
        wrap_float_functions(pico_float_pico_vfp
                # for these 3, arguably compiler is probably inlining anyway, but use the cmopiler's
                # version for explicit AEABI calls
                NO_AEABI_ARITHMETIC # direct VFP instruction support
                NO_AEABI_CMP        # direct VFP instruction support
                NO_AEABI_CONV_32    # direct VFP instruction support
                #NO_AEABI_CONV_64   # we have optimized M33 versions
                NO_AEABI_CONV_DOUBLE# direct VFP instruction support
                NO_SQRTF            # direct VFP instruction support
                #NO_SCI             # we have optimized VFP versions
                #NO_SCI_EXTRA       # todo - are our versions better than what GCC proides?
                NO_FMAF             # direct VFP instruction support
        )
        # this allows inlining of sqrtf for example - if you really want errno support, use pico_float_compiler
        target_compile_options(pico_float_pico_vfp INTERFACE -fno-math-errno)

        target_link_libraries(pico_float_pico INTERFACE
                pico_float_pico_vfp)
    else()
        target_sources(pico_float_pico INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/float_single_hazard3.S
            )
    endif()

    macro(pico_set_float_implementation TARGET IMPL)
        get_target_property(target_type ${TARGET} TYPE)
        if ("EXECUTABLE" STREQUAL "${target_type}")
            set_target_properties(${TARGET} PROPERTIES PICO_TARGET_FLOAT_IMPL "pico_float_${IMPL}")
        else()
            message(FATAL_ERROR "float implementation must be set on executable not library")
        endif()
    endmacro()
endif()
