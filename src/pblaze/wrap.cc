#include "wrap.h"

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

Register* ICode::getRegister() {
    // TODO clearUnusedOpFromReg

    Register *reg = Bank::current()->getFirstFree();
}
