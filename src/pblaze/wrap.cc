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
