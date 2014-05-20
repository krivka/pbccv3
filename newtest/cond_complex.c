#include "pbcc3_test.h"

int test() {
    volatile int i;
    volatile char foo = 0xFF, bar = 0xAA;
    volatile int qux = 0;

    for (i = foo | bar; i < foo + bar; foo &= bar ^ 0xA0) {
        qux++;
    }

    return qux;
}

void main() {
    volatile int res = test();
    END_EXECUTION;
}