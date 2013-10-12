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

#define MEMSIZE 256

#include "common.h"
#include "dbuf_string.h"

symbol *interrupts[INTNO_MAX + 1];

void printIval(symbol *, sym_link *, initList *, struct dbuf_s *, bool check);
set *pblaze_publics = NULL; /* public variables */
set *pblaze_externs = NULL; /* Variables that are declared as extern */

//unsigned maxInterrupts = 0;
int pblaze_allocInfo = 1;
symbol *pblaze_mainf;
int pblaze_noInit = 0;      /* no initialization */


/*-----------------------------------------------------------------*/
/* pblaze_emitDebugSym - emit label for debug symbol                      */
/*-----------------------------------------------------------------*/
static void pblaze_emitDebugSym(struct dbuf_s *oBuf, symbol * sym)
{
    if (!sym->level) {      /* global */
        if (IS_STATIC(sym->etype))
            dbuf_printf(oBuf, "F%s$", moduleName);  /* scope is file */
        else
            dbuf_printf(oBuf, "G$");    /* scope is global */
    } else {
        /* symbol is local */
        dbuf_printf(oBuf, "L%s$", (sym->localof ? sym->localof->name : "-null-"));
    }
    dbuf_printf(oBuf, "%s$%d$%d", sym->name, sym->level, sym->block);
}


/*-----------------------------------------------------------------*/
/* pblaze_printChar - formats and prints a characater string with DB      */
/*-----------------------------------------------------------------*/
void pblaze_printChar(struct dbuf_s *oBuf, char *s, int plen)
{
    int i;
    int len = plen;
    int pplen = 0;
    char buf[100];
    char *p = buf;

    while (len && pplen < plen) {
    i = 60;
    while (i && pplen < plen) {
        if (*s < ' ' || *s == '\"' || *s == '\\') {
            *p = '\0';
            if (p != buf)
                dbuf_tprintf(oBuf, "\t!ascii\n", buf);
            dbuf_tprintf(oBuf, "\t!db !constbyte\n", (unsigned char) *s);
            p = buf;
        } else {
            *p = *s;
            p++;
        }
        s++;
        pplen++;
        i--;
    }
    if (p != buf) {
        *p = '\0';
        dbuf_tprintf(oBuf, "\t!ascii\n", buf);
        p = buf;
    }

    if (len > 60)
        len -= 60;
    else
        len = 0;
    }
    while (pplen < plen) {
        dbuf_tprintf(oBuf, "\t!db !constbyte\n", 0);
        pplen++;
    }
}

