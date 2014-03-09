/*-------------------------------------------------------------------------
  SDCCgen51.c - source file for code generation for 8051

  Written By -  Sandeep Dutta . sandeep.dutta@usa.net (1998)
         and -  Jean-Louis VERN.jlvern@writeme.com (1999)
  Bug Fixes  -  Wojciech Stryjewski  wstryj1@tiger.lsu.edu (1999 v2.1.9a)

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation; either version 2, or (at your option) any
  later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

  In other words, you are welcome to use, share and improve this program.
  You are forbidden to forbid anyone else to use, share and improve
  what you give them.   Help stamp out software-hoarding!

-------------------------------------------------------------------------*/

//#define D(x)
#define D(x) x

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "SDCCglobl.h"
#include "newalloc.h"

#include "common.h"
#include "SDCCpeeph.h"
#include "ralloc.h"
#include "gen.h"

extern int allocInfo;


extern struct dbuf_s *codeOutBuf;

void bailOut(char *mesg) {
    fprintf(stderr, "%s: bailing out\n", mesg);
    exit(1);
}


/*-----------------------------------------------------------------*/
/* newAsmop - creates a new asmOp                                  */
/*-----------------------------------------------------------------*/
static asmop *newAsmop(short type) {
    asmop *aop;

    aop = Safe_calloc(1, sizeof(asmop));
    aop->type = type;
    return aop;
}

char *aopTypeName(asmop * aop) {
    switch (aop->type) {
    case AOP_LIT:
        return "lit";
    case AOP_REG:
        return "reg";
    case AOP_DIR:
        return "dir";
    case AOP_FAR:
        return "far";
    case AOP_CODE:
        return "code";
    case AOP_GPTR:
        return "gptr";
    case AOP_STK:
        return "stack";
    case AOP_IMMD:
        return "imm";
    case AOP_BIT:
        return "bit";
    }
    return "unknown";
}

/*-----------------------------------------------------------------*/
/* aopForSym - for a true symbol                                   */
/*-----------------------------------------------------------------*/
static asmop *aopForSym(symbol * sym, bool canUsePointer, bool canUseOffset) {
    int size;
    asmop *aop;

    sym->aop = aop = newAsmop(0);
    size = aop->size = getSize(sym->type);

    // if the sym has registers
    if (sym->nRegs && sym->regs[0]) {
        aop->type = AOP_REG;
        sprintf(aop->name[0], "%s", sym->regs[0]->name);
        if (size > 2) {
            sprintf(aop->name[1], "%s", sym->regs[1]->name);
        }
        return aop;
    }

    // if it is on stack
    if (sym->onStack) {
        if (!canUsePointer || !canUseOffset) {
            aop->type = AOP_REG;
            switch (size) {
            }
        }
        aop->type = AOP_STK;
        return aop;
    }

    // if it has a spillLoc
    if (sym->usl.spillLoc) {
        return aopForSym(sym->usl.spillLoc, canUsePointer, canUseOffset);
    }

    // if in bit space
    if (IN_BITSPACE(SPEC_OCLS(sym->etype))) {
        aop->type = AOP_BIT;
        sprintf(aop->name[0], "%s", sym->rname);
        return aop;
    }

    // if in direct space
    if (IN_DIRSPACE(SPEC_OCLS(sym->etype))) {
        aop->type = AOP_DIR;
        sprintf(aop->name[0], "%s", sym->rname);
        if (size > 2) {
            sprintf(aop->name[1], "%s+2", sym->rname);
        }
        return aop;
    }

    // if in code space
    if (IN_CODESPACE(SPEC_OCLS(sym->etype))) {
        if (!canUsePointer) {
            aop->type = AOP_REG;
            switch (size) {
            }

        }
        else {
            aop->type = AOP_CODE;
            emitcode("mov", "r0,#%s ; aopForSym:code", sym->rname);
            sprintf(aop->name[0], "[r0]");
            if (size > 2) {
                sprintf(aop->name[1], "[r0+2]");
            }
        }
        return aop;
    }

    // if in far space
    if (IN_FARSPACE(SPEC_OCLS(sym->etype))) {
        if (!canUsePointer) {
            aop->type = AOP_REG;
            switch (size) {
            }
        }
        else {
            aop->type = AOP_FAR;
            emitcode("mov.w", "r0,#%s ; aopForSym:far", sym->rname);
            sprintf(aop->name[0], "[r0]");
            if (size > 2) {
                sprintf(aop->name[1], "[r0+2]");
            }
            return aop;
        }
    }

    bailOut("aopForSym");
    return NULL;
}

