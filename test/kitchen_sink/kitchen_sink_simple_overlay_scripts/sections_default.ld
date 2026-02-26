INCLUDE "section_default_text.ld"
INCLUDE "section_default_data.ld"

SECTIONS
{
    PROVIDE(__overlays_start__ = .);
    OVERLAY : {
        .overlay_first {
            KEEP (*(.first*))
            . = ALIGN(4);
        }
        .overlay_second {
            KEEP (*(.second*))
            . = ALIGN(4);
        }
    } > RAM AT> RAM_STORE
    PROVIDE(__overlays_end__ = .);
}

INCLUDE "section_heap.ld"
INCLUDE "section_scratch.ld"
INCLUDE "section_flash_end.ld"
INCLUDE "section_end.ld"
INCLUDE "section_platform_end.ld"
