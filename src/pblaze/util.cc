#include "util.h"
#include <iostream>

Emitter emit;

Emitter& Emitter::operator<<(const char* s) {
    std::cerr << s;
}

Emitter& Emitter::operator<<(unsigned long s) {
    std::cerr << s;
}