/*-----------------------------------------------------------------*/
/* aopForVal - for a value                                         */
/*-----------------------------------------------------------------*/
static asmop *aopForVal(operand * op) {
    asmop *aop;

    if (IS_OP_LITERAL(op)) {
        op->aop = aop = newAsmop(AOP_LIT);
        switch ((aop->size = getSize(operandType(op)))) {
        }
        return aop;
    }

    // must be immediate
    if (IS_SYMOP(op)) {
        op->aop = aop = newAsmop(AOP_IMMD);
        switch ((aop->size = getSize(OP_SYMBOL(op)->type))) {
        }
    }

    bailOut("aopForVal: unknown type");
    return NULL;
}

static int aopOp(operand * op, bool canUsePointer, bool canUseOffset) {

    if (IS_SYMOP(op)) {
        op->aop = aopForSym(OP_SYMBOL(op), canUsePointer, canUseOffset);
        return AOP_SIZE(op);
    }
    if (IS_VALOP(op)) {
        op->aop = aopForVal(op);
        return AOP_SIZE(op);
    }

    bailOut("aopOp: unexpected operand");
    return 0;
}

bool aopEqual(asmop * aop1, asmop * aop2, int offset) {
    if (strcmp(aop1->name[offset], aop2->name[offset])) {
        return FALSE;
    }
    return TRUE;
}

bool aopIsDir(operand * op) {
    return AOP_TYPE(op) == AOP_DIR;
}

bool aopIsBit(operand * op) {
    return AOP_TYPE(op) == AOP_BIT;
}

