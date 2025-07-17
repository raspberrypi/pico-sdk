MEMORY
{
    RAM(rx) : ORIGIN =  0x20000000, LENGTH = 512k
}

ENTRY(_start)

SECTIONS
{
    .wifi_fw : {
        *(.wifi_fw*)
        . = ALIGN(4);
    } > RAM
}
