/*
 *  Copyright © 2003..2013 : Henk van Kampen <henk@mediatronix.com>
 *
 *  This file is part of pBlazASM.
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

//!
// List of directive recognized by pBlazASM.
// For compatability, pBlazIDE and KCPSM3 directives
// are recognized and processed
//
const symbol_t directives[] = {
	// pBlazASM
	{ tDIRECTIVE, stORG,        ".ORG",    { 0 }, -1 },
	{ tDIRECTIVE, stALIGN,      ".ALN",    { 0 }, -1 },
	{ tDIRECTIVE, stEND,        ".END",    { 0 }, -1 },
	{ tDIRECTIVE, stPAGE,       ".PAG",    { 0 }, -1 },

	{ tDIRECTIVE, stSCRATCHPAD, ".SCR",    { 0 }, -1 },
	{ tDIRECTIVE, stORG,        ".ORG",    { 0 }, -1 },
	{ tDIRECTIVE, stDSG,        ".DSG",    { 0 }, -1 },
	{ tDIRECTIVE, stESG,        ".ESG",    { 0 }, -1 },

	{ tDIRECTIVE, stDEF,        ".DEF",    { 0 }, -1 },
	{ tDIRECTIVE, stEQU,        ".EQU",    { 0 }, -1 },
	{ tDIRECTIVE, stSET,        ".SET",    { 0 }, -1 },
	{ tDIRECTIVE, stBYTE,       ".BYT",    { 0 }, -1 },
	{ tDIRECTIVE, stWORD_BE,    ".WBE",    { 0 }, -1 },
	{ tDIRECTIVE, stWORD_LE,    ".WLE",    { 0 }, -1 },
	{ tDIRECTIVE, stLONG_BE,    ".LBE",    { 0 }, -1 },
	{ tDIRECTIVE, stLONG_LE,    ".LLE",    { 0 }, -1 },
	{ tDIRECTIVE, stBUFFER,     ".BUF",    { 0 }, -1 },
	{ tDIRECTIVE, stTEXT,       ".TXT",    { 0 }, -1 },

	{ tDIRECTIVE, stIF,         ".IF",     { 0 }, -1 },
	{ tDIRECTIVE, stFI,         ".FI",     { 0 }, -1 },

	// pBlazIDE
	{ tDIRECTIVE, stORG,        "ORG",     { 0 }, -1 },
	{ tDIRECTIVE, stEQU,        "EQU",     { 0 }, -1 },
	{ tDIRECTIVE, stDS,         "DS",      { 0 }, -1 },
	{ tDIRECTIVE, stDSIN,       "DSIN",    { 0 }, -1 },
	{ tDIRECTIVE, stDSOUT,      "DSOUT",   { 0 }, -1 },
	{ tDIRECTIVE, stDSIO,       "DSIO",    { 0 }, -1 },
	{ tDIRECTIVE, stDSROM,      "DSROM",   { 0 }, -1 },
	{ tDIRECTIVE, stDSRAM,      "DSRAM",   { 0 }, -1 },

	{ tDIRECTIVE, stEXEC,       "EXEC",    { 0 }, -1 },
	{ tDIRECTIVE, stVHDL,       "VHDL",    { 0 }, -1 },
	{ tDIRECTIVE, stMEM,        "MEM",     { 0 }, -1 },
	{ tDIRECTIVE, stCOE,        "COE",     { 0 }, -1 },
	{ tDIRECTIVE, stHEX,        "HEX",     { 0 }, -1 },

	// KCPSM
	{ tDIRECTIVE, stADDRESS,    "ADDRESS", { 0 }, -1 },
	{ tDIRECTIVE, stCONSTANT,   "CONSTANT",{ 0 }, -1 },
	{ tDIRECTIVE, stNAMEREG,    "NAMEREG", { 0 }, -1 }
} ;

