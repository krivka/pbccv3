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

Register* ICode::getRegister() {
    // TODO clearUnusedOpFromReg

    Register *reg = Bank::current()->getFirstFree();
}
