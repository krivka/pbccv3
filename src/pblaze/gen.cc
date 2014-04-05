#include "gen.h"
#include "util.h"
#include "wrap.h"
#include <map>

namespace Gen {

typedef void (*genFunc)(ICode *);

void Function(ICode *ic) {
    Symbol *sym = ic->getLeft()->getSymbol();
    emit << sym << ":";
}

void Call(ICode *ic) {
    Symbol *sym = ic->getLeft()->getSymbol();
    emit << I::Call(sym);
}

void AssignLiteral(ICode *ic) {
    Operand *result = ic->getResult();
    Operand *right = ic->getRight();

    if (!right->isLiteral)
        return;

    int size = right->getType()->getSize();

    emit << "; " << "Assigning value " << right->getValue()->getUnsignedLong() << " into " << result->getSymbol()->name << "\n";

    auto getByte = [](unsigned long n, uint8_t byte) {
        return n & (0xFF << (byte * 8));
    };

    for (int i = 0; i < result->getType()->getSize(); i++) {
        Register **regp = &result->getSymbol()->regs[i];
        if (!*regp)
            *regp = Bank::current()->getFreeRegister();

        (*regp)->occupy(result, i);
        emit << I::Load(*regp, getByte(right->getValue()->getUnsignedLong(), i));
    }
}

void Assign(ICode *ic) {
    Operand *result = ic->getResult();
    Operand *right = ic->getRight();

    if (result == right)
        return;

    if (right->isLiteral)
        AssignLiteral(ic);


}

std::map<unsigned int, genFunc> map {
    { FUNCTION, Function },
    { CALL, Call },
    { '=', Assign },
};

};

void genPBlazeCode(ICode *lic) {
    ICode *ic;

    for (ic = lic; ic; ic = ic->getNext()) {
        if (ic->generated)
            continue;

        Gen::genFunc gen = Gen::map[ic->op];
        if (gen)
            gen(ic);

        emit << "\n";
    }
}
