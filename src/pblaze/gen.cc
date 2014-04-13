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

    // for some reason, assignment of a symbolic operand that is neither in memory nor in registers was requested by the frontend
    // HACK: just ignore
    // seems to actually mean something
    if (right->isSymOp() && !Memory::get()->contains(right, 0) && !right->getSymbol()->regs[0])
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
    Emitter::i = 0;
}

void Sub(ICode *ic) {
    Operand *result = ic->getResult();
    Operand *left = ic->getLeft();
    Operand *right = ic->getRight();

    for (Emitter::i = 0; Emitter::i < left->getType()->getSize(); Emitter::i++) {
        emit << I::Sub(left, right);
    }

    // TODO this is definitely wrong
    if (*result != *left) {
        for (Emitter::i = 0; Emitter::i < left->getType()->getSize(); Emitter::i++) {
            emit << I::Load(result, left);
        }
        Emitter::i = 0;
    }
}

void Ifx(ICode *ic) {
    emit << "; this would definitely be a IFX\n";
}

void CmpLt(ICode *ic) {
    if (ic->getNext()->op != IFX) {
        std::cerr << "what now?";
        return;
    }

    for (Emitter::i = 0; Emitter::i < ic->getLeft()->getType()->getSize(); Emitter::i++) {
        emit << I::Compare(ic->getLeft(), ic->getRight());
    }
    Emitter::i = 0;

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
