#include "ralloc.h"
#include "gen.h"
#include <iostream>

Allocator *Allocator::_self = nullptr;
int Allocator::seq = 0;

Allocator::Allocator() {
}

void Allocator::assignRegisters(EbbIndex* ebbi) {
    if (!_self)
        _self = new Allocator();
    genPBlazeCode(ebbi);
}

void pblaze_assignRegisters(ebbIndex * ebbi) {
    return Allocator::assignRegisters(static_cast<EbbIndex*>(ebbi));
}

void pblaze_genCodeLoop(void) {

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

void Stack::preCall() {
    int emitterStorage = Emitter::i;
    Emitter::i;
    int diff = m_lastValue - m_offset;
    if (diff > 0) {
        emit << I::Add(Bank::currentStackPointer(), diff);
    }
    else {
        emit << I::Sub(Bank::currentStackPointer(), -diff);
    }
    m_offset = m_lastValue;
    Emitter::i = emitterStorage;
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
    int emitterStorage = Emitter::i;
    Emitter::i = index;
    StackCell *loc = contains(op, index);
    if (loc) {
        int diff = loc->m_pos - m_offset + index;
        if (diff > 0) {
            emit << I::Add(Bank::currentStackPointer(), diff);
        }
        else if (diff < 0) {
            emit << I::Sub(Bank::currentStackPointer(), -diff);
        }
        m_offset += diff;
    }
    else {
        int diff = m_lastValue - m_offset + index;
        if (diff)
            emit << I::Add(Bank::currentStackPointer(), diff);
        m_offset += diff;
        // to reserve the whole space
        for (int i = 0; i < op->getType()->getSize(); i++) {
            m_mem[m_lastValue+i].m_oper = op;
            m_mem[m_lastValue+i].m_index = i;
        }
        m_lastValue += op->getType()->getSize();
    }
    emit << I::Store(op);
    Emitter::i = emitterStorage;
}

void Stack::fetchVariable(Operand* op, int index) {
    int emitterStorage = Emitter::i;
    StackCell *cell = contains(op, index);
    if (!cell) {
        cerr << "Warning: Can't find variable" << op->getSymbol()->rname << "on stack!\n";
        return;
    }
    int diff = cell->m_pos - m_offset + index;
    m_offset += diff;
    if (diff > 0)
        emit << I::Add(Bank::currentStackPointer(), diff);
    else
        emit << I::Sub(Bank::currentStackPointer(), -diff);
    Emitter::i = emitterStorage;
}

void Stack::loadAddress(Operand* res, Operand* obj) {
    if (!contains(obj, 0)) {
        // reserve
        for (int i = 0; i < obj->getType()->getSize(); i++) {
            m_mem[m_lastValue+i].m_oper = obj;
            m_mem[m_lastValue+i].m_index = i;
        }
        m_lastValue += obj->getType()->getSize();
    }
    int emitterStorage = Emitter::i;
    Emitter::i = 0;
    emit << I::Load(res, Bank::currentStackPointer());
    emit << I::Add(res->getSymbol()->regs[0], (uint8_t) m_lastValue - obj->getType()->getSize());
    Emitter::i = emitterStorage;
}

StackCell* Stack::contains(Operand* o, int index) {
    for (int i = 0; i < m_lastValue; i++) {
        if(*m_mem[i].m_oper == *o)
            return &m_mem[i];
    }
    return nullptr;
}




Bank Bank::m_banks[2];
bool Bank::m_first = true;

void Bank::swap() {
    emit << I::RegBank(m_first ? I::RegBank::B : I::RegBank::A);
    m_first = m_first ? false : true;
}

Register* Bank::getFreeRegister() {
    // first look for a free register
    for (int i = VAR_REG_START; i < VAR_REG_END; i++) {
        if (m_regs[i].isFree())
            return &m_regs[i];
    }

    // then look for a register containing a dead variable
    for (int i = VAR_REG_START; i < VAR_REG_END; i++) {
        if (m_regs[i].m_oper && m_regs[i].m_oper->getSymbol()->liveTo < Allocator::seq) {
            m_regs[i].clear();
            return &m_regs[i];
        }
    }

    // last, look for a variable that is alive for the longest time
    // TODO: not optimal, improve
    int latest = INT_MAX;
    Register *toFree = m_regs;

    for (int i = VAR_REG_START; i < VAR_REG_END; i++) {
        if (m_regs[i].m_oper && m_regs[i].m_oper->getSymbol()->liveTo < latest) {
            toFree = &m_regs[i];
            latest = m_regs[i].m_oper->getSymbol()->liveTo;
        }
    }
    toFree->clear();
    return toFree;
}

void Bank::purge() {
    for (int i = 0; i < VAR_REG_END; i++) {
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
    if (!m_oper || !m_oper->isSymOp() || m_oper->getSymbol()->regs[m_index] != this) 
        return;

    moveToMemory();
    Byte::clear();
}

bool reg_info::containsLive(ICode *ic) {
    if (m_oper && ic->seq > m_oper->liveTo())
        return false;
    return true;
}

void reg_info::moveToMemory() {
    if (m_oper->liveTo() >= Allocator::seq) {
        if (Memory::get()->containsStatic(m_oper, m_index)) {
            Operand *staticSym = (Operand *) newiTempOperand(newCharLink(), 0);
            emit << I::Load(staticSym, Memory::get()->containsStatic(m_oper, m_index)->m_pos);
        }
        else {
            Stack *stack = Stack::instance();
            stack->pushVariable(m_oper, m_index);
        }
    }
    m_oper->getSymbol()->regs[m_index] = nullptr;
}

