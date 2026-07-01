MEMORY {
    /* We don't know where we'll be loaded, so use an invalid address as code
       should be location invariant */
    SRAM(rx) : ORIGIN = 0x0, LENGTH = 256
}

SECTIONS {
    . = ORIGIN(SRAM);
    .text : {
        _start = .; /* make LLVM happy */
        *(.entry)
        *(.text)
    } >SRAM
}
