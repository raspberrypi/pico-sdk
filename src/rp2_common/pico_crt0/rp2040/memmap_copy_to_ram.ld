INCLUDE "memory_flash.ld"
INCLUDE "memory_ram.ld"
INCLUDE "memory_scratch.ld"

ENTRY(_entry_point)

INCLUDE "sections_copy_to_ram.ld"
