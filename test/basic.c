void main(void) {
    volatile char foo = 0x12;
    volatile char bar = 0x34;
    volatile char baz = 0x56;
    volatile char qux = 0x78;
    volatile char quux = 0x9A;
    volatile char quuux = 0xBC;
    volatile char quuuux = 0xDE;
    volatile char quuuuux = 0xF0;
    (void) foo, bar, baz, qux, quux, quuux, quuuux, quuuuux;
}