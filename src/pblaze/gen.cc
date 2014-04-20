#include "gen.h"
#include "util.h"
#include "wrap.h"
#include <map>

ICode *currentIC = nullptr;
static ICode *stepIC();

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
    emit << ";;;;; Send " << ic->getLeft() << "\n";
}

void Receive(ICode *ic) {
    emit << ";;;;; Receive " << ic->getResult()->getSymbol() << "\n";
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

    emit << ";;;;; Assign " << right << " to " << result << "\n";

    // for some reason, assignment of a symbolic operand that is neither in memory nor in registers was requested by the frontend
    // HACK: just ignore
    // seems to actually mean something
    if (right->isSymOp() && !Memory::get()->contains(right, 0) && !right->getSymbol()->regs[0])
        return;

    for (Emitter::i = 0; Emitter::i < result->getType()->getSize(); Emitter::i++) {
        emit << I::Load(result, right);
    }
}

void Add(ICode *ic) {
    Operand *result = ic->getResult();
    Operand *left = ic->getLeft();
    Operand *right = ic->getRight();

    emit << ";;;;; Add " << left << " to " << right << " and store it into " << result << "\n";

    for (Emitter::i = 0; Emitter::i < left->getType()->getSize(); Emitter::i++) {
        emit << I::Add(left, right);
    }
    Emitter::i = 0;
}

void Sub(ICode *ic) {
    Operand *result = ic->getResult();
    Operand *left = ic->getLeft();
    Operand *right = ic->getRight();

    emit << ";;;;; Sub " << left << " from " << right << " and store it into " << result << "\n";

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
    emit << ";;;;; Jump if " << ic->getCondition() << "\n";
    // if we depend on the result of the previous operation, we should already
    // have what we need in the zero flag (except LT/GT)
    if (ic->getPrev()->getResult() == ic->getCondition()) {
        if (ic->getPrev()->op == '<') {
            if (ic->icTrue())
                emit << I::Jump(ic->icTrue(), I::Jump::C);
            else
                emit << I::Jump(ic->icFalse(), I::Jump::NC);
        }
        else if (ic->getPrev()->op == '>') {
            if (ic->icTrue())
                emit << I::Jump(ic->icTrue(), I::Jump::NC);
            else
                emit << I::Jump(ic->icFalse(), I::Jump::C);
        }
        else {
            if (ic->icTrue())
                emit << I::Jump(ic->icTrue(), I::Jump::Z);
            else
                emit << I::Jump(ic->icFalse(), I::Jump::NZ);
        }
    }
    else {
        std::cerr << "; Condition too complex\n";
    }
    emit << I::Load(ic->getCondition(), ic->getCondition());
}

void CmpEq(ICode *ic) {

    emit << ";;;;; Compare " << ic->getLeft() << " to " << ic->getRight() << " and store result into " << ic->getResult() << "\n";
//     emit << "; comparing " << ic->getRight()->getSymbol()->rname << " to " << ic->getResult()->getSymbol()->rname << "\n";
    for (Emitter::i = 0; Emitter::i < ic->getLeft()->getType()->getSize(); Emitter::i++) {
        emit << I::Compare(ic->getLeft(), ic->getRight());
    }
}

void CmpLt(ICode *ic) {
    emit << ";;;;; Compare (lower) " << ic->getLeft() << " to " << ic->getRight() << " and store result into " << ic->getResult() << "\n";

    for (Emitter::i = 0; Emitter::i < ic->getLeft()->getType()->getSize(); Emitter::i++) {
        emit << I::Compare(ic->getLeft(), ic->getRight());
    }
    Emitter::i = 0;
}

void Cast(ICode *ic) {
    // Don't forget about bool -> byte casting
    emit << "; casting " << ic->getRight()->getSymbol()->rname << " to " << ic->getResult()->getSymbol()->rname << "\n";
    emit << "; right: size: " << ic->getRight()->getType()->getSize() << "\n";
    emit << "; result: size: " << ic->getResult()->getType()->getSize() << "\n";
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
    { CAST, Cast },

    { '+', Add },
    { '-', Sub },

    { IFX, Ifx },
    { EQ_OP, CmpEq },
    { '<', CmpLt },

    { INLINEASM, InlineAsm },
};

};

ICode *stepIC() {
    if (!currentIC->generated) {
        Gen::genFunc gen = Gen::map[currentIC->op];
        if (gen)
            gen(currentIC);
        else
            std::cerr << "  ; !!! op " << currentIC->op << "(" << getTableEntry(currentIC->op)->printName << ") in " << currentIC->filename << " on line " << currentIC->lineno << "\n";
    }
    ICode *previous = currentIC;
    currentIC = currentIC->getNext();
    return previous;
}

void genPBlazeCode(ICode *lic) {
    currentIC = lic;

    while (currentIC) {
        stepIC();
    }
}
