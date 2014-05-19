#include "pbcc3_test.h"

void main() {
    volatile char foo = 0x33;
    volatile char bar = 0x03;
    volatile char baz = 0x30;
    volatile char qux = 0x22;

    volatile char quux = 0x00;
    volatile char quuux = 0x00;

    char i;

    if (((foo && bar) > 0) || ((baz & qux) < (foo ^ bar))) {
        
    }

    for (i = (foo & bar) | baz;
        ((i & foo) | bar) ^ baz;
        i += (foo ^ qux) & qux) {
        quux++;
    }

    END_EXECUTION;
}