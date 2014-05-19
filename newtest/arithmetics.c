#include "pbcc3_test.h"

char arithmetics() {
    volatile char foo = 0x33;
    volatile char bar = 0x03;
    volatile char baz = 0x30;
    volatile char qux = 0x22;
    //            0x11   ((    0x02 ) 0x22 )  0x22
    volatile char quux = ((foo & bar) | baz) ^ qux;
    //             0x41   0x11  0x33  0x03
    volatile char quuux = quux + foo - bar;

    return quuux;
}

void main() {
    arithmetics();
    END_EXECUTION;
}