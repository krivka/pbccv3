void main(void) {
    volatile char foo = 1, bar = 2, baz = 3, qux = 4, quux;
#if 0
    if (foo)
        quux = 5;
    __asm _________
    __endasm;

    if (foo < bar)
        quux = 5;
    __asm _________
    __endasm;

    if (foo == baz)
        quux = 5;
    __asm _________
    __endasm;
#endif
    if (foo < bar && foo == baz)
        quux = 5;
    __asm _________
    __endasm;

    if (foo < bar || foo == baz)
        quux = 5;
    
    __asm _________
    __endasm;
    if (foo >= bar)
        quux = 5;
}
