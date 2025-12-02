/* Include memory regions used */
INCLUDE "rp2_common/memory_flash.ld"
INCLUDE "platform/memory_scratch.ld"

/* Only use 128k of SRAM, starting at 0x20020000 */
MEMORY
{
    RAM(rwx) : ORIGIN =  0x20020000, LENGTH = 128k
}

/* Include aliases for storage memory regions */
INCLUDE "rp2_common/memory_aliases_default.ld"

/* Define entry point symbol */
ENTRY(_entry_point)

/* Include default sections */
INCLUDE "rp2_common/sections_default.ld"
