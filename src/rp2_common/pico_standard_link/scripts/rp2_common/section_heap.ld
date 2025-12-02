SECTIONS
{
    .heap DEFINED(HEAP_LOC) ? HEAP_LOC : . (NOLOAD):
    {
        __end__ = .;
        end = __end__;
        KEEP(*(.heap*))
    } > RAM
    /* historically on GCC sbrk was growing past __HeapLimit to __StackLimit, however
       to be more compatible, we now set __HeapLimit explicitly to where the end of the heap is */
    __HeapLimit = DEFINED(HEAP_LIMIT) ? HEAP_LIMIT : ORIGIN(RAM) + LENGTH(RAM);
}
