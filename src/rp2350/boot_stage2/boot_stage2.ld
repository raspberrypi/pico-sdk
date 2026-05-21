MEMORY {
    /* We are loaded to the top 256 bytes of SRAM, which is above the bootrom
       stack. */
    SRAM(rx) : ORIGIN = 0x20081f00, LENGTH = 256
}

SECTIONS {
    . = ORIGIN(SRAM);
    .text : {
        _start = .; /* make LLVM happy */
        *(.entry)
        *(.text)
    } >SRAM
}
