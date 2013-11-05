/*
 * Copyright (C) 2012-2013 Martin Bříza <m@rtinbriza.cz>
 * Copyright (C) 2013 Zbyněk Křivka <krivka@fit.vutbr.cz>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB. If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "common.h"
#include "main.h"
#include "dbuf_string.h"

#include "glue.h"

#define SEND_REG_COUNT 4

void pblaze_init(void) {
    fprintf(stderr, "%s\n", __FUNCTION__);
}

bool pblaze_parseOption(int *pargc, char **argv, int *i) {
    fprintf(stderr, "%s\n", __FUNCTION__);
    return false;
    // currently does nothing as no options are allowed
}

void pblaze_initPaths(void) {
    fprintf(stderr, "%s\n", __FUNCTION__);
}

void pblaze_finaliseOptions(void) {
    fprintf(stderr, "%s\n", __FUNCTION__);
}

void pblaze_setDefaultOptions(void) {
    fprintf(stderr, "%s\n", __FUNCTION__);
}

#define NREGS 16
extern struct dbuf_s *codeOutBuf;
operand *register_vars[NREGS] = { 0 };

void pblaze_assignRegisters(ebbIndex * ebbi)
{
    fprintf(stderr, "%s\n", __FUNCTION__);
	eBBlock **ebbs = ebbi->bbOrder;
	int count = ebbi->count;
	int i;
	for (i = 0; i < count; i++) {
		fprintf(stderr, "Block %d:\n", i);
		iCode *ic = ebbs[i]->sch;
        printiCChain(ic, stderr);
        char linebuf[256]; // FIXME
        size_t c_copied = 0; //FIXME
        size_t i; //FIXME
        while (ic) {
            switch (ic->op) {
                case FUNCTION:
                    fprintf(stderr, "Would generate a function %s\n", OP_SYMBOL(IC_LEFT(ic))->name);
                    sprintf(linebuf, "_%s:", OP_SYMBOL(IC_LEFT(ic))->name);
                    printLine(newLineNode(linebuf), codeOutBuf);
                    break;
                case LABEL:
                    fprintf(stderr, "Label\n");
                    break;
                case CALL:
                    // TODO calling conventions
                    fprintf(stderr, "Call: %s\n", (OP_SYMBOL(IC_LEFT(ic))->rname[0] ? OP_SYMBOL(IC_LEFT(ic))->rname : OP_SYMBOL(IC_LEFT(ic))->name));
                    sprintf(linebuf, "CALL %s", (OP_SYMBOL(IC_LEFT(ic))->rname[0] ? OP_SYMBOL(IC_LEFT(ic))->rname : OP_SYMBOL(IC_LEFT(ic))->name));
                    printLine(newLineNode(linebuf), codeOutBuf);
                    break;
                case GOTO:
                    fprintf(stderr, "Goto\n");
                    break;
                case ENDFUNCTION:
                    fprintf(stderr, "Endfunction\n");
                    // TODO can't tell if this is correct now
                    // TODO returning twice
                    // fallthrough
                case RETURN:
                    // no value returning
                    fprintf(stderr, "Return\n");
                    strcpy(linebuf, "RETURN");
                    printLine(newLineNode(linebuf), codeOutBuf);
                    break;
                case '=':
                    // won't care about initialization - parser handles this
                    // so far only 1B variables available
                    // FIXME very very bad register allocation
                    for (i = 0; i < NREGS; i++) {
                        if (!register_vars[i]) {
                            register_vars[i] = IC_LEFT(ic);
                            sprintf(linebuf, "LOAD s%1x, $%x", i, ulFromVal(OP_VALUE(IC_RIGHT(ic))));
                            printLine(newLineNode(linebuf), codeOutBuf);
                            break;
                        }
                    }
                    fprintf(stderr, "Assignment\n");
                    break;
                default:
                    fprintf(stderr, "%d!\n", ic->op);
                    break;
            }
            ic = ic->next;
        }
	}
}

const char *pblaze_getRegName(const struct reg_info *reg) {
    fprintf(stderr, "%s\n", __FUNCTION__);
    /*
    if (reg)
        return reg->name;
    */
    return "ERROR"; // TODO find out what NULL would do
}

void pblaze_genAssemblerPreamble(FILE *of) {
    fprintf(stderr, "%s\n", __FUNCTION__);
}

