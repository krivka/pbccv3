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
    int regsUsed = 0;
    if (0 == strcmp(sym->name, "main"))
        Bank::choose(true);
    else
        Bank::choose(false);
    emit << "\n; Function " << sym->name << ", arguments: [";
    Value *v = (Value*) ic->getLeft()->getSymbol()->getType()->funcAttrs.args;
    while (v) {
        emit << v->sym->name;
        if (regsUsed + v->getType()->getSize() < VAR_ARGS) {
            emit << ":{";
            for (int i = regsUsed; i < regsUsed + v->getType()->getSize(); i++)
                emit << Bank::current()->regs()[i].getName() << ",";
            emit << "}";
        }
        emit << ",";
        regsUsed += v->getType()->getSize();
        v = v->getNext();
    }
    emit << "]\n" << sym << ":\n";
    Bank::current()->purge();
}

void Call(ICode *ic) {
    bool isMain = false;
    ICode *currentFunc = ic;
    while(currentFunc) {
        if (currentFunc->op == FUNCTION)
            break;
        currentFunc = currentFunc->getPrev();
    }
    if (currentFunc && 0 == strcmp(currentFunc->getLeft()->getSymbol()->name, "main"))
        isMain = true;
    Stack::instance()->callFunction();
    if (isMain)
        Bank::swap();
    emit << I::Call(ic->getLeft()->getSymbol());
    if (isMain)
        Bank::swap();
    // treat the previous variables as invalid (stored on stack)
    for (int i = 0; i < VAR_REG_CNT; i++) {
        if (Bank::current()->regs()[i].m_oper)
            Bank::current()->regs()[i].m_oper->getSymbol()->regs[Bank::current()->regs()[i].m_index] = nullptr;
        Bank::current()->regs()[i].purge();
    }
    for (int i = 0; i < ic->getResult()->getType()->getSize(); i++) {
        Register *reg = &Bank::current()->regs()[i];
        ic->getResult()->getSymbol()->regs[i] = reg;
        reg->occupy(ic->getResult(), i);
    }
    Stack::instance()->returnFromFunction();
}

void Return(ICode *ic) {
    Operand *left = ic->getLeft();

    // First store any changes to variables that will live longer than scope of this function
    for (int i = 0; i < VAR_REG_CNT; i++) {
        Register *reg = &Bank::current()->regs()[i];
        if (reg && reg->m_oper && reg->m_oper->liveTo() > ic->seq) {
            reg->clear();
        }
    }

    // If this function returns anything, put it in the first register(s)
    if (left) {
        emit << "\t; Return starting:\n";
        for (Emitter::i = 0; Emitter::i < left->getType()->getSize(); Emitter::i++) {
            // already there
            if (left->isSymOp() && left->getSymbol()->regs[Emitter::i]->sX == Emitter::i)
                continue;

            if (Bank::current()->contains(ic->getLeft(), Emitter::i)) {
                emit << I::Load(&Bank::current()->regs()[Emitter::i], left);
            }
            else {
                if (left->isSymOp())
                    emit << I::Fetch(&Bank::current()->regs()[Emitter::i], left);
                else
                    emit << I::Load(&Bank::current()->regs()[Emitter::i], left);
            }
        }
    }
}

void EndFunction(ICode *ic) {
    emit << I::Ret();
}

void Send(ICode *ic) {
    static unsigned lastReg = 0;
    static ICode *callIC = nullptr;
    static bool isMain = false;
    if (!ic->getPrev() || ic->getPrev()->op != SEND) {
        ICode *currentFunc = ic;
        callIC = ic;
        while(callIC) {
            callIC = callIC->getNext();
            if (callIC->op == CALL)
                break;
        }
        emit << "\t; Call " << callIC->getLeft()->friendlyName() << " starting:\n";
        while(currentFunc) {
            if (currentFunc->op == FUNCTION)
                break;
            currentFunc = currentFunc->getPrev();
        }
        if (currentFunc && 0 == strcmp(currentFunc->getLeft()->getSymbol()->name, "main"))
            isMain = true;
        else
            isMain = false;
        lastReg = 0;
    }
    if (!isMain) {
        for (int i = 0; i < ic->getLeft()->getType()->getSize(); i++) {
            Register *reg = &Bank::current()->regs()[lastReg];
            reg->clear();
            // symbol
            if (ic->getLeft()->isSymOp()) {
                reg->occupy(ic->getLeft(), i);
                ic->getLeft()->getSymbol()->regs[i] = reg;
                if (Bank::current()->contains(ic->getLeft(), i))
                    emit << I::Load(reg, ic->getLeft());
                else
                    emit << I::Fetch(reg, ic->getLeft());
            }
            // literal
            else {
                Emitter::i = i;
                emit << I::Load(reg, ic->getLeft());
            }
            lastReg++;
        }
    }
    else {
        Bank::star(ic->getLeft(), &lastReg);
    }
    if (ic->getNext() && ic->getNext()->op != SEND) {
        // put the rest of the registers on the current stack
        for( ; lastReg < VAR_REG_CNT; lastReg++) {
            Register *reg = &Bank::current()->regs()[lastReg];
            if (reg && reg->m_oper && reg->m_oper->liveTo() > ic->seq) {
                Symbol *sym = reg->m_oper->getSymbol();
                int index = reg->m_index;
                reg->clear();
                sym->regs[index] = nullptr;
            }
        }
    }
}

