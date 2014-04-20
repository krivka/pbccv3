char func(volatile long arg1, volatile int arg2, volatile char arg3) {
    arg1 += arg2 + arg3;
    return 0;
}

void main(void) {
    volatile char foo = 123;
    volatile int bar = 22222;
    volatile long baz = 999999999;
    func(baz, bar, foo);
}