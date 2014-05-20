#include "pbcc3_test.h"

void main(void) {
    volatile char foo = 0x87;
    volatile int bar = 0x5678;
    volatile long baz = 0x12340000;

    foo += bar;
    baz += bar;
    bar += 0x5443;
    (void) foo, bar, baz;
    END_EXECUTION;
    // foo == 0xFF, bar == 0xAABB, baz == 0x12345678 - these bytes have to be somewhere
}