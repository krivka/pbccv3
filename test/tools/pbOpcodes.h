
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

const symbol_t opcodes3[] = {
    // preferred mnemonics
    { tOPCODE, stMOVE3,   "ADD",      {0x18000}, -1 },
    { tOPCODE, stMOVE3,   "ADDC",     {0x1A000}, -1 },
    { tOPCODE, stMOVE3,   "AND",      {0x0A000}, -1 },
    { tOPCODE, stCJMP3,   "CALL",     {0x30000}, -1 },
    { tOPCODE, stMOVE3,   "COMP",     {0x14000}, -1 },
    { tOPCODE, stINT,     "DINT",     {0x3C000}, -1 },
    { tOPCODE, stINT,     "EINT",     {0x3C001}, -1 },
    { tOPCODE, stIO3,     "IN",       {0x04000}, -1 },
    { tOPCODE, stCJMP3,   "JUMP",     {0x34000}, -1 },
    { tOPCODE, stIO3,     "LD",       {0x06000}, -1 },
    { tOPCODE, stMOVE3,   "MOVE",     {0x00000}, -1 },
    { tOPCODE, stINT,     "NOP",      {0x00000}, -1 },
    { tOPCODE, stMOVE3,   "OR",       {0x0C000}, -1 },
    { tOPCODE, stIO3,     "OUT",      {0x2C000}, -1 },
    { tOPCODE, stCRET3,   "RET",      {0x2A000}, -1 },
    { tOPCODE, stINTE,    "RETI",     {0x38001}, -1 },
    { tOPCODE, stSHIFT,   "RL",       {0x20002}, -1 },
    { tOPCODE, stSHIFT,   "RR",       {0x2000C}, -1 },
    { tOPCODE, stCSKP,    "SKIP",     {0x34000}, -1 },
    { tOPCODE, stSHIFT,   "SL0",      {0x20006}, -1 },
    { tOPCODE, stSHIFT,   "SL1",      {0x20007}, -1 },
    { tOPCODE, stSHIFT,   "SLA",      {0x20000}, -1 },
    { tOPCODE, stSHIFT,   "SLX",      {0x20004}, -1 },
    { tOPCODE, stSHIFT,   "SR0",      {0x2000E}, -1 },
    { tOPCODE, stSHIFT,   "SR1",      {0x2000F}, -1 },
    { tOPCODE, stSHIFT,   "SRA",      {0x20008}, -1 },
    { tOPCODE, stSHIFT,   "SRX",      {0x2000A}, -1 },
    { tOPCODE, stIO3,     "ST",       {0x2E000}, -1 },
    { tOPCODE, stMOVE3,   "SUB",      {0x1C000}, -1 },
    { tOPCODE, stMOVE3,   "SUBC",     {0x1E000}, -1 },
    { tOPCODE, stMOVE3,   "TEST",     {0x12000}, -1 },
    { tOPCODE, stMOVE3,   "XOR",      {0x0E000}, -1 },
    { tOPCODE, stINST,    "INST",     {0x00000}, -1 },

    // alternative mnemonics
    { tOPCODE, stMOVE3,   "ADDCY",    {0x1A000}, -1 },
    { tOPCODE, stMOVE3,   "COMPARE",  {0x14000}, -1 },
    { tOPCODE, stINTI,    "DISABLE",  {0x3C000}, -1 },
    { tOPCODE, stINTI,    "ENABLE",   {0x3C001}, -1 },
    { tOPCODE, stIO3,     "FETCH",    {0x06000}, -1 },
    { tOPCODE, stIO3,     "INPUT",    {0x04000}, -1 },
    { tOPCODE, stINTE,    "HALT",     {0x3C003}, -1 },
    { tOPCODE, stMOVE3,   "LOAD",     {0x00000}, -1 },
    { tOPCODE, stIO3,     "OUTPUT",   {0x2C000}, -1 },
    { tOPCODE, stCRET3,   "RETURN",   {0x2A000}, -1 },
    { tOPCODE, stINTE,    "RETURNI",  {0x38001}, -1 },
    { tOPCODE, stIO3,     "STORE",    {0x2E000}, -1 },
    { tOPCODE, stMOVE3,   "SUBCY",    {0x1E000}, -1 }
} ;