/*-----------------------------------------------------------------*/
/* pblaze_emitRegularMap - emit code for maps with no special cases       */
/*-----------------------------------------------------------------*/
static void pblaze_emitRegularMap(memmap * map, bool addPublics, bool arFlag)
{
    symbol *sym;
    ast *ival = NULL;

    if (!map)
        return;

    if (addPublics) {

        if (map->regsp)
            dbuf_tprintf(&map->oBuf, "\t!org\n", 0);
    }

    for (sym = setFirstItem(map->syms); sym; sym = setNextItem(map->syms)) {
        symbol *newSym = NULL;

        /* if extern then add it into the extern list */
        if (IS_EXTERN(sym->etype)) {
            addSetHead(&pblaze_externs, sym);
            continue;
        }

        /* if allocation required check is needed
        then check if the symbol really requires
        allocation only for local variables */

        if (arFlag && !IS_AGGREGATE(sym->type) && !(sym->_isparm && !IS_REGPARM(sym->etype)) && !sym->allocreq && sym->level)
            continue;

        /* for bitvar locals and parameters */
        if (!arFlag && !sym->allocreq && sym->level && !SPEC_ABSA(sym->etype)) {
            continue;
        }

        /* if global variable & not static or extern
        and addPublics allowed then add it to the public set */
        if ((sym->level == 0 ||
            (sym->_isparm && !IS_REGPARM(sym->etype))) &&
            addPublics && !IS_STATIC(sym->etype) && (IS_FUNC(sym->type) ? (sym->used || IFFUNC_HASBODY(sym->type)) : 1)) {
            addSetHead(&pblaze_publics, sym);
        }

        /* if extern then do nothing or is a function
        then do nothing */
        if (IS_FUNC(sym->type) && !(sym->isitmp))
            continue;

        /* if it has an initial value then do it only if
        it is a global variable */
        if (sym->ival && sym->level == 0) {
            if ((SPEC_OCLS(sym->etype) == xidata) && !SPEC_ABSA(sym->etype)) {
                sym_link *t;
                /* create a new "XINIT (CODE)" symbol, that will be emited later
                in the static seg */
                newSym = copySymbol(sym);
                SPEC_OCLS(newSym->etype) = xinit;
                SNPRINTF(newSym->name, sizeof(newSym->name), "__xinit_%s", sym->name);
                SNPRINTF(newSym->rname, sizeof(newSym->rname), "__xinit_%s", sym->rname);
                /* find the first non-array link */
                t = newSym->type;
                while (IS_ARRAY(t))
                    t = t->next;
                if (IS_SPEC(t))
                    SPEC_CONST(t) = 1;
                else
                    DCL_PTR_CONST(t) = 1;
                SPEC_STAT(newSym->etype) = 1;
                resolveIvalSym(newSym->ival, newSym->type);

                // add it to the "XINIT (CODE)" segment
                addSet(&xinit->syms, newSym);

                if (!SPEC_ABSA(sym->etype)) {
                    struct dbuf_s tmpBuf;

                    dbuf_init(&tmpBuf, 4096);
                    // before allocation we must parse the sym->ival tree
                    // but without actually generating initialization code
                    ++noAlloc;
                    resolveIvalSym(sym->ival, sym->type);
                    ++pblaze_noInit;
                    printIval(sym, sym->type, sym->ival, &tmpBuf, TRUE);
                    --pblaze_noInit;
                    --noAlloc;
                    dbuf_destroy(&tmpBuf);
                }
            } else {
                if (IS_AGGREGATE(sym->type)) {
                    ival = initAggregates(sym, sym->ival, NULL);
                } else {
                    if (getNelements(sym->type, sym->ival) > 1) {
                    werrorfl(sym->fileDef, sym->lineDef, W_EXCESS_INITIALIZERS, "scalar", sym->name);
                    }
                    ival = newNode('=', newAst_VALUE(symbolVal(sym)), decorateType(resolveSymbols(list2expr(sym->ival)), RESULT_TYPE_NONE));
                }
                codeOutBuf = &statsg->oBuf;

                if (ival) {
                    // set ival's lineno to where the symbol was defined
                    setAstFileLine(ival, filename = sym->fileDef, lineno = sym->lineDef);
                    // check if this is not a constant expression
                    if (!constExprTree(ival)) {
                    werror(E_CONST_EXPECTED, "found expression");
                    // but try to do it anyway
                    }
                    pblaze_allocInfo = 0;
                    if (!astErrors(ival))
                    eBBlockFromiCode(iCodeFromAst(ival));
                    pblaze_allocInfo = 1;
                }
            }
        }

        /* if it has an absolute address then generate
        an equate for this no need to allocate space */
        if (SPEC_ABSA(sym->etype) && !sym->ival) {
            char *equ = "=";

            /* print extra debug info if required */
            if (options.debug) {
                pblaze_emitDebugSym(&map->oBuf, sym);
                dbuf_printf(&map->oBuf, " == 0x%04x\n", SPEC_ADDR(sym->etype));
            }
            if (TARGET_IS_XA51) {
                if (map == sfr) {
                    equ = "sfr";
                } else if (map == bit || map == sfrbit) {
                    equ = "bit";
                }
            }
            dbuf_printf(&map->oBuf, "%s\t%s\t0x%04x\n", sym->rname, equ, SPEC_ADDR(sym->etype));
        } else {
            int size = getSize(sym->type) + sym->flexArrayLength;
            if (size == 0) {
                werrorfl(sym->fileDef, sym->lineDef, E_UNKNOWN_SIZE, sym->name);
            }
            /* allocate space */
            if (SPEC_ABSA(sym->etype)) {
                dbuf_tprintf(&map->oBuf, "\t!org\n", SPEC_ADDR(sym->etype));
            }
            /* print extra debug info if required */
            if (options.debug) {
                pblaze_emitDebugSym(&map->oBuf, sym);
                dbuf_printf(&map->oBuf, "==.\n");
            }
            if (IS_STATIC(sym->etype) || sym->level)
                dbuf_tprintf(&map->oBuf, "!slabeldef\n", sym->rname);
            else
                dbuf_tprintf(&map->oBuf, "!labeldef\n", sym->rname);
            dbuf_tprintf(&map->oBuf, "\t!ds\n", (unsigned int) size & 0xffff);
        }
        sym->ival = NULL;
    }
}


