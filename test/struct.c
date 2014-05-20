#include "pbcc3_test.h"

struct test{
    int a;
    char b;
    int c;
};

int test() {
    volatile struct test foo = { 0x1111, 0x22, 0x3333 };
    foo.a += foo.b;
    return foo.a + foo.c;
}

void main(void) {
    test();
    END_EXECUTION;
    // s0 == 0x66, s1 == 0x44
}