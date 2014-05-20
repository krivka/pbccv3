#include "pbcc3_test.h"

#define ARR_SIZE 4

int test() {
    int array[ARR_SIZE] = { 0xFF, 0xFF, };
    int *arrPtr = array;
    char *ptr = (char *) array;
    char i;
    int result = 0;
    for (i = 0; i < ARR_SIZE; i++) {
        *arrPtr = 0xFF - i;
        arrPtr++;
    }
    for (i = 0; i < ARR_SIZE + ARR_SIZE; i++) {
        result += *ptr;
        ptr++;
    }
    return result;
}

void main() {
    test();
    END_EXECUTION;
    // s0 == F6, s1 == 0x03
}