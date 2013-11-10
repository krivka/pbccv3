/*
 * Copyright (C) 2013 Martin Bříza <m@rtinbriza.cz>
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

#include "tcode.h"
#include <SDCCicode.h>
#include <SDCCsymt.h>
#include <SDCCy.h>

TCode *TCode::_self = NULL;

extern "C" {
    bool processIcode(iCode* ic) {
        fprintf(stderr, "Yay, processIcode was called!\n");
        switch (ic->op) {
            case FUNCTION:
            case LABEL:
                TCode::instance()->insertLabel(OP_SYMBOL(IC_LEFT(ic))->name);
                break;
            case CALL:
                // TODO parameters
                Instruction *instr = TCode::instance()->requestLabel(IC_LEFT(ic))->rname[0] ? OP_SYMBOL(IC_LEFT(ic))->rname : OP_SYMBOL(IC_LEFT(ic))->name));                
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
#if 0
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
#endif
            default:
                break;
        }
        return false;
    }
}

bool TCode::insertInstruction(Instruction* instr) {
    m_instructions.push_back(instr);
}

bool TCode::insertLabel(const char* name) {
    if (m_labels.find(name) != m_labels.end()) {
        // hm, na tohle ted uz nemam, musim nejak vyresit jak najit, jestli to jmeno uz byla nebo nebyla pouzite, TBD later
    }
    // CONTINUE HERE
    //m_labels
    
}


Instruction::TwoOperandInstruction::TwoOperandInstruction(const Operand *op1, const Operand *op2, bool withCarry)
: m_op1(op1)
, m_op2(op2)
, m_carry(withCarry) {
    
}

void Instruction::TwoOperandInstruction::printInstruction() const {
    if (!m_op1 || !m_op2)
        throw "Two operand instruction tried to use a NULL operand";
    fprintf(stderr, "\t%s%s\t", name(), m_carry ? "CY" : "");
    m_op1->printOperand();
    fprintf(stderr, ",\t");
    m_op2->printOperand();
    fprintf(stderr, "\n");
}

void Instruction::Conditional::printInstruction() const {
    const char *suffix;
    switch(m_type) {
        case Plain:    suffix = "";     break;
        case Zero:     suffix = " Z,";  break;
        case NotZero:  suffix = " NZ,"; break;
        case Carry:    suffix = " C,";  break;
        case NotCarry: suffix = " NC,"; break;
        case Indirect: suffix = "@ ";   break;
        default: throw "Conditional instruction type not supported"; break;
    }
    fprintf(stderr, "\t%s%s\t%s", name(), suffix, m_type == Indirect ? "(" : "");
    if (m_op1)
        m_op1->printOperand();
    if (m_op2) {
        fprintf(stderr, ",\t");
        m_op2->printOperand();
    }
    fprintf(stderr, "%s\n", m_type == Indirect ? ")" : "");
}

void Register::printOperand() const {
    fprintf(stderr, "s%X", m_number);
}