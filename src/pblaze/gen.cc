#include "gen.h"
#include "util.h"
#include <map>

namespace Gen {

typedef void (*genFunc)(ICode *);

void Function(ICode *ic) {
    Symbol *sym = ic->getLeft()->getSymbol();
    emit << sym << ":";
}

void Call(ICode *ic) {
    Symbol *sym = ic->getLeft()->getSymbol();
    emit << "call " << sym;
}

void AssignLiteral(ICode *ic) {
    Operand *result = ic->getResult();
    Operand *right = ic->getRight();

    if (!right->isLiteral)
        return;

    int size = right->getType()->getSize();

    emit << "; " << "Assigning value " << right->getValue()->getUnsignedLong() << "\n";

    for (int i = 0; i < result->getType()->getSize(); i++) {
        if (!result->getSymbol()->regs[i])
            result->getSymbol()->regs[i] = Bank::current()->getFreeRegister();

//         result->getSymbol()->regs[i]
    }
}

void Assign(ICode *ic) {
    Operand *result = ic->getResult();
    Operand *right = ic->getRight();

    if (result == right)
        return;

    if (right->isLiteral)
        AssignLiteral(ic);

    std::cerr << result->getSymbol()->name << result->getSymbol()->regs[0] << std::endl;
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
