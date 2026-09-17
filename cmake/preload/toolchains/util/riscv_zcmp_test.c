/* Test that the toolchain can assemble Zcmp instructions */
void _start(void) {
    // Cannot use s0 as it is also fp, so use s1 & s2 instead
    asm volatile ("cm.mvsa01 s1, s2" : : : "s1", "s2");
}
