#include "wrap.h"
#include "gen.h"

#include <iostream>

bool ICode::isUsedInCurrentInstruction(Operand* op) {
    if (getLeft() && getLeft() == op)
        return true;
    if (getRight() && getRight() == op)
        return true;
    if (getResult() && getResult() == op)
        return true;
    return false;
}

bool Operand::isValid() {
    if (isSymOp()) {
        // check just the first byte
        if (Stack::instance()->contains(this, 0) || Memory::get()->containsStatic(this, 0) || this->getSymbol()->regs[0])
            return true;
        return false;
    }
    else if (isLiteral) {
        return true;
    }
    cerr << "Unknown Operand type!" << endl;
    return false;
}