/*-----------------------------------------------------------------*/
/* pblaze_emitStaticSeg - emitcode for the static segment                 */
/*-----------------------------------------------------------------*/
void pblaze_emitStaticSeg(memmap * map, struct dbuf_s *oBuf)
{
    symbol *sym;

    /* fprintf(out, "\t.area\t%s\n", map->sname); */

    /* for all variables in this segment do */
    for (sym = setFirstItem(map->syms); sym; sym = setNextItem(map->syms)) {
        /* if it is "extern" then do nothing */
        if (IS_EXTERN(sym->etype))
            continue;

        /* if it is not static add it to the public table */
        if (!IS_STATIC(sym->etype)) {
            addSetHead(&pblaze_publics, sym);
        }

        /* if it has an absolute address and no initializer */
        if (SPEC_ABSA(sym->etype) && !sym->ival) {
            if (options.debug) {
                pblaze_emitDebugSym(oBuf, sym);
                dbuf_printf(oBuf, " == 0x%04x\n", SPEC_ADDR(sym->etype));
            }
            dbuf_printf(oBuf, "%s\t=\t0x%04x\n", sym->rname, SPEC_ADDR(sym->etype));
        } else {
            int size = getSize(sym->type);

            if (size == 0) {
                werrorfl(sym->fileDef, sym->lineDef, E_UNKNOWN_SIZE, sym->name);
            }
            /* if it has an initial value */
            if (sym->ival) {
                if (SPEC_ABSA(sym->etype)) {
                    dbuf_tprintf(oBuf, "\t!org\n", SPEC_ADDR(sym->etype));
                }
                if (options.debug) {
                    pblaze_emitDebugSym(oBuf, sym);
                    dbuf_printf(oBuf, " == .\n");
                }
                dbuf_printf(oBuf, "%s:\n", sym->rname);
                ++noAlloc;
                resolveIvalSym(sym->ival, sym->type);
                printIval(sym, sym->type, sym->ival, oBuf, map != xinit);
                --noAlloc;
                /* if sym is a simple string and sym->ival is a string,
                WE don't need it anymore */
                if (IS_ARRAY(sym->type) && IS_CHAR(sym->type->next) &&
                    IS_AST_SYM_VALUE(list2expr(sym->ival)) && list2val(sym->ival)->sym->isstrlit) {
                    freeStringSymbol(list2val(sym->ival)->sym);
                }
            } else {
                /* allocate space */
                if (options.debug) {
                    pblaze_emitDebugSym(oBuf, sym);
                    dbuf_printf(oBuf, " == .\n");
                }
                dbuf_printf(oBuf, "%s:\n", sym->rname);
                /* special case for character strings */
                if (IS_ARRAY(sym->type) && IS_CHAR(sym->type->next) && SPEC_CVAL(sym->etype).v_char) {
                    pblaze_printChar(oBuf, SPEC_CVAL(sym->etype).v_char, size);
                } else {
                    dbuf_tprintf(oBuf, "\t!ds\n", (unsigned int) size & 0xffff);
                }
            }
        }
    }
}

