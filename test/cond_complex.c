#include "pbcc3_test.h"

int test() {
    volatile int i;
    volatile char foo = 0xFF, bar = 0xAA;
    volatile int qux = 0;

    for (i = (~foo) ^ bar - 0xAA; i < (foo - bar) >> 4; foo += (qux & foo) >> 6) {
        qux++;
    }

    return qux;
}

void main() {
    volatile int res = test();
    END_EXECUTION
}
