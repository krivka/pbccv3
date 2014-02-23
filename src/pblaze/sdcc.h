#ifndef _SDCC_H
#define _SDCC_H

#include "common.h"
#include "SDCCicode.h"
#include "../SDCCicode.h"
#include <SDCCsymt.h>

namespace PBCC {

    class SymLink : public ::sym_link {
    public:
    };

    class Symbol : public ::symbol {
    public:
    };

    class Operand : public ::operand {
    public:
        Symbol *getSymbol() {
            return static_cast<Symbol*>(OP_SYMBOL(((::operand*)this)));
        }
        bool isSymOp() {
            return IS_SYMOP(((::operand*)this));
        }
        SymLink *getType() {
            return static_cast<SymLink*>(operandType(((::operand*)this)));
        }
    };
 
    class ICode : public ::iCode {
    public:
        Operand *getLeft() {
            return static_cast<Operand*>(IC_LEFT(((::iCode*)this)));
        }
        Operand *getRight() {
            return static_cast<Operand*>(IC_RIGHT(((::iCode*)this)));
        }
        Operand *getResult() {
            return static_cast<Operand*>(IC_RESULT(((::iCode*)this)));
        }
        Symbol *getLabel() {
            return static_cast<Symbol*>(IC_LABEL(((::iCode*)this)));
        }
        ICode *getNext() {
            return static_cast<ICode*>(this->next);
        }
    };
    
}

#endif // _SDCC_H