/*-----------------------------------------------------------------*/
/* pblaze_emitMaps - emits the code for the data portion the code         */
/*-----------------------------------------------------------------*/
void pblaze_emitMaps(void)
{
    int publicsfr = TARGET_IS_MCS51;    /* Ideally, this should be true for all  */
    /* ports but let's be conservative - EEP */

    inInitMode++;
    /* no special considerations for the following
       data, idata & bit & xdata */
    pblaze_emitRegularMap(data, TRUE, TRUE);
    pblaze_emitRegularMap(idata, TRUE, TRUE);
    pblaze_emitRegularMap(d_abs, TRUE, TRUE);
    pblaze_emitRegularMap(i_abs, TRUE, TRUE);
    pblaze_emitRegularMap(bit, TRUE, TRUE);
    pblaze_emitRegularMap(pdata, TRUE, TRUE);
    pblaze_emitRegularMap(xdata, TRUE, TRUE);
    pblaze_emitRegularMap(x_abs, TRUE, TRUE);
    if (port->genXINIT) {
        pblaze_emitRegularMap(xidata, TRUE, TRUE);
    }
    pblaze_emitRegularMap(sfr, publicsfr, FALSE);
    pblaze_emitRegularMap(sfrbit, publicsfr, FALSE);
    pblaze_emitRegularMap(home, TRUE, FALSE);
    pblaze_emitRegularMap(code, TRUE, FALSE);


    pblaze_emitStaticSeg(c_abs, &code->oBuf);
    inInitMode--;
}

/*-----------------------------------------------------------------*/
/* pblaze_createInterruptVect - creates the interrupt vector              */
/*-----------------------------------------------------------------*/
void pblaze_createInterruptVect(struct dbuf_s *vBuf)
{
    pblaze_mainf = newSymbol("main", 0);
    pblaze_mainf->block = 0;

    /* only if the main function exists */
    if (!(pblaze_mainf = findSymWithLevel(SymbolTab, pblaze_mainf))) {
        if (!options.cc_only && !noAssemble && !options.c1mode)
            werror(E_NO_MAIN);
        return;
    }

    /* if the main is only a prototype ie. no body then do nothing */
    if (!IFFUNC_HASBODY(pblaze_mainf->type)) {
        /* if ! compile only then main function should be present */
        if (!options.cc_only && !noAssemble)
            werror(E_NO_MAIN);
        return;
    }
    //dbuf_printf (vBuf, "__interrupt_vect:\n");

    if (!port->genIVT || !(port->genIVT(vBuf, interrupts, maxInterrupts))) {
        /* There's no such thing as a "generic" interrupt table header. */
        wassert(0);
    }
}

char *pblaze_iComments1 = {
    ";--------------------------------------------------------\n" "; File Created by SDCC : free open source ANSI-C Compiler\n"
};

char *pblaze_iComments2 = {
    ";--------------------------------------------------------\n"
};


/*-----------------------------------------------------------------*/
/* pbInitialComments - puts in some initial comments                 */
/*-----------------------------------------------------------------*/
void pbInitialComments(FILE * afile)
{
    time_t t;
    time(&t);
    fprintf(afile, "%s", pblaze_iComments1);
    fprintf(afile, "; Version " SDCC_VERSION_STR " #%s (%s) (%s)\n", getBuildNumber(), getBuildDate(), getBuildEnvironment());
    fprintf(afile, "; This file was generated %s", asctime(localtime(&t)));
    fprintf(afile, "%s", pblaze_iComments2);
}


/*-----------------------------------------------------------------*/
/* pblaze_printPublics - generates .global for publics                    */
/*-----------------------------------------------------------------*/
void pblaze_printPublics(FILE * afile)
{
    symbol *sym;

    fprintf(afile, "%s", pblaze_iComments2);
    fprintf(afile, "; Public variables in this module\n");
    fprintf(afile, "%s", pblaze_iComments2);

    for (sym = setFirstItem(pblaze_publics); sym; sym = setNextItem(pblaze_publics))
    tfprintf(afile, "\t!global\n", sym->rname);
}


/*-----------------------------------------------------------------*/
/* pblaze_printExterns - generates .global for Externs                    */
/*-----------------------------------------------------------------*/
void pblaze_printExterns(FILE * afile)
{
    fprintf(afile, "%s", pblaze_iComments2);
    fprintf(afile, "; Externals used\n");
    fprintf(afile, "%s", pblaze_iComments2);

}

