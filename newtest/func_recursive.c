#include "pbcc3_test.h"

int addToZero(int x) {
    if (!x)
        return 0;
    return x + addToZero(x - 1);
}

void main(void) {
    volatile int result = addToZero(5);
    END_EXECUTION;
}