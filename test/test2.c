void main(void) {
    volatile char var;
    if (var < 64) {
        var = 255;
    }
    else {
        var = 16;
    }
}
