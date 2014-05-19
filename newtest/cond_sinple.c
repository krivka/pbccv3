#include "pbcc3_test.h"

void main() {
    volatile char foo = 0x33;
    volatile char bar = 0x03;
    volatile char baz = 0x30;
    volatile char qux = 0x22;

    volatile char quux = 0x00;
    volatile char quuux = 0x00;

    int i;

    if (foo == bar) {
        quux &= 0x01;
    }
    else {
        quux &= 0x02;
    }

    for (i = 0; i < bar; i++) {
        quuux += i;
    }

    while (quuux > 0) {
        quuux--;
    }

    if (quuux)
        quux &= 0x04;
    else
        quux &= 0x08;

    do {
        quuux += bar;
    } while (bar >>= 1);

    quux &= bar;

    END_EXECUTION;
}