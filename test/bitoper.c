#include "pbcc3_test.h"

int test() {
    volatile int foo = 0x88, bar = 0x04, baz = 0x40;
    volatile int qux = (bar ^ baz);
    qux |= foo;
    qux &= 0xEE;
    qux |= 0x33;
    qux <<= 4;
    return qux;
}

void main(void) {
    test();
    END_EXECUTION;
    // s0 == 0xF0, s1 == 0x0F
}