const symbol_t opcodes6[] = {
    // preferred mnemonics
    { tOPCODE, stMOVE6,   "ADD",      {0x10000}, -1 },
    { tOPCODE, stMOVE6,   "ADDC",     {0x12000}, -1 },
    { tOPCODE, stMOVE6,   "AND",      {0x02000}, -1 },
    { tOPCODE, stCJMP6,   "CALL",     {0x20000}, -1 },
    { tOPCODE, stMOVE6,   "COMP",     {0x1C000}, -1 },
    { tOPCODE, stINT,     "DINT",     {0x28000}, -1 },
    { tOPCODE, stINT,     "EINT",     {0x28001}, -1 },
    { tOPCODE, stIO6,     "IN",       {0x08000}, -1 },
    { tOPCODE, stCJMP6,   "JUMP",     {0x22000}, -1 },
    { tOPCODE, stIO6,     "LD",       {0x0A000}, -1 },
    { tOPCODE, stMOVE6,   "MOVE",     {0x00000}, -1 },
    { tOPCODE, stINT,     "NOP",      {0x00000}, -1 },
    { tOPCODE, stMOVE6,   "OR",       {0x04000}, -1 },
    { tOPCODE, stIO6,     "OUT",      {0x2C000}, -1 },
    { tOPCODE, stCRET6,   "RET",      {0x21000}, -1 },
    { tOPCODE, stINTE,    "RETI",     {0x29001}, -1 },
    { tOPCODE, stSHIFT,   "RL",       {0x14002}, -1 },
    { tOPCODE, stSHIFT,   "RR",       {0x1400C}, -1 },
    { tOPCODE, stCSKP,    "SKIP",     {0x22000}, -1 },
    { tOPCODE, stSHIFT,   "SL0",      {0x14006}, -1 },
    { tOPCODE, stSHIFT,   "SL1",      {0x14007}, -1 },
    { tOPCODE, stSHIFT,   "SLA",      {0x14000}, -1 },
    { tOPCODE, stSHIFT,   "SLX",      {0x14004}, -1 },
    { tOPCODE, stSHIFT,   "SR0",      {0x1400E}, -1 },
    { tOPCODE, stSHIFT,   "SR1",      {0x1400F}, -1 },
    { tOPCODE, stSHIFT,   "SRA",      {0x14008}, -1 },
    { tOPCODE, stSHIFT,   "SRX",      {0x1400A}, -1 },
    { tOPCODE, stIO6,     "ST",       {0x2E000}, -1 },
    { tOPCODE, stMOVE6,   "SUB",      {0x18000}, -1 },
    { tOPCODE, stMOVE6,   "SUBC",     {0x1A000}, -1 },
    { tOPCODE, stMOVE6,   "TEST",     {0x0C000}, -1 },
    { tOPCODE, stMOVE6,   "XOR",      {0x06000}, -1 },
    { tOPCODE, stINST,    "INST",     {0x00000}, -1 },

    // KCPSM6
    { tOPCODE, stMOVE6,   "STAR",     {0x16000}, -1 },
    { tOPCODE, stOUTK,    "OUTK",     {0x2B000}, -1 },
    { tOPCODE, stBANK,    "BANK",     {0x37000}, -1 },
    { tOPCODE, stMOVE6,   "TSTC",     {0x0E000}, -1 },
    { tOPCODE, stMOVE6,   "CMPC",     {0x1E000}, -1 },
    { tOPCODE, stSHIFT,   "CORE",     {0x14080}, -1 },


    // alternative mnemonics
    { tOPCODE, stMOVE6,   "ADDCY",    {0x12000}, -1 },
    { tOPCODE, stMOVE6,   "COMPARE",  {0x1C000}, -1 },
    { tOPCODE, stINTI,    "DISABLE",  {0x28000}, -1 },
    { tOPCODE, stINTI,    "ENABLE",   {0x28001}, -1 },
    { tOPCODE, stIO6,     "FETCH",    {0x0A000}, -1 },
    { tOPCODE, stBREAK,   "BREAK",    {0x23000}, -1 },
    { tOPCODE, stIO6,     "INPUT",    {0x08000}, -1 },
    { tOPCODE, stMOVE6,   "LOAD",     {0x00000}, -1 },
    { tOPCODE, stIO6,     "OUTPUT",   {0x2C000}, -1 },
    { tOPCODE, stCRET6,   "RETURN",   {0x21000}, -1 },
    { tOPCODE, stINTE,    "RETURNI",  {0x29000}, -1 },
    { tOPCODE, stIO6,     "STORE",    {0x2E000}, -1 },
    { tOPCODE, stMOVE6,   "SUBCY",    {0x1A000}, -1 },

    { tOPCODE, stMOVE6,   "STAR",     {0x16000}, -1 },
    { tOPCODE, stOUTK,    "OUTPUTK",  {0x2B000}, -1 },
    { tOPCODE, stBANK,    "REGBANK",  {0x37000}, -1 },
    { tOPCODE, stMOVE6,   "TESTCY",   {0x0E000}, -1 },
    { tOPCODE, stMOVE6,   "COMPARECY",{0x1E000}, -1 },
    { tOPCODE, stSHIFT,   "HWBUILD",  {0x14080}, -1 }
} ;

