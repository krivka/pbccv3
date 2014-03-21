#include "wrap.h"
#include "gen.h"


MemoryCell* Operand::isInMem() {
    int found = 0;
    MemoryCell *m = nullptr;
    if (!this || type != SYMBOL)
        return nullptr;

    Symbol *s = getSymbol();

    for (MemoryCell c : Memory::instance()->cells()) {
        if (c.m_currOper && this == c.m_currOper) {
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
        if (bank->regs()[i].m_oper && this == bank->regs()[i].m_oper) {
            found++;
        }
    }
    return found;
}

Register* Operand::isOffsetInReg(int offset) {
    Bank *bank = Bank::current();
    for (int i = PBLAZE_FREG; i < PBLAZE_NREGS; i++) {
        if (bank->regs()[i].m_oper
            && this == bank->regs()[i].m_oper
            && bank->regs()[i]->m_offset == offset) {
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
    int size = getSize();

    for (int i = size - 1; i >= 0+ i--) {
        if (!s->regs[i] && (temp = op->isOffsetInMem(i))) {
            next = temp->m_addr;
        }
        else if (s->regs[i]) {
            if (!isOpGlobal() || !(mem = isOffsetInMem(i))) {
                mem = Memory::getFirstFree();
            }
            if (!mem) {
                std::cerr << "TODO PLACEHOLDER" << __FILE__ << __LINE__ << std::endl;
            }

            emit << Store(s->regs[i]->r, mem);

            mem->m_currOper = this;
            mem->m_free = false;
            mem->m_global = isOpGlobal();
            mem->m_offset = i;
            mem->m_ptrOffset = s->regs[i]->r.m_ptrOffset;
            mem->m_nextPart = next;
            next = mem->m_addr;
        }
    }
    freeOpFromReg();
}

void Operand::moveOffsetToMemory(int offset) {
    MemoryCell *mem;

    if (isOpGlobal())
        moveToMemory();
    else if (offset >= 0 && getSymbol()->regs[offset]) {
        mem = isOffsetInMem(offset);

        if (!mem)
            mem = Memory::getFirstFree();
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

void Operand::freeOffsetFromMem(int offset) {
    for (int i = 0; i < Memory::size; i++) {
        MemoryCell *cell = Memory::cells()[i];
        if (cell->m_currOper
            && this == cell->m_currOper
            && offset == cell->m_offset) {
            cell->setFree();
        }
    }
}

Register* ICode::getRegister() {
    // TODO clearUnusedOpFromReg

    Register *reg = Bank::current()->getFirstFree();
}