void Receive(ICode *ic) {
    for (int i = 0; i < ic->getResult()->getType()->getSize(); i++) {
        Register *reg = &Bank::current()->regs()[ic->argreg - 1 + i];
        reg->occupy(ic->getResult(), i);
        ic->getResult()->getSymbol()->regs[i] = reg;
    }
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

    // stuffing parameters in the call
    if (ic->isInFunctionCall()) {
        // Don't bother, these go on stack
    }
    // assignment to a pointer
    else if (ic->isPointerSet()) {
        emit << "TODO: assignment to a pointer\n";
    }
    // assignment to temporary local variable
    else if (right->isITmp()) {
        for (int i = 0; i < right->getType()->getSize(); i++) {
            result->getSymbol()->regs[i] = right->getSymbol()->regs[i];
            result->getSymbol()->regs[i]->m_oper = result;
            right->getSymbol()->regs[i] = nullptr;
        }
    }
    // assignment to a regular local variable
    else {
        for (Emitter::i = 0; Emitter::i < result->getType()->getSize(); Emitter::i++) {
            emit << I::Load(result, right);
        }
    }
}

void AddressOf(ICode *ic) {

}

void Cast(ICode *ic) {
    // Don't forget about bool -> byte casting
    emit << "; casting " << ic->getRight()->getSymbol()->rname << " to " << ic->getResult()->getSymbol()->rname << "\n";
    emit << "; right: size: " << ic->getRight()->getType()->getSize() << "\n";
    emit << "; result: size: " << ic->getResult()->getType()->getSize() << "\n";
}

void Add(ICode *ic) {
    Operand *result = ic->getResult();
    Operand *left = ic->getLeft();
    Operand *right = ic->getRight();

    if (*result != *left &&
        !(ic->getNext()->op == '=' && *ic->getNext()->getResult() == *left && *ic->getNext()->getRight() == *result)) {
        for (Emitter::i = 0; Emitter::i < left->getType()->getSize(); Emitter::i++) {
            emit << I::Load(result, left);
        }
    }
    else {
        result = left;
    }
    for (Emitter::i = 0; Emitter::i < left->getType()->getSize(); Emitter::i++) {
        emit << I::Add(result, right);
    }
}

void Sub(ICode *ic) {
    Operand *result = ic->getResult();
    Operand *left = ic->getLeft();
    Operand *right = ic->getRight();

    if (*result != *left &&
        !(ic->getNext()->op == '=' && *ic->getNext()->getResult() == *left && *ic->getNext()->getRight() == *result)) {
        for (Emitter::i = 0; Emitter::i < left->getType()->getSize(); Emitter::i++) {
            emit << I::Load(result, left);
        }
    }
    else {
        result = left;
    }
    for (Emitter::i = 0; Emitter::i < left->getType()->getSize(); Emitter::i++) {
        emit << I::Sub(result, right);
    }
}

void Ifx(ICode *ic) {
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
    for (Emitter::i = 0; Emitter::i < ic->getLeft()->getType()->getSize(); Emitter::i++) {
        emit << I::Compare(ic->getLeft(), ic->getRight());
    }
}

void CmpLt(ICode *ic) {
    for (Emitter::i = 0; Emitter::i < ic->getLeft()->getType()->getSize(); Emitter::i++) {
        emit << I::Compare(ic->getLeft(), ic->getRight());
    }
    Emitter::i = 0;
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
    { ADDRESS_OF, AddressOf },
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
