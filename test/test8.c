volatile static char qux = 0x11;

void main() {
    volatile char foo = 0;
    volatile char *bar = &foo;
    volatile char *baz = (char *) 0xFF;
    *bar = 0xFF;
    baz[0] = 0xEE;
    qux = 0xDD;
}
