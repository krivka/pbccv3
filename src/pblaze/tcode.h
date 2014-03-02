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

#ifndef TCODE_H
#define TCODE_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <sstream>
#include <vector>
#include <map>

#include "common.h"
#include "sdcc.h"

#define BANK_REGISTERS 0x10
// Use only 11 registers for variable storage
#define GP_REGISTERS   0x0B

class Register;
class Operand;
class Variable;
class Memory;
class Stack;

class Operand {
public:
    // creates a null operand to assure safety of operations
    Operand() {

    }
    Operand(std::string name, int size)
            : m_name(name)
            , m_size(size) { 

    }

    static Operand *fromInternal(SDCC::Operand *op);

    int size() const { return m_size; }

    void toMemory();
    void toRegisters();

    void assign(Register *reg, int byte);
    
    std::vector<std::string> getRepresentations() const;

protected:
    std::vector<Register*> m_regs;
    std::string m_name { };
    int m_size { 0 };
};

class Variable : public Operand {
public:
    Variable() : Operand() { }

protected:
    static std::map<std::string, Variable*> m_variableTable;
};

class Register {
public:
    Register(int pos) : m_pos(pos) {}
    bool free() const { return m_op; }

    void assign(Operand *op, int byte) {
        m_op = op;
    }

    void toMemory() {
        m_op->toMemory();
    }

    void clear() {
        m_op = nullptr;
    }

    std::string getRepresentation() const {
        std::stringstream ss;
        ss << "s" << m_pos;
        return ss.str();
    }

    int age() const { return m_age; }

private:
    Operand *m_op { nullptr };
    int m_pos { -1 };
    int m_age { 0 };
};

class Bank {
public:
    Bank() {
        for (int i = 0; i < BANK_REGISTERS; i++) {
            m_regs.emplace(m_regs.end(), Register(i));
        }
    }
    void allocate(Operand *op) {
        // first, try finding a completely free spot of this size
        for (int i = 0; i < GP_REGISTERS; i++) {
            if (m_regs[i].free()) {
                int j;
                for (j = 0; i + j < GP_REGISTERS && j < op->size(); j++) {
                    if (!m_regs[i + j].free())
                        break;
                }
                if (j == op->size()) {
                    for (j = 0; j < op->size(); j++) {
                        m_regs[i + j].assign(op, j);
                    }
                    return;
                }
            }
        }

        // if this fails, try finding the oldest spot of this size
        // oh. yeah, it's a stupid approach but i can't just come here and say 
        // "hey, i want the one that was not USED for a long time" simply 
        // because there's no such information in this state
        int oldest = INT_MAX;
        int oldest_pos = -1;
        for (int i = 0; i < GP_REGISTERS; i++) {
            int current = m_regs[i].age();
            if (current > oldest)
                continue;

            int j;
            for (j = 0; i + j < GP_REGISTERS && j < op->size(); j++) {
                if (current != m_regs[i + j].age())
                    break;
            }
            if (j == op->size()) {
                for (j = 0; j < op->size(); j++) {
                    oldest = current;
                    oldest_pos = i;
                }
            }
        }
        if (oldest_pos != -1) {
            for (int i = 0; i < op->size(); i++) {
                m_regs[oldest_pos + i].toMemory();
                m_regs[oldest_pos + i].assign(op, i);
            }
            return;
        }

        // so there's no free space for this variable? NUKE RANDOM REGISTERS
        // this is heavily depending on the fact the register then deallocates
        // the WHOLE variable it's going to nuke, not just this particular byte
        // which is of course a feature we have but it's not yet implemented
        int base = rand() % (GP_REGISTERS - op->size());
        for (int i = 0; base + i < op->size(); i++) {
            m_regs[base + i].toMemory();
            m_regs[base + i].assign(op, i);
        }
    }

    enum Name {
        BANK_A = 0,
        BANK_B
    };
private:
    std::vector<Register> m_regs;
};

class Memory {
public:
    Memory(int size)
            : m_free(size, true) {

    }
    int size() const { return m_free.size(); }
    bool set(int pos, bool value) {
        if (m_free.size() < pos || m_free[pos] == value)
            return false;
        m_free[pos] = value;
        return true;
    }

private:
    std::vector<bool> m_free;
};

class Stack {
public:
    Stack(Memory *memory)
            : m_memory(memory) {

    }

private:
    Memory *m_memory { nullptr };
    int m_top { 0 };
};

class AsideVars {
public:
    AsideVars(Memory *memory)
            : m_memory(memory)
            , m_top(memory->size() - 1) {

    }
    void push(Operand *op) {
        // TODO here we should save the position of the variable in the memory
        for (int i = 0; i < op->size(); i++) {
            m_memory->set(m_top, false);
            m_top--;
        }
    }

private:
    Memory *m_memory { nullptr };
    // this value will be here most of the time anyway 
    // but set it in the constructor too just to be sure
    int m_top { 255 };
};

class Instruction {
public:
    virtual void print() = 0;
};

class OperandInstruction : public Instruction {
public:
    virtual void print1B() = 0;
    virtual void print2B() {
        print1B();
    }
    virtual void print4B() {
        print2B();
    }

    virtual void print() {
        print1B();
    }
};

class OneOperandInstruction : public OperandInstruction {
public:
    OneOperandInstruction(Operand *op)
            : OperandInstruction()
            , op1(op) { }
    virtual void print() {
        op1->toRegisters();
    }
protected:
    Operand *op1;
};

class ShiftRight : public OneOperandInstruction {
public:
    ShiftRight(Operand *op) : OneOperandInstruction(op) { }
    virtual void print1B();
};

class Processor {
public:
    static Processor *instance() {
        if (!_self)
            _self = new Processor();
        return _self;
    }

    int step() const { return m_step; }

    Bank *currentBank() {
        return &m_banks[m_currBank];
    }

    void allocate(Operand *op) {
        m_step++;
        currentBank()->allocate(op);
    }

    void setAside(Operand *op) {
        m_aside.push(op);
    }
    
    class Flag {
    public:
        bool isClean() const {
            return m_clean;
        }
        void setClean(bool s) {
            m_clean = s;
        }
    private:
        bool m_clean { true };
    };

    Flag carry;

private:
    Processor() {}
    static Processor *_self;

    Bank::Name m_currBank { Bank::BANK_A };
    int m_step { 0 }; // to be able to tell in which iteration of allocation we are

    Bank m_banks[2];
    Memory m_memory { 256 };
    Stack m_stack { &m_memory };
    AsideVars m_aside { &m_memory };
};

#endif
