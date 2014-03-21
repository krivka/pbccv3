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

    emit << "; " << "Assigning value " << right->getValue()->getUnsignedLong();

    if (result->isOpGlobal()) {
        for (int i = 0; i < right->getType()->getSize(); i++) {
            Allocator::putVal(ic, result, i);
        }
    }
    else if (result->isPointerSet()) {
        if (result->isInOutRef()) {
            Allocator::putVal(ic, result, 0);
        }
        for (int i = 0; i < size; i++) {
            Allocator::putVal(ic, result, i);
            if (i + 1 < size) {
                emit << Add(ic, 1);
                Allocator::updateOpInMem(ic, op, offset);
            }
        }
        if (!ic->pointerSetOpt(size - 1) && size >1 && result->liveTo() > ic->seq) {
            emit << Sub(ic, size - 1);
            Allocator::updateOpInMem(ic, op, 0);
        }
    }
    else {
        for (int i = 0; i < size; i++) {
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
