#include "pbcc3_test.h"

void main(void) {
    volatile char foo = 11;
    volatile int bar = 333;
    volatile long baz = 77777;

    bar += foo;
    baz += bar;
    foo = baz;
    END_EXECUTION;
}