void pblaze_genAssemblerEnd(FILE *of) {
    fprintf(stderr, "%s\n", __FUNCTION__);
}

int pblaze_genIVT(struct dbuf_s *oBuf, symbol **intTable, int intCount) {
    fprintf(stderr, "%s\n", __FUNCTION__);
    return 1; // TODO
}

void pblaze_genInitStartup(FILE *of) {
    fprintf(stderr, "%s\n", __FUNCTION__);
}

static int regParmFlg = 0;

void pblaze_reset_regparms(void) {
    fprintf(stderr, "%s\n", __FUNCTION__);
    regParmFlg = 0;
}

int pblaze_reg_parm(struct sym_link *link, bool reentrant) {
    fprintf(stderr, "%s\n", __FUNCTION__);
    if (regParmFlg < SEND_REG_COUNT) {
        int size;
        if ((size = getSize(link)) > (SEND_REG_COUNT - regParmFlg)) {
            /* all remaining go on stack */
            regParmFlg = SEND_REG_COUNT;
            return 0;
        }
        regParmFlg += size;
        return 1;
    }
    
    return 0;
}

bool pblaze_hasExtBitOp(int op, int size) {
    fprintf(stderr, "%s\n", __FUNCTION__);
    return false; // TODO: would be worth finding out what it actually does
}

int pblaze_oclsExpense(struct memmap *oclass) {
    fprintf(stderr, "%s\n", __FUNCTION__);
    return 1; // TODO: would be worth finding out what it actually does
}

int pblaze_assemble() {
    return 0;
}

/* $3 is replaced by assembler.debug_opts resp. port->assembler.plain_opts */
static const char *_asmCmd[] =
{
  "pblazasm", "$l", "$3", "$2", "$1.asm", NULL
};


/*
 * TODO: When finishing, delete all the direct member initializers to make the 
 * PORT_MAGIC work again (for potential porting to newer versions of sdcc)
 */
