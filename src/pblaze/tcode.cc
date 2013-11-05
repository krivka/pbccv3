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

Instruction::TwoOperandInstruction::TwoOperandInstruction(const Operand *op1, const Operand *op2, bool withCarry)
: m_op1(op1)
, m_op2(op2)
, m_carry(withCarry) {
    
}

void Instruction::TwoOperandInstruction::printInstruction() const {
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