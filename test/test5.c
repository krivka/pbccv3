void main(void) {
    volatile int foo = 1, bar = 2, baz = 3;
    volatile int res = foo < bar < baz;
}