/*-----------------------------------------------------------------*/
/* pblaze_emitOverlay - will emit code for the overlay stuff              */
/*-----------------------------------------------------------------*/
static void pblaze_emitOverlay(struct dbuf_s *aBuf)
{
    set *ovrset;


    /* for each of the sets in the overlay segment do */
    for (ovrset = setFirstItem(ovrSetSets); ovrset; ovrset = setNextItem(ovrSetSets)) {
        symbol *sym;

        for (sym = setFirstItem(ovrset); sym; sym = setNextItem(ovrset)) {
            /* if extern then it is in the pblaze_publics table: do nothing */
            if (IS_EXTERN(sym->etype))
            continue;

            /* if allocation required check is needed
            then check if the symbol really requires
            allocation only for local variables */
            if (!IS_AGGREGATE(sym->type) && !(sym->_isparm && !IS_REGPARM(sym->etype))
            && !sym->allocreq && sym->level)
            continue;

            /* if global variable & not static or extern
            and addPublics allowed then add it to the public set */
            if ((sym->_isparm && !IS_REGPARM(sym->etype))
            && !IS_STATIC(sym->etype)) {
            addSetHead(&pblaze_publics, sym);
            }

            /* if extern then do nothing or is a function
            then do nothing */
            if (IS_FUNC(sym->type))
            continue;

            /* print extra debug info if required */
            if (options.debug) {
            if (!sym->level) {  /* global */
                if (IS_STATIC(sym->etype))
                dbuf_printf(aBuf, "F%s$", moduleName);  /* scope is file */
                else
                dbuf_printf(aBuf, "G$");    /* scope is global */
            } else
                /* symbol is local */
                dbuf_printf(aBuf, "L%s$", (sym->localof ? sym->localof->name : "-null-"));
            dbuf_printf(aBuf, "%s$%d$%d", sym->name, sym->level, sym->block);
            }

            /* if is has an absolute address then generate
            an equate for this no need to allocate space */
            if (SPEC_ABSA(sym->etype)) {
            if (options.debug)
                dbuf_printf(aBuf, " == 0x%04x\n", SPEC_ADDR(sym->etype));

            dbuf_printf(aBuf, "%s\t=\t0x%04x\n", sym->rname, SPEC_ADDR(sym->etype));
            } else {
            int size = getSize(sym->type);

            if (size == 0) {
                werrorfl(sym->fileDef, sym->lineDef, E_UNKNOWN_SIZE);
            }
            if (options.debug)
                dbuf_printf(aBuf, "==.\n");

            /* allocate space */
            dbuf_tprintf(aBuf, "!labeldef\n", sym->rname);
            dbuf_tprintf(aBuf, "\t!ds\n", (unsigned int) getSize(sym->type) & 0xffff);
            }
        }
    }
}

