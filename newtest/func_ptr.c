#include "pbcc3_test.h"

int fibonacci(int step, int n_1, int n_2) {
    int current = n_1 + n_2;
    return current + fibonacci(step - 1, n_2, current);
}

void main(void) {
    int (*fptr)(int, int, int) = fibonacci;
    volatile int result = fptr(5, 1, 1);
    END_EXECUTION;
}