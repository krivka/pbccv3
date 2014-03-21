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

    emit << "; " << "Assigning value " << right->getValue()->getUnsignedLong();

    if (result->isOpGlobal()) {
        for (int i = 0; i < right->getType()->getSize(); i++) {
            Allocator::putVal(ic, result, i);
        }
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
