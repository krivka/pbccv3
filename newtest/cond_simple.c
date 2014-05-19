#include "pbcc3_test.h"

char test() {
    volatile char one = 0x1;
    volatile char One = 0x1; // man, it just keeps optimizing
    volatile char zero = 0x0;

    volatile char foo = 0x0;

    if (one > zero) {
        foo |= 0x01;
    }
    if (one >= zero) {
        foo |= 0x02;
    }
    if (zero < one) {
        foo |= 0x04;
    }
    if (zero <= one) {
        foo |= 0x04;
    }
    if (one != zero) {
        foo |= 0x08;
    }
    if (one == One) {
        foo |= 0x10;
    }
    if (one >= One) {
        foo |= 0x20;
    }
    if (one <= One) {
        foo |= 0x40;
    }
    if (one) {
        foo |= 0x80;
    }
    return foo;
}

void main() {
    test();
    END_EXECUTION;
}