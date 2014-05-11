#include "ralloc.h"
#include "gen.h"
#include <iostream>

Allocator *Allocator::_self = nullptr;

Allocator::Allocator() {
}

void Allocator::assignRegisters(EbbIndex* ebbi) {
    if (!_self)
        _self = new Allocator();

    EbBlock **ebbs = ebbi->getBbOrder();
    int count = ebbi->getCount();
    ICode *ic = ICode::fromEbBlock(ebbs, count);
    genPBlazeCode(ic);
}

void pblaze_assignRegisters(ebbIndex * ebbi) {
    return Allocator::assignRegisters(static_cast<EbbIndex*>(ebbi));
}

void pblaze_genCodeLoop(void) {

}




void MemoryCell::clear(reg_info *reg) {
    int tempI = Emitter::i;
    Emitter::i = reg->m_index;
    emit << I::Fetch(reg->m_oper, m_pos);
    Emitter::i = tempI;
    Byte::clear();
}

MemoryCell* Memory::containsStatic(Operand* o, int index) {
    for (MemoryCell &c : m_cells) {
        if (*c.m_oper == *o && c.m_index == index) {
            return &c;
        }
    }
    return nullptr;
}




Memory *Memory::_self = nullptr;

void Memory::allocateGlobal(Operand* o) {
    for (int i = 0; i < o->getSymbol()->getType()->getSize(); i++) {
        occupy(o, i);
    }
}



Stack *Stack::_self = nullptr;

Stack* Stack::instance() {
    if (!_self)
        _self = new Stack;
    return _self;
}

void Stack::functionStart() {
    m_offset = 0;
    m_lastValue = 0;
}

void Stack::functionEnd() {
    if (m_offset)
        emit << I::Sub(Bank::currentStackPointer(), m_offset);
}

void Stack::pushVariable(Operand* op, int index) {
    StackCell *loc = contains(op, index);
    if (loc) {
        int diff = loc->m_pos - m_offset;
        if (diff > 0) {
            emit << I::Add(Bank::currentStackPointer(), diff);
        }
        else if (diff < 0) {
            emit << I::Sub(Bank::currentStackPointer(), diff);
        }
    }
    else {
        int diff = m_lastValue - m_offset;
        if (diff)
            emit << I::Add(Bank::currentStackPointer(), diff);
        m_mem[m_lastValue].m_oper = op;
        m_mem[m_lastValue].m_index = index;
        m_lastValue++;
    }
    emit << I::Store(op);
}

StackCell* Stack::contains(Operand* o, int index) {
    
}




Bank Bank::m_banks[2];
bool Bank::m_first = true;

void Bank::swap() {
    emit << I::RegBank(m_first ? I::RegBank::B : I::RegBank::A);
    m_first = m_first ? false : true;
}

Register* Bank::getFreeRegister(int seq) {
    // first look for a free register
    for (int i = 0; i < VAR_REG_CNT; i++) {
        if (m_regs[i].isFree())
            return &m_regs[i];
    }

    // then look for a register containing a dead variable
    for (int i = 0; i < VAR_REG_CNT; i++) {
        if (m_regs[i].m_oper && m_regs[i].m_oper->getSymbol()->liveTo < seq) {
            m_regs[i].clear();
            return &m_regs[i];
        }
    }

    // last, look for a variable that is alive for the longest time
    // TODO: not optimal, improve
    int latest = INT_MAX;
    Register *toFree = m_regs;

    for (int i = 0; i < VAR_REG_CNT; i++) {
        if (m_regs[i].m_oper && m_regs[i].m_oper->getSymbol()->liveTo < latest) {
            toFree = &m_regs[i];
            latest = m_regs[i].m_oper->getSymbol()->liveTo;
        }
    }
    cerr << "WILL FREE " << toFree->getName() << "\n";
    toFree->clear();
    return toFree;
}

void Bank::purge() {
    for (int i = 0; i < VAR_REG_CNT; i++) {
        m_regs[i].purge();
    }
}

reg_info* Bank::contains(Operand* o, int index) {
    for (int i = 0; i < REG_CNT; i++) {
        if (*m_regs[i].m_oper == *o && m_regs[i].m_index == index)
            return &m_regs[i];
    }
    return nullptr;
}

void Bank::star(Operand* o, unsigned *firstReg) {
    static Operand *starSym = nullptr;
    Bank *bank = other();
    if (!o->isSymOp()) {
        if (!starSym)
            starSym = (Operand *) newiTempOperand(o->getType(), 0);
        for (Emitter::i = 0; Emitter::i < o->getType()->getSize(); Emitter::i++) {
            emit << I::Load(starSym, o);
            emit << I::Star(&bank->m_regs[*firstReg], starSym);
            starSym->getSymbol()->regs[Emitter::i]->purge();
            (*firstReg)++;
        }
    }
    else {
        for (Emitter::i = 0; Emitter::i < o->getType()->getSize(); Emitter::i++) {
            emit << I::Star(&bank->m_regs[*firstReg], o);
            (*firstReg)++;
        }
    }
}


void reg_info::purge() {
    Byte::clear();
}

void Register::clear() {
    if (!m_oper || !m_oper->isSymOp() || !m_oper->getSymbol()->regs[m_index]) 
        return;

    MemoryCell *cell = Memory::get()->occupy(m_oper, m_index);

    int tempI = Emitter::i;
    Emitter::i = m_index;
    emit << I::Store(m_oper->getSymbol()->regs[m_index], cell->m_pos);
    Emitter::i = tempI;
    m_oper->getSymbol()->regs[m_index] = nullptr;

    Byte::clear();
}

bool reg_info::containsLive(ICode *ic) {
    if (m_oper && ic->seq > m_oper->liveTo())
        return false;
    return true;
}

void reg_info::moveToMemory() {

}

