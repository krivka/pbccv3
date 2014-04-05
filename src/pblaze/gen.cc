#include "gen.h"
#include "util.h"
#include "wrap.h"
#include <map>

namespace Gen {

typedef void (*genFunc)(ICode *);

void Function(ICode *ic) {
    Symbol *sym = ic->getLeft()->getSymbol();
    emit << sym << ":\n";
}

void Label(ICode *ic) {
    emit << ic->getLabel() << ":\n";
}

void Call(ICode *ic) {
    Symbol *sym = ic->getLeft()->getSymbol();
    emit << I::Call(sym);
}

void GoTo(ICode *ic) {
    emit << I::Jump(ic->getLabel());
}

void Assign(ICode *ic) {
    Operand *result = ic->getResult();
    Operand *right = ic->getRight();

    if (*result == *right)
        return;

    emit << I::Load(result, right);

}

void Add(ICode *ic) {
    Operand *result = ic->getResult();
    Operand *left = ic->getLeft();
    Operand *right = ic->getRight();

    for (Emitter::i = 0; Emitter::i < left->getType()->getSize(); Emitter::i++) {
        emit << I::Add(left, right);
    }

    // += ...
    if (*result != *left) {
        for (Emitter::i = 0; Emitter::i < left->getType()->getSize(); Emitter::i++) {
            emit << I::Load(result, left);
        }
    }
}

void Sub(ICode *ic) {
    Operand *result = ic->getResult();
    Operand *left = ic->getLeft();
    Operand *right = ic->getRight();

    for (Emitter::i = 0; Emitter::i < left->getType()->getSize(); Emitter::i++) {
        emit << I::Sub(left, right);
    }

    // += ...
    if (*result != *left) {
        for (Emitter::i = 0; Emitter::i < left->getType()->getSize(); Emitter::i++) {
            emit << I::Load(result, left);
        }
    }
}

void Ifx(ICode *ic) {

}

std::map<unsigned int, genFunc> map {
    { FUNCTION, Function },
    { LABEL, Label },
    { CALL, Call },
    { GOTO, GoTo },

    { '=', Assign },
    { '+', Add },
    { '-', Sub },

    { IFX, Ifx },
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
        else
            std::cerr << "  ; !!! op " << ic->op << "(" << getTableEntry(ic->op)->printName << ") in " << ic->filename << " on line " << ic->lineno << "\n";
    }
}
