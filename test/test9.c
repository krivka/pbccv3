struct test_t {
    char a;
    char b;
    char c;
    char d;
    char e;
};

void main(void) {
    volatile struct test_t foo;
    volatile struct test_t bar = { .a=6, .b=7, .c=8, .d=9, .e=10 };
    foo.a = 0xFF;
    foo.b = 0xFE;
    foo.c = 0xFD;
    foo.d = 0xFC;
    foo.e = 0xFB;
    bar.a = 0xAA;
    bar.b = 0xAB;
    bar.c = 0xAC;
    bar.d = 0xAD;
    bar.e = 0xAE;
}
