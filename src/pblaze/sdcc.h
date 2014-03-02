#ifndef _SDCC_H
#define _SDCC_H

#include "common.h"
#include "SDCCicode.h"
#include "../SDCCicode.h"
#include <SDCCsymt.h>

namespace SDCC {

    class SymLink : public ::sym_link {
    public:
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