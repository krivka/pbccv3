#include "gen.h"
#include "util.h"
#include "wrap.h"
#include <map>

stringstream& operator<<(stringstream &ss, Operand *o) {
    int i = Emitter::i;
    if (o->getType()->isFunc()) {
        ss << o->getSymbol()->rname;
        if (!i)
            ss << " & 0xFF";
        else
            ss << " >> 8";
    }
    else if (o->isLiteral) {
        ss << "0x" << std::hex << std::uppercase << ((o->getValue()->getUnsignedLong() & (0xFF << (i << 3))) >> (i << 3));
    }
    else if (o->isSymOp()) {
        if (!o->getSymbol()->regs[i]) {
            Register *reg = Bank::current()->getFreeRegister();
            reg->occupy(o, i);
            o->getSymbol()->regs[i] = reg;
            if (Stack::instance()->contains(o, i)) {
                Stack::instance()->fetchVariable(o, i);
                emit << I::Fetch(reg, Bank::currentStackPointer());
            }
            else if (Memory::get()->containsStatic(o, i)) {
                emit << I::Fetch(o, Memory::get()->containsStatic(o, i)->m_pos);
            }
        }
        ss << o->getSymbol()->regs[i];
    }
    else {
        std::cerr << "Unknown Emitter Operand type!\n";
    }
    Emitter::i = i;
}

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
    Function::processNew(ic);
    // print the parameters
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
    // purge the bank
    Bank::current()->purge();
    // initialize the stack
    Stack::instance()->functionStart();
}

/* Calling conventions:
 *
 * Current registers are stored on stack
 *  - If current function is main, we need to store only enough space for the result
 *  - else, store everything
 * Registers are filled with arguments
 * Stack pointer is propagated
 *  * Banks are switched
 * Function is called
 *  * Return value is pulled from the other bank
 *  * Banks are switched
 *
 */
void Call(ICode *ic) {
    if (0 == strcmp(ic->getLeft()->friendlyName(), "port_out")) {
        Operand *value = ic->getPrev()->getPrev()->getLeft();
        Operand *port = ic->getPrev()->getLeft();
        Emitter::i = 0;
        if (!value->isSymOp() && !port->isSymOp() && port->getValue()->getUnsignedLong() < 0xF) {
            emit << I::OutputK(value, port);
        }
        else {
            if (port->isSymOp()) {
                cerr << "Only constant port number is supported now, sorry";
                return;
            }
            emit << I::Output(value, port);
        }
        return;
    }
    if (0 == strcmp(ic->getLeft()->friendlyName(), "port_in")) {
        Operand *value = ic->getPrev()->getPrev()->getLeft();
        Operand *port = ic->getPrev()->getLeft();
        Emitter::i = 0;
        if (port->isSymOp()) {
            cerr << "Only constant port number is supported now, sorry";
            return;
        }
        emit << I::Input(value, port);
        return;
    }
    if (Function::isMain) {
        if (ic->op == PCALL) {
            for (Emitter::i = 0; Emitter::i < ic->getLeft()->getType()->getSize(); Emitter::i++) {
                emit << I::Star(&Bank::current()->regs()[VAR_REG_CNT-1-Emitter::i], ic->getLeft());
            }
        }
        // treat the previous variables as invalid (stored on stack)
        for (int i = 0; i < ic->getResult()->getType()->getSize(); i++) {
            Bank::current()->regs()[i].clear();
        }
        // propagate the current stack pointer
        emit << I::Star(&Bank::current()->regs()[REG_CNT-1], &Bank::current()->regs()[REG_CNT-1]);
        Bank::swap();
    }
    else {
        unsigned argcnt = 0;
        Value *args = (Value*) ic->getLeft()->getType()->funcAttrs.args;
        while (args) {
            if (argcnt + args->getType()->getSize() < VAR_REG_CNT)
                argcnt += args->getType()->getSize();
            else
                break;
            args = args->getNext();
        }
        for( ; argcnt < VAR_REG_CNT; argcnt++) {
            Register *reg = &Bank::current()->regs()[argcnt];
            if (reg && reg->m_oper && reg->m_oper->liveTo() > ic->seq) {
                Symbol *sym = reg->m_oper->getSymbol();
                int index = reg->m_index;
                reg->clear();
                sym->regs[index] = nullptr;
            }
        }
        if (ic->op == PCALL) {
            if (argcnt >= VAR_REG_CNT - 2) {
                cerr << "Warning: You're calling a pointer (" << ic->getLeft() << ") to a function that has too many arguments. This will result in undefined behavior.\n";
            }
            for (Emitter::i = 0; Emitter::i < ic->getLeft()->getType()->getSize(); Emitter::i++) {
                emit << I::Load(&Bank::current()->regs()[VAR_REG_CNT-1-Emitter::i], ic->getLeft());
                if (ic->getLeft()->getSymbol()->regs[Emitter::i]) {
                    ic->getLeft()->getSymbol()->regs[Emitter::i]->purge();
                }
                ic->getLeft()->getSymbol()->regs[Emitter::i] = &Bank::current()->regs()[VAR_REG_CNT-1-Emitter::i];
            }
        }
    }
    emit << I::Call(ic);
    if (Function::isMain) {
        for (int i = 0; i < ic->getResult()->getType()->getSize(); i++) {
            emit << I::Star(&Bank::current()->regs()[i], &Bank::current()->regs()[i]);
        }
        Bank::swap();
    }
    else {
        // treat the previous variables as invalid (stored on stack)
        for (int i = 0; i < VAR_REG_CNT; i++) {
            if (Bank::current()->regs()[i].m_oper)
                Bank::current()->regs()[i].m_oper->getSymbol()->regs[Bank::current()->regs()[i].m_index] = nullptr;
            Bank::current()->regs()[i].purge();
        }
    }
    for (int i = 0; i < ic->getResult()->getType()->getSize(); i++) {
        Register *reg = &Bank::current()->regs()[i];
        ic->getResult()->getSymbol()->regs[i] = reg;
        reg->occupy(ic->getResult(), i);
    }
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
            if (left->isSymOp() && left->getSymbol()->regs[Emitter::i] && left->getSymbol()->regs[Emitter::i]->sX == Emitter::i)
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
    Stack::instance()->functionEnd();
    emit << I::Ret();
}

