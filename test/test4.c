void main(void) {
    volatile char foo, bar, baz;
    for (foo = 1, bar = baz = foo + 31; (64 + foo) && (32 - bar) == baz && ((bar - bar) + foo); foo++, bar--, baz = foo + bar) {
        volatile char insideLoop = 64;
        insideLoop += foo - bar + baz;
    }
}
