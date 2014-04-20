// static char globalVar = 0x6A; // makes it crash, investigate

char function(volatile char argument) {
    volatile char localVar = 0x1F;
    volatile char i;
    argument = 0x31;
    for (i = localVar + argument; i < 123; i++) {
        argument += localVar;
    }
    localVar = 0x44;
    return localVar;
}

void main(void) {
    volatile char var = 0x23;
    function(var);
    __asm
    ; This is a comment
    inlinelabel:
        load s1, s1;
    __endasm;
    var = 0x99;
    while (1);
}
