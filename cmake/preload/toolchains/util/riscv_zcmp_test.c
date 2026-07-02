/* Test that the toolchain can assemble Zcmp instructions */
void _start(void) {
    asm volatile ("cm.mvsa01 s0, s1" : : : "s0", "s1");
}
