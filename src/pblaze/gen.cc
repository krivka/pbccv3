#include "gen.h"
#include "util.h"
#include "wrap.h"
#include <map>

namespace Gen {

typedef void (*genFunc)(ICode *);

void Function(ICode *ic) {
    Symbol *sym = ic->getLeft()->getSymbol();
    emit << "\n" << sym << ":\n";
}

void Call(ICode *ic) {
    Symbol *sym = ic->getLeft()->getSymbol();
    emit << I::Call(sym);
}

void Return(ICode *ic) {
    Operand *left = ic->getLeft();

    if (!left)
        return;

    // and now... what
}

void EndFunction(ICode *ic) {
    emit << I::Ret();
}

void Send(ICode *ic) {

}

void Receive(ICode *ic) {

}

void Label(ICode *ic) {
    emit << ic->getLabel() << ":\n";
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

void CmpLt(ICode *ic) {
    if (ic->getNext()->op != IFX)
        return;

    for (Emitter::i = 0; Emitter::i < ic->getLeft()->getType()->getSize(); Emitter::i++) {
        emit << I::Compare(ic->getLeft(), ic->getRight());
    }

    if (ic->getNext()->icTrue())
        emit << I::Jump(ic->getNext()->icTrue(), I::Jump::C);
    else
        emit << I::Jump(ic->getNext()->icFalse(), I::Jump::C);

    ic->getNext()->generated = 1;
}

void InlineAsm(ICode *ic) {
    emit << ic->inlineAsm;
}

std::map<unsigned int, genFunc> map {
    { FUNCTION, Function },
    { RETURN, Return },
    { ENDFUNCTION, EndFunction },
    { CALL, Call },
    { SEND, Send },
    { RECEIVE, Receive },

    { LABEL, Label },
    { GOTO, GoTo },

    { '=', Assign },
    { '+', Add },
    { '-', Sub },

    { IFX, Ifx },
    { '<', CmpLt },

    { INLINEASM, InlineAsm },
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
