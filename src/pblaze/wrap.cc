#include "wrap.h"
#include "gen.h"

#include <iostream>

MemoryCell* Operand::isInMem() {
    int found = 0;
    MemoryCell *m = nullptr;
    if (!this || type != SYMBOL)
        return nullptr;

    Symbol *s = getSymbol();

    for (MemoryCell c : Memory::instance()->cells()) {
        if (c.m_currOper && *this == *c.m_currOper) {
            found++;
            if (!m)
                m = &c;
        }
    }

    if (found == getSymbol()->getType()->getSize())
        return m;
    return nullptr;
}

bool ICode::isUsedInCurrentInstruction(Operand* op) {
    if (getLeft() && getLeft() == op)
        return true;
    if (getRight() && getRight() == op)
        return true;
    if (getResult() && getResult() == op)
        return true;
    return false;
}

int Operand::isOpInReg() {
    int found = 0;
    Bank *bank = Bank::current();
    for (int i = PBLAZE_FREG; i < PBLAZE_NREGS; i++) {
        if (bank->regs()[i].m_oper && *this == *bank->regs()[i].m_oper) {
            found++;
        }
    }
    return found;
}

MemoryCell* Operand::isOffsetInMem(int offset) {
    if (type != SYMBOL)
        return nullptr;

    for (int i = 0; i < Memory::size; i++) {
        MemoryCell *cell = &Memory::instance()->cells()[i];
        if (cell->m_currOper && *this == *cell->m_currOper && offset == cell->m_offset) {
            return cell;
        }
    }
    return nullptr;
}

Register* Operand::isOffsetInReg(int offset) {
    Bank *bank = Bank::current();
    for (int i = PBLAZE_FREG; i < PBLAZE_NREGS; i++) {
        if (bank->regs()[i].m_oper
            && *this == *bank->regs()[i].m_oper
            && bank->regs()[i].m_offset == offset) {
            return &bank->regs()[i];
        }
    }
    return nullptr;
}

void Operand::testOperand(int free, bitVect* rUse) {
    Register *reg;
    int sizeInReg = isOpInReg();
    int remainOp = bitVectnBitsOn(rUse);
    if (sizeInReg > 0 && remainOp - sizeInReg >= free) {
        int size = getType()->getSize();

        for (int i = 0; i < size; i++) {
            reg = isOffsetInReg(i);
            if (reg) {
                bitVectUnSetBit(rUse, reg->m_index);
            }
        }
    }
}

void Operand::moveToMemory() {
    MemoryCell *mem, *temp;
    Symbol *s = getSymbol();
    int next = -1;
    int size = s->getType()->getSize();

    for (int i = size - 1; i >= 0; i--) {
        if (!s->regs[i] && (temp = isOffsetInMem(i))) {
            next = temp->m_addr;
        }
        else if (s->regs[i]) {
            if (!isOpGlobal() || !(mem = isOffsetInMem(i))) {
                mem = Memory::instance()->getFirstFree();
            }
            if (!mem) {
                std::cerr << "TODO PLACEHOLDER" << __FILE__ << __LINE__ << std::endl;
            }

            emit << I::Store(s->regs[i]->r, mem);

            mem->m_currOper = this;
            mem->m_free = false;
            mem->m_global = isOpGlobal();
            mem->m_offset = i;
            mem->m_ptrOffset = s->regs[i]->r.m_ptrOffset;
            mem->m_nextPart = next;
            next = mem->m_addr;
        }
    }
    freeFromReg();
}

void Operand::moveOffsetToMemory(int offset) {
    MemoryCell *mem;

    if (isOpGlobal())
        moveToMemory();
    else if (offset >= 0 && getSymbol()->regs[offset]) {
        mem = isOffsetInMem(offset);

        if (!mem)
            mem = Memory::instance()->getFirstFree();
        if (!mem)
            std::cerr << "TODO PLACEHOLDER" << __FILE__ << __LINE__ << std::endl;

        emit << I::Store(getSymbol()->regs[offset]->r, mem);

        mem->m_currOper = this;
        mem->m_free = false;
        mem->m_global = isOpGlobal();
        mem->m_offset = offset;
        mem->m_ptrOffset = getSymbol()->regs[offset]->r.m_ptrOffset;

        MemoryCell *temp = Memory::containsOffset(this, offset + 1);
        if (temp)
            mem->m_nextPart = temp->m_addr;
        else
            mem->m_nextPart = -1;

        freeOffsetFromReg(offset);
    }
}

void Operand::freeFromMemory() {
    for (int i = 0; i < Memory::size; i++) {
        MemoryCell *cell = &Memory::instance()->cells()[i];
        if (*this == *cell->m_currOper) {
            cell->setFree();
        }
    }
}

void Operand::freeOffsetFromMem(int offset) {
    for (int i = 0; i < Memory::size; i++) {
        MemoryCell *cell = &Memory::instance()->cells()[i];
        if (cell->m_currOper
            && *this == *cell->m_currOper
            && offset == cell->m_offset) {
            cell->setFree();
        }
    }
}

void Operand::freeFromReg() {
    for (int i = 0; i < getType()->getSize(); i++) {
        Symbol *s = getSymbol();
        if (s->regs[i]) {
            Register *reg = s->regs[i]->r;
            s->regs[i] = NULL;
            reg->setFree();
        }
    }
}

void Operand::freeOffsetFromReg(int offset) {
    Symbol *s = getSymbol();

    if (s->regs[offset]) {
        Register *reg = &s->regs[offset]->r;
        reg->setFree();
    }
}

Register* ICode::getRegister() {
    // TODO clearUnusedOpFromReg

    Register *reg = Bank::current()->getFirstFree();
}

int ICode::pointerSetOpt(int currOffset) {
    if (!isPointerSet() || !getNext() || !getNext()->getNext() || !getPrev())
        return 0;

    int pOffset, nOffset;

    if (*getResult() == *getPrev()->getResult()
        && getResult()->liveTo() == seq) {
        if (getPrev()->op == '+' && getPrev()->getRight()->isLiteral) {
            pOffset = getPrev()->getRight()->getValue()->getUnsignedLong();
        }
        else if (getPrev()->op == ADDRESS_OF) {
            pOffset = 0;
        }
        else {
            return 0;
        }
    }
    else {
        return 0;
    }

    if (getNext()->op == '+'
        && getNext()->getNext()->isPointerSet()
        && *getNext()->getResult() == *getNext()->getNext()->getResult()
        && getNext()->getNext()->getResult()->liveTo() == seq + 2) {
        if (getNext()->getRight()->isLiteral) {
            nOffset = getNext()->getRight()->getValue()->getUnsignedLong();
        }
        else {
            return 0;
        }
    }
    else {
        return 0;
    }

    // TODO ASSIGN OPTIMIZATION
    emit << I::Add(this, nOffset - pOffset - currOffset);
    Allocator::updateOpInMem(this, getNext()->getResult(), 0);
    getNext()->generated = true;
    return 1;
}