/*-----------------------------------------------------------------*/
/* glue - the final glue that hold the whole thing together        */
/*-----------------------------------------------------------------*/
void pblaze_do_glue(void)
{
    fprintf(stderr, "%s\n", __FUNCTION__);
    struct dbuf_s vBuf;
    struct dbuf_s ovrBuf;
    struct dbuf_s asmFileName;
    FILE *asmFile;


    //pblaze_genCodeLoop();

    dbuf_init(&vBuf, 4096);
    dbuf_init(&ovrBuf, 4096);

    /* print the global struct definitions */
    if (options.debug)
    cdbStructBlock(0);

    /* PENDING: this isn't the best place but it will do */
    if (port->general.glue_up_main) {
    /* create the interrupt vector table */
    pblaze_createInterruptVect(&vBuf);
    }

    /* emit code for the all the variables declared */
    pblaze_emitMaps();
    /* do the overlay segments */
    pblaze_emitOverlay(&ovrBuf);

    outputDebugSymbols();

    /* now put it all together into the assembler file */
    /* create the assembler file name */

    /* -o option overrides default name? */
    dbuf_init(&asmFileName, PATH_MAX);
    if ((noAssemble || options.c1mode) && fullDstFileName) {
    dbuf_append_str(&asmFileName, fullDstFileName);
    } else {
    dbuf_append_str(&asmFileName, dstFileName);
    dbuf_append_str(&asmFileName, port->assembler.file_ext);
    }

    if (!(asmFile = fopen(dbuf_c_str(&asmFileName), "w"))) {
    werror(E_FILE_OPEN_ERR, dbuf_c_str(&asmFileName));
    dbuf_destroy(&asmFileName);
    exit(EXIT_FAILURE);
    }
    dbuf_destroy(&asmFileName);

    /* initial comments */
    //pbInitialComments(asmFile);


    /* Let the port generate any global directives, etc. */
    if (port->genAssemblerPreamble) {
    port->genAssemblerPreamble(asmFile);
    }

    /* print the global variables in this module */
    //pblaze_printPublics (asmFile);
    if (port->assembler.externGlobal)
    pblaze_printExterns(asmFile);


    /* If the port wants to generate any extra areas, let it do so. */
    if (port->extraAreas.genExtraAreaDeclaration) {
    port->extraAreas.genExtraAreaDeclaration(asmFile, pblaze_mainf && IFFUNC_HASBODY(pblaze_mainf->type));
    }

    /* copy global & static initialisations */
    fprintf(asmFile, "%s", pblaze_iComments2);
    fprintf(asmFile, "; global & static initialisations\n");
    fprintf(asmFile, "%s", pblaze_iComments2);

    if (pblaze_mainf && IFFUNC_HASBODY(pblaze_mainf->type)) {
    if (port->genInitStartup) {
        port->genInitStartup(asmFile);
    } else {

        // if the port can copy the XINIT segment to XISEG
        if (port->genXINIT) {
        port->genXINIT(asmFile);
        }
    }
    }
    dbuf_write_and_destroy(&statsg->oBuf, asmFile);

    if (port->general.glue_up_main && pblaze_mainf && IFFUNC_HASBODY(pblaze_mainf->type)) {
    /* This code is generated in the post-static area.
     * This area is guaranteed to follow the static area
     * by the ugly shucking and jiving about 20 lines ago.
     */

    //fprintf(asmFile, "\tLOAD\tsF, $%02x\n", MEMSIZE - 1);
    fprintf(asmFile, "\tJUMP\t__sdcc_program_startup\n");
    }

    fprintf(asmFile, "%s" "; Home\n" "%s", pblaze_iComments2, pblaze_iComments2);
    dbuf_write_and_destroy(&home->oBuf, asmFile);

    if (pblaze_mainf && IFFUNC_HASBODY(pblaze_mainf->type)) {
    /* entry point @ start of HOME */
    fprintf(asmFile, "__sdcc_program_startup:\n");

    /* put in jump or call to main */
    if (TRUE) { // N/A: TODO: options.mainreturn
        fprintf(asmFile, "\tJUMP\t_main\n");    /* needed? */
        if (!options.noCcodeInAsm)
        fprintf(asmFile, ";\treturn from main will return to caller\n");
    } else {
        fprintf(asmFile, "\tCALL\t_main\n");
        if (!options.noCcodeInAsm)
        fprintf(asmFile, ";\treturn from main will lock up\n");
        fprintf(asmFile, "__sdcc_loop:\n");
        fprintf(asmFile, "\tJUMP\t__sdcc_loop\n");
    }
    }
    /* copy over code */
    fprintf(asmFile, "%s", pblaze_iComments2);
    fprintf(asmFile, "; code\n");
    fprintf(asmFile, "%s", pblaze_iComments2);
    dbuf_write_and_destroy(&code->oBuf, asmFile);

    if (port->genAssemblerEnd) {
    port->genAssemblerEnd(asmFile);
    }

    /* copy the interrupt vector table */
    if (pblaze_mainf && IFFUNC_HASBODY(pblaze_mainf->type)) {
        fprintf(asmFile, "%s", pblaze_iComments2);
        fprintf(asmFile, "; interrupt vector \n");
        fprintf(asmFile, "%s", pblaze_iComments2);
        dbuf_write_and_destroy(&vBuf, asmFile);
    }

    fclose(asmFile);
}
