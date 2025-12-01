INCLUDE "memory_flash.ld"
INCLUDE "memory_scratch.ld"

/* Only use 128k of SRAM, starting at 0x20020000 */
MEMORY
{
    RAM(rwx) : ORIGIN =  0x20020000, LENGTH = 128k
}

ENTRY(_entry_point)

INCLUDE "sections_default.ld"
