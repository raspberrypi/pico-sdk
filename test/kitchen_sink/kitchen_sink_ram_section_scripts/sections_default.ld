INCLUDE "section_default_text.ld"
INCLUDE "section_default_data.ld"

SECTIONS
{
    .extra_data : {
        __extra_data_start__ = .;
        *(.extra_data*)
        . = ALIGN(4);
        __extra_data_end__ = .;
    } > RAM AT> RAM_STORE
    __extra_data_source__ = LOADADDR(.extra_data);
}

INCLUDE "section_heap.ld"
INCLUDE "section_scratch.ld"
INCLUDE "section_flash_end.ld"
INCLUDE "section_end.ld"
INCLUDE "section_platform_end.ld"