const symbol_t conditions3[] = {
    { tCONDITION, stNONE, "Z",        {0x01000}, -1 },
    { tCONDITION, stNONE, "C",        {0x01800}, -1 },
    { tCONDITION, stNONE, "NZ",       {0x01400}, -1 },
    { tCONDITION, stNONE, "NC",       {0x01C00}, -1 }
} ;

const symbol_t conditions6[] = {
    { tCONDITION, stNONE, "Z",        {0x10000}, -1 },
    { tCONDITION, stNONE, "NZ",       {0x14000}, -1 },
    { tCONDITION, stNONE, "C",        {0x18000}, -1 },
    { tCONDITION, stNONE, "NC",       {0x1C000}, -1 }
} ;

const symbol_t registers[] = {
    { tREGISTER, stNONE, "S0",        {0}, -1 },
    { tREGISTER, stNONE, "S1",        {1}, -1 },
    { tREGISTER, stNONE, "S2",        {2}, -1 },
    { tREGISTER, stNONE, "S3",        {3}, -1 },
    { tREGISTER, stNONE, "S4",        {4}, -1 },
    { tREGISTER, stNONE, "S5",        {5}, -1 },
    { tREGISTER, stNONE, "S6",        {6}, -1 },
    { tREGISTER, stNONE, "S7",        {7}, -1 },
    { tREGISTER, stNONE, "S8",        {8}, -1 },
    { tREGISTER, stNONE, "S9",        {9}, -1 },
    { tREGISTER, stNONE, "SA",        {0xA}, -1 },
    { tREGISTER, stNONE, "SB",        {0xB}, -1 },
    { tREGISTER, stNONE, "SC",        {0xC}, -1 },
    { tREGISTER, stNONE, "SD",        {0xD}, -1 },
    { tREGISTER, stNONE, "SE",        {0xE}, -1 },
    { tREGISTER, stNONE, "SF",        {0xF}, -1 },
    { tREGISTER, stNONE, "s0",        {0}, -1 },
    { tREGISTER, stNONE, "s1",        {1}, -1 },
    { tREGISTER, stNONE, "s2",        {2}, -1 },
    { tREGISTER, stNONE, "s3",        {3}, -1 },
    { tREGISTER, stNONE, "s4",        {4}, -1 },
    { tREGISTER, stNONE, "s5",        {5}, -1 },
    { tREGISTER, stNONE, "s6",        {6}, -1 },
    { tREGISTER, stNONE, "s7",        {7}, -1 },
    { tREGISTER, stNONE, "s8",        {8}, -1 },
    { tREGISTER, stNONE, "s9",        {9}, -1 },
    { tREGISTER, stNONE, "sA",        {0xA}, -1 },
    { tREGISTER, stNONE, "sB",        {0xB}, -1 },
    { tREGISTER, stNONE, "sC",        {0xC}, -1 },
    { tREGISTER, stNONE, "sD",        {0xD}, -1 },
    { tREGISTER, stNONE, "sE",        {0xE}, -1 },
    { tREGISTER, stNONE, "sF",        {0xF}, -1 }
} ;
