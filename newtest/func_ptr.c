#include "pbcc3_test.h"

int addToZero(int x) {
    if (!x)
        return 0;
    return x + addToZero(x - 1);
}

void main(void) {
    int (*fptr)(int) = addToZero;
    volatile int result = fptr(5);
    END_EXECUTION;
    // s0 == 0x0F, s1 == 0x00
}