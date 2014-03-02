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

#include <iostream>
#include <string>
#include <vector>

Processor *Processor::_self = nullptr;

extern "C" bool processIcode(iCode* _ic) {
    SDCC::ICode *ic = static_cast<SDCC::ICode*>(_ic);
    switch (ic->op) {
        case RIGHT_OP: {
            Operand *op = Operand::fromInternal(ic->getLeft());
            ShiftRight *i = new ShiftRight(op);
            break;
        }
        default:
            fprintf(stderr, "Something else: %d\n", ic->op);
            break;
    }
    return false;
}

std::vector<std::string> Operand::getRepresentations() const {
    std::vector<std::string> ret;
    for (int i = 0; i < size(); i++) {
        if (m_regs[i])
            ret.push_back(m_regs[i]->getRepresentation());
        // TODO ADD MEMORY and others
    }
}

Operand* Operand::fromInternal(SDCC::Operand* op) {
    return new Variable();
}

void Operand::toMemory() {
    for (Register *r : m_regs) {
        r->clear();
        Processor::instance()->setAside(this);
    }
    m_regs.clear();
}

void Operand::toRegisters() {

}

void Operand::assign(Register* reg, int byte) {
    m_regs.reserve(byte + 1);
    m_regs[byte] = reg;
}

void ShiftRight::print1B() {
    for (std::string r : op1->getRepresentations()) {
        if (Processor::instance()->carry.isClean()) {
            std::cout << "SRA";
        }
        else {
            std::cout << "SR0";
            Processor::instance()->carry.setClean(true);
        }
    }
    Processor::instance()->carry.setClean(false);
}
