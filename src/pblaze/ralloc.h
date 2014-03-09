/*-------------------------------------------------------------------------

  SDCCralloc.h - header file register allocation

                Written By -  Sandeep Dutta . sandeep.dutta@usa.net (1998)

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
#include "SDCCicode.h"
#include "SDCCBBlock.h"
#ifndef SDCCRALLOC_H
#define SDCCRALLOC_H 1

#define REG_PTR 0x01 // pointer register
#define REG_GPR 0x02 // general purpose register
#define REG_CND 0x04 // condition (bit) register
#define REG_SCR 0x40 // scratch register
#define REG_STK 0x80 // stack pointer register

// register ID's
enum {
  S0_ID,
  S1_ID,
  S2_ID,
  S3_ID,
  S4_ID,
  S5_ID,
  S6_ID,
  S7_ID,
  S8_ID,
  S9_ID,
  SA_ID,
  SB_ID,
  SC_ID,
  SD_ID,
  SE_ID,
  SF_ID,
};

typedef struct reg_info {
  unsigned char rIdx; // the register ID
  unsigned char size; // size of register (1,2,4)
  unsigned char type; // scratch, pointer, general purpose, stack, condition (bit)
  char *name;
  unsigned long regMask;
  bool isFree;
  symbol *sym; // used by symbol
} regs;

extern regs regsXA51[];
extern unsigned long xa51RegsInUse;

regs *xa51_regWithIdx (int);
void pblaze_assignRegisters (ebbIndex * ebbi);

bitVect *xa51_rUmaskForOp (operand * op);

#endif
