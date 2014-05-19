#include "util.h"
#include "wrap.h"
#include <iostream>

Emitter Emitter::_emit = {};
stringstream Emitter::ss = {};
int Emitter::i = 0;

Emitter& Emitter::operator<<(const char &s) {
    ss << s;
    std::cerr << s;
}

Emitter& Emitter::operator<<(const char *s) {
    ss << s;
    std::cerr << s;
}

Emitter& Emitter::operator<<(unsigned int s) {
    ss << s;
    std::cerr << s;
}

Emitter& Emitter::operator<<(unsigned long s) {
    ss << s;
    std::cerr << s;
}

Emitter& Emitter::operator<<(const std::string &s) {
    ss << s;
    std::cerr << s;
}


bool Function::isMain = false;
int Function::argumentCnt = 0;
int Function::registerSize = 0;
int Function::stackSize = 0;
int Function::futureSP = 0;

void Function::processNew(ICode* ic) {
    // determine if it's main
    if (0 == strcmp(ic->getLeft()->getSymbol()->name, "main"))
        Function::isMain = true;

    // count the arguments
    Function::argumentCnt = 0;
    Function::registerSize = 0;
    Function::stackSize = 0;
    Value *args = (Value*) ic->getLeft()->getType()->funcAttrs.args;
    while (args) {
        Function::argumentCnt++;
        if (Function::registerSize + args->getType()->getSize() < VAR_REG_CNT && !Function::stackSize)
            Function::registerSize += args->getType()->getSize();
        else
            Function::stackSize += args->getType()->getSize();
        args = args->getNext();
    }
}
