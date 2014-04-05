#ifndef PBLAZE_WRAP_H
#define PBLAZE_WRAP_H
#ifdef __cplusplus

extern "C" {
#include "common.h"
#include "SDCCsymt.h"
#include "SDCCy.h"
#include "SDCCBBlock.h"
#include "SDCCicode.h"
}

#include "ralloc.h"
#include "util.h"
#include <iostream>

class EbbIndex;
class EbBlock;
class ICode;
class MemoryCell;
class Memory;
class Bank;

template<typename T>
class Set : public ::set {
public:
    T *firstItem() {
        return (T*) setFirstItem(this);
    }
    T *nextItem() {
        return (T*) setNextItem(this);
    }
    void addSet(T* item) {
        ::addSet(this, item);
    }
};

class EbbIndex : public ebbIndex {
public:
    EbBlock **getBbOrder() { return (EbBlock**) bbOrder; }
    int getCount() { return count; }
};

class EbBlock : public eBBlock {
public:
};

class SymLink : public ::sym_link {
public:
    unsigned int getSize() {
        return ::getSize((::sym_link*)this);
    }
    bool isDecl() {
        return IS_DECL(this);
    }
    bool isSpec() {
        return IS_SPEC(this);
    }
    bool isArray() {
        return IS_ARRAY(this);
    }
    bool isDataPtr() {
        return IS_DATA_PTR(this);
    }
    bool isSmallPtr() {
        return IS_SMALL_PTR(this);
    }
    bool isPtr() {
        return IS_PTR(this);
    }
    bool isPtrConst() {
        return IS_PTR_CONST(this);
    }
    bool isPtrRestrict() {
        return IS_PTR_RESTRICT(this);
    }
    bool isFarPtr() {
        return IS_FARPTR(this);
    }
    bool isCodePtr() {
        return IS_CODEPTR(this);
    }
    bool isGenPtr() {
        return IS_GENPTR(this);
    }
    bool isFuncPtr() {
        return IS_FUNCPTR(this);
    }
    bool isFunc() {
        return IS_FUNC(this);
    }
    bool isLong() {
        return IS_LONG(this);
    }
    bool isLongLong() {
        return IS_LONGLONG(this);
    }
    bool isUnsigned() {
        return IS_UNSIGNED(this);
    }
    bool isTypedef() {
        return IS_TYPEDEF(this);
    }
    /*
bool isConstant() {
return IS_CONSTANT(this);
}
*/
    /*
bool isRestrict() {
return IS_RESTRICT(this);
}
*/
    bool isStruct() {
        return IS_STRUCT(this);
    }
    bool isAbsolute() {
        return IS_ABSOLUTE(this);
    }
    bool isRegister() {
        return IS_REGISTER(this);
    }
    /*
bool isRent() {
return IS_RENT(this);
}
*/
    bool isStatic() {
        return IS_STATIC(this);
    }
    bool isInline() {
        return IS_INLINE(this);
    }
    bool isNoreturn() {
        return IS_NORETURN(this);
    }
    bool isInt() {
        return IS_INT(this);
    }
    bool isVoid() {
        return IS_VOID(this);
    }
    bool isBool() {
        return IS_BOOL(this);
    }
    bool isChar() {
        return IS_CHAR(this);
    }
    bool isExtern() {
        return IS_EXTERN(this);
    }
    /*
bool isVolatile() {
return IS_VOLATILE(this);
}
*/
    bool isIntegral() {
        return IS_INTEGRAL(this);
    }
    bool isBitfield() {
        return IS_BITFIELD(this);
    }
    bool isBitvar() {
        return IS_BITVAR(this);
    }
    bool isBit() {
        return IS_BIT(this);
    }
    bool isBoolean() {
        return IS_BOOLEAN(this);
    }
    bool isFloat() {
        return IS_FLOAT(this);
    }
    bool isFixed() {
        return IS_FIXED(this);
    }
    bool isArithmetic() {
        return IS_ARITHMETIC(this);
    }
    bool isAggregate() {
        return IS_AGGREGATE(this);
    }
    bool isLiteral() {
        return IS_LITERAL(this);
    }
    bool isCode() {
        return IS_CODE(this);
    }
    bool isRegparm() {
        return IS_REGPARM(this);
    }
};

class Symbol : public ::symbol {
public:
    SymLink *getType() {
        return (SymLink*) type;
    }
};

class Value : public ::value {
public:
    unsigned long getUnsignedLong() {
        return ulFromVal((::value*)this);
    }
};

class Operand : public ::operand {
public:
    bool operator==(Operand &other) {
        if (!this->isSymOp() || !other.isSymOp())
            return false;

        if (this->getSymbol() == other.getSymbol())
            return true;

        if (0 == strcmp(this->getSymbol()->rname, other.getSymbol()->rname))
            return true;

        return false;
    }
    bool operator!=(Operand &other) {
        return !(*this == other);
    }
    Symbol *getSymbol() {
        if (!this->isSymOp())
            CRASH();
        return static_cast<Symbol*>(OP_SYMBOL(((::operand*)this)));
    }
    bool isSymOp() {
        return IS_SYMOP(((::operand*)this));
    }
    SymLink *getType() {
        return static_cast<SymLink*>(operandType(((::operand*)this)));
    }
    bool isOpGlobal() {
        ::operand *op = (::operand*) this;
        return this && this->type == SYMBOL && this->isGlobal;
    }
    bool isGlobalVolatile() {
        return isvolatile && isOpGlobal();
    }
    bool isInOutRef() {
        // TODO Don't really know what to do about this, not really necessary
        return false;
    }
    int liveTo() {
        return OP_LIVETO((::operand*)this);
    }
    int liveFrom() {
        return OP_LIVEFROM(((::operand*)this));
    }
    Value *getValue() {
        return (Value*) OP_VALUE((::operand*)this);
    }
};

class ICode : public ::iCode {
public:
    static ICode *fromEbBlock(EbBlock **ebbs, int count) {
        return (ICode*) iCodeFromeBBlock((eBBlock**) ebbs, count);
    }
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
    ICode *getPrev() {
        return static_cast<ICode*>(this->prev);
    }
    ICode *getFirst() {
        iCode *ic = (::iCode*)this;
        while (ic->prev) {
            if (SKIP_IC2(ic->prev) || ic->prev->op == IFX)
                break;
            ic = ic->prev;
        }
        return static_cast<ICode*>(ic);
    }
    bool skipNoOp() {
        unsigned int op = this->op;
        return op == GOTO || op == LABEL || op == FUNCTION ||
               op == INLINEASM || op == JUMPTABLE || op == IFX ||
               op == CALL || op == PCALL || op == ARRAYINIT ||
               op == CRITICAL || op == ENDCRITICAL || op == ENDFUNCTION;
    }
    bool isPointerSet() {
        return POINTER_SET(((::iCode*)this));
    }
    bool isUsedInCurrentInstruction(Operand* op);
};

inline Emitter& operator<<(Emitter &e, Symbol *s) {
    e << s->rname;
}


#endif // __cplusplus
#endif // PBLAZE_WRAP_H

