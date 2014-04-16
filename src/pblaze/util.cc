#include "util.h"
#include <iostream>

Emitter emit;
int Emitter::i = 0;

Emitter& Emitter::operator<<(const char &s) {
    std::cerr << s;
}

Emitter& Emitter::operator<<(const char *s) {
    std::cerr << s;
}

Emitter& Emitter::operator<<(unsigned int s) {
    std::cerr << s;
}

Emitter& Emitter::operator<<(unsigned long s) {
    std::cerr << s;
}

Emitter& Emitter::operator<<(const std::string &s) {
    std::cerr << s;
}