bool aopIsPtr(operand * op) {
    if (AOP_TYPE(op) == AOP_STK ||
        AOP_TYPE(op) == AOP_CODE || AOP_TYPE(op) == AOP_FAR) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

char *opRegName(operand * op, int offset, char *opName, bool decorate) {

    if (IS_SYMOP(op)) {
        if (OP_SYMBOL(op)->onStack) {
            return opName;
        }
        if (IS_TRUE_SYMOP(op))
            return OP_SYMBOL(op)->rname;
        else if (OP_SYMBOL(op)->regs[offset])
            return OP_SYMBOL(op)->regs[offset]->name;
        else
            bailOut("opRegName: unknown regs");
    }

    if (IS_VALOP(op)) {
        switch (SPEC_NOUN(OP_VALUE(op)->type)) {
        case V_SBIT:
        case V_BIT:
            if (SPEC_CVAL(OP_VALUE(op)->type).v_int &&
                SPEC_CVAL(OP_VALUE(op)->type).v_int != 1) {
                bailOut("opRegName: invalid bit value");
            }
            // fall through
        case V_CHAR:
            sprintf(opName, "#%s0x%02x", decorate ? "(char)" : "",
                    SPEC_CVAL(OP_VALUE(op)->type).v_int);
            break;
        case V_INT:
            if (SPEC_LONG(OP_VALUE(op)->type)) {
                sprintf(opName, "#%s0x%02x", decorate ? "(long)" : "",
                        SPEC_CVAL(OP_VALUE(op)->type).v_long);
            }
            else {
                sprintf(opName, "#%s0x%02x", decorate ? "(int)" : "",
                        SPEC_CVAL(OP_VALUE(op)->type).v_int);
            }
            break;
        case V_FLOAT:
            sprintf(opName, "#%s%f", decorate ? "(float)" : "",
                    SPEC_CVAL(OP_VALUE(op)->type).v_float);
            break;
        default:
            bailOut("opRegName: unexpected noun");
        }
        return opName;
    }
    bailOut("opRegName: unexpected operand type");
    return NULL;
}

char *printOp(operand * op) {
    static char line[132];
    sym_link *optype = operandType(op);
    bool isPtr = IS_PTR(optype);

    if (IS_SYMOP(op)) {
        symbol *sym = OP_SYMBOL(op);
        if (!sym->regs[0] && SYM_SPIL_LOC(sym)) {
            sym = SYM_SPIL_LOC(sym);
        }
        if (isPtr) {
            sprintf(line, "[");
            if (DCL_TYPE(optype) == FPOINTER)
                strcat(line, "far * ");
            else if (DCL_TYPE(optype) == CPOINTER)
                strcat(line, "code * ");
            else if (DCL_TYPE(optype) == GPOINTER)
                strcat(line, "gen * ");
            else if (DCL_TYPE(optype) == POINTER)
                strcat(line, "near * ");
            else
                strcat(line, "unknown * ");
            strcat(line, "(");
            strcat(line, nounName(sym->etype));
            strcat(line, ")");
            strcat(line, sym->name);
            strcat(line, "]:");
        }
        else {
            sprintf(line, "(%s)%s:", nounName(sym->etype), sym->name);
        }
        if (sym->regs[0]) {
            strcat(line, sym->regs[0]->name);
            if (sym->regs[1]) {
                strcat(line, ",");
                strcat(line, sym->regs[1]->name);
            }
            return line;
        }
        if (sym->onStack) {
            sprintf(line + strlen(line), "stack%+d", sym->stack);
            return line;
        }
        if (IN_CODESPACE(SPEC_OCLS(sym->etype))) {
            strcat(line, "code");
            return line;
        }
        if (IN_FARSPACE(SPEC_OCLS(sym->etype))) {
            strcat(line, "far");
            return line;
        }
        if (IN_BITSPACE(SPEC_OCLS(sym->etype))) {
            strcat(line, "bit");
            return line;
        }
        if (IN_DIRSPACE(SPEC_OCLS(sym->etype))) {
            strcat(line, "dir");
            return line;
        }
        strcat(line, "unknown");
        return line;
    }
    else if (IS_VALOP(op)) {
        opRegName(op, 0, line, 1);
    }
    else if (IS_TYPOP(op)) {
        sprintf(line, "(");
        if (isPtr) {
            if (DCL_TYPE(optype) == FPOINTER)
                strcat(line, "far * ");
            else if (DCL_TYPE(optype) == CPOINTER)
                strcat(line, "code * ");
            else if (DCL_TYPE(optype) == GPOINTER)
                strcat(line, "gen * ");
            else if (DCL_TYPE(optype) == POINTER)
                strcat(line, "near * ");
            else
                strcat(line, "unknown * ");
        }
        // forget about static, volatile, ... for now
        if (SPEC_USIGN(operandType(op)))
            strcat(line, "unsigned ");
        if (SPEC_LONG(operandType(op)))
            strcat(line, "long ");
        strcat(line, nounName(operandType(op)));
        strcat(line, ")");
    }
    else {
        bailOut("printOp: unexpected operand type");
    }
    return line;
}

void
printIc(bool printToStderr,
        char *op, iCode * ic, bool result, bool left, bool right) {
    char line[132];

    sprintf(line, "%s(%d)", op, ic->lineno);
    if (result) {
        strcat(line, " result=");
        strcat(line, printOp(IC_RESULT(ic)));
    }
    if (left) {
        strcat(line, " left=");
        strcat(line, printOp(IC_LEFT(ic)));
    }
    if (right) {
        strcat(line, " right=");
        strcat(line, printOp(IC_RIGHT(ic)));
    }
    emitcode(";", line);
    if (printToStderr) {
        fprintf(stderr, "%s\n", line);
    }
}

/*-----------------------------------------------------------------*/
/* resultRemat - result  is rematerializable                       */
/*-----------------------------------------------------------------*/
static int resultRemat(iCode * ic) {
    if (SKIP_IC(ic) || ic->op == IFX)
        return 0;

    if (IC_RESULT(ic) && IS_ITEMP(IC_RESULT(ic))) {
        symbol *sym = OP_SYMBOL(IC_RESULT(ic));
        if (sym->remat && !POINTER_SET(ic))
            return 1;
    }

    return 0;
}

static char *toBoolean(operand *op) {
    // TODO
    return "";
}

static void genFunction(iCode *ic) {
    symbol *sym = OP_SYMBOL(IC_LEFT(ic));
    emitcode(";", "function %s", sym->name);
    emitcode("", "%s:", sym->rname[0] ? sym->rname : sym->name);
    if (IFFUNC_ISNAKED(sym->type)) {
        emitcode(";", "naked function");
        return;
    }
}

static void genCall(iCode *ic) {
    symbol *sym = OP_SYMBOL(IC_LEFT(ic));
    emitcode("call", "%s", sym->rname[0] ? sym->rname : sym->name);
}

static void genPointerSet(iCode *ic) {

}

static void genAssign(iCode *ic) {
    operand *result = IC_RESULT(ic);
    operand *right = IC_RIGHT(ic);

    aopOp(right, ic, FALSE);
    aopOp(result, ic, TRUE);

    if (AOP_TYPE(result) == AOP_BIT) {
        if (AOP_TYPE (right) == AOP_LIT) {
            if ((int) operandLitValue(right))
                emitcode(";", "TODO setbit");
            else
                emitcode("xor", "%s, %s", AOP_NAME(result)[0], AOP_NAME(result)[0]);
        }
        else if (AOP_TYPE(right) == AOP_BIT) {
            emitcode("load", "%s, %s", AOP_NAME(result), AOP_NAME(right));
        }
        else {
            emitcode("load", "%s, %s", AOP_NAME(result), toBoolean(right));
        }
    }
    else {
        int size = AOP_SIZE(result);
        int offset = 0;
        unsigned long lit = 0;
        while (offset != size) {
            emitcode("load", "%s, %s", AOP_NAME(result), AOP_NAME(right));
            offset++;
        }
    }
}

/*-----------------------------------------------------------------*/
/* gen51Code - generate code for 8051 based controllers            */
/*-----------------------------------------------------------------*/
void genPBlazeCode(iCode * lic) {
    iCode *ic;
    int cln = 0;

    /* if debug information required */
    if (options.debug && currFunc) {
        debugFile->writeFunction(currFunc, lic);
    }

    for (ic = lic; ic; ic = ic->next) {
        if (ic->lineno && cln != ic->lineno) {
            if (options.debug) {
                debugFile->writeCLine(ic);
            }
            if (!options.noCcodeInAsm) {
                emitcode("", ";\t%s:%d: %s", ic->filename, ic->lineno,
                         printCLine(ic->filename, ic->lineno));
            }
            cln = ic->lineno;
        }
        if (options.iCodeInAsm) {
            const char *iLine = printILine(ic);
            emitcode("", ";ic:%d: %s", ic->key, iLine);
            dbuf_free(iLine);
        }

        /* if the result is marked as
           spilt and rematerializable or code for
           this has already been generated then
           do nothing */
        if (resultRemat(ic) || ic->generated)
            continue;

        /* depending on the operation */
        switch (ic->op) {
        case INLINEASM:
            genInline(ic);
            break;

        case FUNCTION:
            genFunction(ic);
            break;

        case CALL:
            genCall(ic);
            break;

        case '=':
            if (POINTER_SET(ic))
                genPointerSet(ic);
            else
                genAssign(ic);
            break;

        default:
            ic = ic;
        }
    }


    /* now we are ready to call the
       peep hole optimizer */
    if (!options.nopeep && genLine.lineHead)
        peepHole(&genLine.lineHead);

    /* now do the actual printing */
    if (genLine.lineHead)
        printLine(genLine.lineHead, codeOutBuf);
    return;
}