void EndFunction(ICode *ic) {
}

void Send(ICode *ic) {
    static unsigned lastReg = 0;
    static ICode *callIC = nullptr;
    if (!ic->getPrev() || ic->getPrev()->op != SEND) {
        callIC = ic;
        while(callIC) {
            if (callIC->op == CALL || callIC->op == PCALL) {
                // skip builtins
                if (0 == strcmp(callIC->getLeft()->friendlyName(), "port_out") ||
                    0 == strcmp(callIC->getLeft()->friendlyName(), "port_in")) {
                    return;
                }
                emit << "\t; Call " << callIC->getLeft()->friendlyName() << " starting:\n";
                break;
            }
            callIC = callIC->getNext();
        }
        lastReg = 0;
    }
    if (!Function::isMain) {
        for (int i = 0; i < ic->getLeft()->getType()->getSize(); i++) {
            Register *reg = &Bank::current()->regs()[lastReg];
            reg->clear();
            // symbol
            if (ic->getLeft()->isSymOp()) {
                reg->occupy(ic->getLeft(), i);
                Emitter::i = i;
                if (Bank::current()->contains(ic->getLeft(), i)) {
                    emit << I::Load(reg, ic->getLeft());
                }
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
    if (right->isSymOp() && !right->getType()->isFunc() && !Memory::get()->containsStatic(right, 0) && !right->getSymbol()->regs[0] && !Stack::instance()->contains(right, 0)) {
        return;
    }

    // stuffing parameters in the call
    if (ic->isInFunctionCall()) {
        // Don't bother, these go on stack
    }
    // assignment to a pointer
    else if (ic->isPointerSet()) {
        Operand *tmpOp = nullptr;
        if (right->isLiteral) 
            tmpOp = (Operand *) newiTempOperand(right->getType(), 0);
        for (Emitter::i = 0; Emitter::i < result->getType()->getSize(); Emitter::i++) {
            if (tmpOp) {
                emit << I::Load(tmpOp, right);
                emit << I::Store(result, tmpOp);
            }
            else {
                emit << I::Store(result, right);
            }
        }
    }
    // assignment from a temporary local variable
    else if (right->isITmp() && right->isSymOp()) {
        for (int i = 0; i < right->getType()->getSize(); i++) {
            if (!right->getSymbol()->regs[i]) {
                continue;
            }
            result->getSymbol()->regs[i] = right->getSymbol()->regs[i];
            result->getSymbol()->regs[i]->m_oper = result;
            right->getSymbol()->regs[i] = nullptr;
        }
        emit << "\t\t\t\t\t\t; " << result->friendlyName() << "=" << right->friendlyName() << "\n";
    }
    // assignment to a regular local variable
    else {
        for (Emitter::i = 0; Emitter::i < result->getType()->getSize(); Emitter::i++) {
            emit << I::Load(result, right);
        }
    }
}

void AddressOf(ICode *ic) {
    MemoryCell *memoryCell = Memory::get()->containsStatic(ic->getLeft(), 0);
    StackCell *stackCell = Stack::instance()->contains(ic->getLeft(), 0);
    if (memoryCell) {
        Emitter::i = 0;
        emit << I::Load(ic->getResult(), memoryCell->m_pos);
    }
    else {
        Stack::instance()->loadAddress(ic->getResult(), ic->getLeft());
    }
}

void AtAddress(ICode *ic) {
    for (Emitter::i = 0; Emitter::i < ic->getResult()->getType()->getSize(); Emitter::i++)
        emit << I::Fetch(ic->getResult(), ic->getLeft());
}

void Cast(ICode *ic) {
    if (!ic->getRight()->isITmp()) {
        for (Emitter::i = 0; Emitter::i < ic->getRight()->getType()->getSize(); Emitter::i++) {
            Register *reg = ic->getRight()->getSymbol()->regs[Emitter::i];
            reg->clear();
            reg->occupy(ic->getResult(), Emitter::i);
            ic->getResult()->getSymbol()->regs[Emitter::i] = reg;
        }
    }
    else {
        for (Emitter::i = 0; Emitter::i < ic->getRight()->getType()->getSize(); Emitter::i++) {
            if (ic->getRight()->getSymbol()->regs[Emitter::i]) {
                ic->getRight()->getSymbol()->regs[Emitter::i]->occupy(ic->getResult(), Emitter::i);
                ic->getResult()->getSymbol()->regs[Emitter::i] = ic->getRight()->getSymbol()->regs[Emitter::i];
                ic->getRight()->getSymbol()->regs[Emitter::i] = nullptr;
            }
            else {
                emit << I::Fetch(ic->getResult(), ic->getRight());
            }
        }
    }
    emit << "\t\t\t\t\t\t; Casting " << ic->getRight()->friendlyName() << " (" << ic->getRight()->getType()->getSize() << ") to " << ic->getResult()->friendlyName() << " (" << ic->getResult()->getType()->getSize() << ")\n";

    // fill the rest with zeroes
    for (Emitter::i = ic->getRight()->getType()->getSize(); Emitter::i < ic->getResult()->getType()->getSize(); Emitter::i++) {
        emit << I::Load(ic->getResult(), (uint8_t) 0);
    }
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


void Or(ICode *ic) {
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
        emit << I::Or(result, right);
    }
}

void And(ICode *ic) {
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
        emit << I::And(result, right);
    }
}

void Xor(ICode *ic) {
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
        emit << I::Xor(result, right);
    }
}

void Ifx(ICode *ic) {
    // if we depend on the result of the previous operation, we should already
    // have what we need in the zero flag (except LT/GT)
    if (ic->getPrev()->getResult() != ic->getCondition()) {
        for (Emitter::i = 0; Emitter::i < ic->getCondition()->getType()->getSize(); Emitter::i++) {
            emit << I::Test(ic->getCondition(), ic->getCondition());
        }
    }
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
            emit << I::Jump(ic->icTrue(), I::Jump::NZ);
        else
            emit << I::Jump(ic->icFalse(), I::Jump::Z);
    }
    Emitter::i = 0;
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
    { PCALL, Call },
    { SEND, Send },
    { RECEIVE, Receive },

    { LABEL, Label },
    { GOTO, GoTo },

    { '=', Assign },
    { ADDRESS_OF, AddressOf },
    { GET_VALUE_AT_ADDRESS, AtAddress },
    { CAST, Cast },

    { '+', Add },
    { '-', Sub },
    { '|', Or },
    { '&', And },
    { '^', Xor },

    { IFX, Ifx },
    { EQ_OP, CmpEq },
    { '<', CmpLt },

    { INLINEASM, InlineAsm },
};

};

ICode *stepIC() {
    Allocator::seq = currentIC->seq;
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

void genPBlazeCode(EbbIndex *ebbi) {
    EbBlock **ebbs = (EbBlock**) ebbi->dfOrder;
    int count = ebbi->getCount();
    currentIC = ICode::fromEbBlock(ebbs, count);

    while (currentIC) {
        stepIC();
    }
}
