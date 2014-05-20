#include "pbcc3_test.h"

int function(char param1, int param2, long param3) {
    return param1 + param2 + param3;
}

void main(void) {
    volatile int foo = function(11, 333, 77777);
    END_EXECUTION;
    // s0 == 0x29, s1 == 31
}