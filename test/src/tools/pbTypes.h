
/*
 *  Copyright © 2003..2013 : Henk van Kampen <henk@mediatronix.com>
 *
 *	This file is part of pBlazASM.
 *
 *  pBlazASM is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  pBlazASM is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with pBlazASM.  If not, see <http://www.gnu.org/licenses/>.
 */

typedef unsigned char bool ;
#define true (1)
#define false (0)

#ifdef _MSC_VER
#define inline
#endif

// token types
typedef enum _TYPE {
	tNONE = 0,
	tERROR,
	tAT,
	tLPAREN,
	tRPAREN,
	tCOMMA,
	tCOLON,
	tDCOLON,
	tOPERATOR,
	tREGISTER,
	tOPERAND,
	tOPCODE,
	tCONDITION,
	tIDENT,
	tLABEL,
	tGLABEL,
	tDIRECTIVE,
	tINDEX,
	tVALUE,
	tCHAR,
	tSTRING,
	tHEX,
	tBIN,
	tDEC,
	tOCT,
	tSTAMP,
	tPC,
	tSET
} type_e ;

// token subtypes
typedef enum _SUBTYPE {
	stNONE = 0,
	stCOMMENT,

	// instruction types
	stMOVE3,
	stMOVE6,
	stINT,
	stINTI,
	stINTE,
	stCJMP3,
	stCJMP6,
	stCSKP,
    stBREAK,
	stCRET3,
	stCRET6,
	stIO3,
	stIO6,
	stSHIFT,
 	stINST,

	stSTAR,
	stOUTK,
	stBANK,
	stCORE,

	// operators
	stADD,
	stSUB,
	stAND,
	stIOR,
	stXOR,
	stSHL,
	stSHR,
	stMUL,
	stDIV,
	stMOD,
	stTILDA,

	// equate types
	stVAL,
	stREG,
	stCLONE,

	// timestamp types
	stHOURS,
	stMINUTES,
	stSECONDS,
	stYEAR,
	stMONTH,
	stDAY,

	stSTRING,
	stTOKENS,

	// directives
	stORG,
	stPAGE,

	stALIGN,
	stSCRATCHPAD,
	stEND,

	stDEF,
	stEQU,
	stSET,
	stBYTE,
	stWORD_BE,
	stWORD_LE,
	stLONG_BE,
	stLONG_LE,
	stTEXT,
	stBUFFER,
	stDSG,
	stESG,

	stIF,
	stFI,

	stDS,
	stDSIN,
	stDSOUT,
	stDSIO,
	stDSROM,
	stDSRAM,

	// KCPSM3
	stADDRESS,
	stCONSTANT,
	stNAMEREG,

	// pBlazIDE (unsupported)
	stEXEC,
	stVHDL,
	stXDL,
	stMEM,
	stCOE,
	stHEX,

	stDOT
} subtype_e ;

struct _SYMBOL ;

// token and symbol type
typedef	union _VALUE {
	int integer ;
	char * string ;
	struct _SYMBOL * tokens ;
} value_t ;

typedef struct _SYMBOL {
	type_e type ;
	subtype_e subtype ;
	char * text ;
	union _VALUE value ;
    int filenbr ;
} symbol_t ;