PORT pblaze_port = {
    .id                          = TARGET_ID_PBLAZE,
    .target                      = "pblaze",
    .target_name                 = "XILINX PicoBlaze",
    .processor                   = NULL,
    .general                     = {
        .do_glue                     = pblaze_do_glue,
        .glue_up_main                = TRUE,
        .supported_models            = MODEL_SMALL,
        .default_model               = MODEL_SMALL,
        .get_model                   = NULL,
    },
    .assembler                   = {
        .cmd                         = NULL,
        .mcmd                        = NULL,
        .debug_opts                  = "-plosgff",
        .plain_opts                  = "-plosgff",
        .externGlobal                = 0,
        .file_ext                    = ".psm",
        .do_assemble                 = pblaze_assemble,
    },
    .linker                      = {
        .cmd                         = NULL,
        .mcmd                        = NULL,
        .do_link                     = pblaze_assemble,
        .rel_ext                     = ".rel",
        .needLinkerScript            = 1,
        .crt                         = NULL,  // new in SDCC 3.2/3.3
        .libs                        = NULL,  // new in SDCC 3.2/3.3
    },
    .peep                        = {
        .default_rules               = NULL,
        .getSize                     = NULL,  // new in SDCC 3.2/3.3
        .getRegsRead                 = NULL,  // new in SDCC 3.2/3.3
        .getRegsWritten              = NULL,  // new in SDCC 3.2/3.3
        .deadMove                    = NULL,  // new in SDCC 3.2/3.3
        .notUsed                     = NULL,  // new in SDCC 3.2/3.3
        .canAssign                   = NULL,  // new in SDCC 3.2/3.3
        .notUsedFrom                 = NULL,  // new in SDCC 3.2/3.3 
    },
    .s                           = {
        .char_size                   = 1,
        .short_size                  = 2,
        .int_size                    = 2,
        .long_size                   = 4,
        .longlong_size               = 4,
        .ptr_size                    = 1,
        .fptr_size                   = 1,
        .gptr_size                   = 1,
        .bit_size                    = 1,
        .float_size                  = 0,
        .max_base_size               = 4,
    },
    .gp_tags                     = {
        .tag_far                     = 0x00,
        .tag_near                    = 0x40,
        .tag_xstack                  = 0x60,
        .tag_code                    = 0x80,
    },
    .mem                         = {
        .xstack_name                 = "XSEG",
        .istack_name                 = "STACK",
        .code_name                   = "CSEG",
        .data_name                   = "DSEG",
        .idata_name                  = "ISEG",
        .pdata_name                  = NULL,
        .xdata_name                  = "XSEG",
        .bit_name                    = "BSEG",
        .reg_name                    = "RSEG",
        .static_name                 = "GSINIT",
        .overlay_name                = "OSEG",
        .post_static_name            = "GSFINAL",
        .home_name                   = "HOME",
        .xidata_name                 = NULL,
        .xinit_name                  = NULL, 
        .const_name                  = "CONST   (CODE)",
        .cabs_name                   = "CABS    (ABS,CODE)",
        .xabs_name                   = "XABS    (ABS,XDATA)",
        .iabs_name                   = "IABS    (ABS,DATA)",
        .initialized_name            = NULL,  // new in SDCC 3.2/3.3 
        .initializer_name            = NULL,  // new in SDCC 3.2/3.3
        .default_local_map           = NULL, 
        .default_globl_map           = NULL,        
        .code_ro                     = 0,
        .maxextalign                 = 1,     // new in SDCC 3.2/3.3
    },
    .extraAreas                  = {
        .genExtraAreaDeclaration     = NULL,
        .genExtraAreaLinkOptions     = NULL,
    },
    .stack                       = {
        .direction                   = -1,
        .bank_overhead               = 1,
        .isr_overhead                = 4,
        .call_overhead               = 1,
        .reent_overhead              = 1,
        .banked_overhead             = 0,
    },
    .support                     = {
        .muldiv                      = 0,
        .shift                       = -1
    },
    .debugger                    = {
        .emitDebuggerSymbol          = NULL,
        .dwarf                       = {
            .regNum                      = NULL,
            .cfiSame                     = NULL,  // new in SDCC 3.2/3.3
            .cfiUndef                    = NULL,  // new in SDCC 3.2/3.3
            .addressSize                 = 1,     // new in SDCC 3.2/3.3
            .regNumRet                   = 4,     // new in SDCC 3.2/3.3
            .regNumSP                    = 1,     // new in SDCC 3.2/3.3
            .regNumBP                    = 1,     // new in SDCC 3.2/3.3
            .offsetSP                    = 0,     // new in SDCC 3.2/3.3
        },
    },
    .jumptableCost               = {
        .maxCount                    = 32,
        .sizeofElement               = 2,
        .sizeofMatchJump             = {
                                        2,
                                        2, 
                                        2,
        },
        .sizeofRangeCompare          = {
                                        0,
                                        0,
                                        0,
        },
        .sizeofSubtract              = 0,
        .sizeofDispatch              = 2,
    },
    .fun_prefix                  = "_",
    .init                        = pblaze_init,
    .parseOption                 = pblaze_parseOption,
    .poptions                    = NULL,
    .initPaths                   = NULL,
    .finaliseOptions             = pblaze_finaliseOptions,
    .setDefaultOptions           = pblaze_setDefaultOptions,
    .assignRegisters             = pblaze_assignRegisters,
    .getRegName                  = pblaze_getRegName,
    .keywords                    = NULL,
    .genAssemblerPreamble        = pblaze_genAssemblerPreamble,
    .genAssemblerEnd             = pblaze_genAssemblerEnd,
    .genIVT                      = pblaze_genIVT,
    .genXINIT                    = NULL,
    .genInitStartup              = pblaze_genInitStartup,
    .reset_regparms              = pblaze_reset_regparms,
    .reg_parm                    = pblaze_reg_parm,
    .process_pragma              = NULL,
    .getMangledFunctionName      = NULL,
    .hasNativeMulFor             = NULL,
    .hasExtBitOp                 = pblaze_hasExtBitOp,
    .oclsExpense                 = pblaze_oclsExpense,
    .use_dw_for_init             = FALSE,
    .little_endian               = TRUE,
    .lt_nge                      = 0,
    .gt_nle                      = 0,
    .le_ngt                      = 1,
    .ge_nlt                      = 1,
    .ne_neq                      = 1,
    .eq_nne                      = 0,
    .arrayInitializerSuppported  = FALSE,
    .cseOk                       = 0,
    .builtintable                = NULL,
    .unqualified_pointer         = GPOINTER,
    .reset_labelKey              = 0,
    .globals_allowed             = 1,
    .num_regs                    = 16,  // new in SDCC 3.2/3.3
    PORT_MAGIC
};
