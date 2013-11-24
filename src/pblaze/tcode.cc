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
#include "sdcc.h"
#include <SDCCicode.h>
#include <SDCCsymt.h>
#include <SDCCy.h>

TCode *TCode::_self = NULL;

extern "C" bool processIcode(iCode* _ic) {
    PBCC::ICode *ic = static_cast<PBCC::ICode*>(_ic);
    switch (ic->op) {
        case '=':
            //fprintf(stderr, "; %s = %s\n", OP_SYMBOL(IC_RESULT(ic))->rname, OP_VALUE(IC_RIGHT(ic))->sym->);
            //fprintf(stderr, "%s\n", ic->getRight()->getSymbol()->rname);
            break;
        case FUNCTION:
            fprintf(stderr, "; Function\n");
            fprintf(stderr, "\t%s\n", ic->getResult()->getSymbol()->rname);
            break;
        case LABEL:
            if (IC_LABEL(ic) == entryLabel)
                break;
            fprintf(stderr, "; Label\n");
            //fprintf(stderr, "\t%s\n", ic->getLabel()->name);
            break;
        case CALL:
            fprintf(stderr, "; Call\n");
            break;
        case GOTO:
            fprintf(stderr, "; Goto\n");
            break;
        case ENDFUNCTION:
            fprintf(stderr, "; Endfunction\n");
            break;
        case RETURN:
            fprintf(stderr, "; Return\n");
            fprintf(stderr, "\tRETURN\n");
            break;
        case '+':
            fprintf(stderr, "; Add\n");
            break;
        case SEND:
            fprintf(stderr, "; Send\n");
            break;
/*
        case PCALL:
            break;
        case IPUSH:
            break;
        case IPOP:
            break;
        case INLINEASM:
            break;
        case CAST:
            break;
        case '!':
            break;
        case '~':
            break;
        case UNARYMINUS:
            break;
        case '-':
            break;
        case '*':
            break;
        case '/':
            break;
        case '%':
            break;
        case '>':
            break;
        case '<':
            break;
        case LE_OP:
            break;
        case GE_OP:
            break;
        case NE_OP:
            break;
        case EQ_OP:
            break;
        case AND_OP:
            break;
        case OR_OP:
            break;
        case '^':
            break;
        case '|':
            break;
        case BITWISEAND:
            break;
        case RRC:
            break;
        case RLC:
            break;
        case GETHBIT:
            break;
        case LEFT_OP:
            break;
        case RIGHT_OP:
            break;
        case GET_VALUE_AT_ADDRESS:
            break;
        case IFX:
            break;
        case ADDRESS_OF:
            break;
        case JUMPTABLE:
            break;
        case RECEIVE:
            break;
            */
        default:
            fprintf(stderr, "Something else: %d\n", ic->op);
            break;
    }
    return false;
}

bool TCode::insertInstruction(Instruction* instr) {
    m_instructions.push_back(instr);
}

bool TCode::insertLabel(const char* name) {
    auto it = m_promisedLabels.find(name);
    if (it != m_promisedLabels.end()) {
        m_instructions.push_back(it->second);
        m_promisedLabels.erase(it);
    }
    else {
        m_instructions.push_back(new Label(name));
    }
}

Instruction* TCode::requestLabel(const char* name) {
    // TODO this may not be completely... optimal
    auto it = m_promisedLabels.find(name);
    for (const Instruction *i : m_instructions) {
        auto label = static_cast<const Label *>(i);
        if (label) {
//             if (0 == strcmp(label->name(), name))
//                 return label;
        }
    }
    //m_promisedLabels[nam
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

// void Register::printOperand() const {
//     fprintf(stderr, "s%X", m_number);
// }