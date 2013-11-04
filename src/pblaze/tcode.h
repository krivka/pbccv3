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

class Operand {
public:
    enum Type {
        None,
        InstructionAddress,
        InstructionPointer,
        Constant8b,
        Port8b,
        Port4b,
        MemoryOrPort,
        Register,
        RegisterInactive
    };
    virtual void printOperand() const = 0;
    virtual Type type() const = 0;
};

class Register : public Operand {
public:
    enum Bank {
        BANK_A,
        BANK_B
    };
    Register(Bank bank, uint8_t no) : m_bank(bank), m_number(no), m_free(true) {}
    inline virtual Type type() const { return Operand::Register; }
    virtual void printOperand() const;
private:
    bool m_bank;
    uint8_t m_number;
    bool m_free;
};


class Instruction {
public:
    virtual const char *name() const = 0;
    virtual void printInstruction() const = 0;

    class TwoOperandInstruction;
        class Load;
        class Star;
        class And;
        class Or;
        class Xor;
        class Add;
        class Sub;
        class Test;
        class Compare;

    class SL;
    class RL;
    class SR;
    class RR;
    class Regbank;
    class Input;
    class Output;
    class Store;
    class Fetch;
    class Interrupt;
    class ReturnI;

    class Conditional;
        class Jump;
        class Call;
        class Return;
    class Hwbuild;

};

class Label : public Operand, public Instruction {
public:
    Label(const char *name) : m_name(name) {}
    virtual Type type() const { return Operand::InstructionAddress; }
    virtual const char *name() const { return m_name; }
    virtual void printInstruction() const { fprintf(stderr, "%s:\n", m_name); }
    virtual void printOperand() const { fprintf(stderr, "%s", m_name); }
private:
    const char *m_name;
};

/**
 * This class assumes the following things:
 * 1) Every of the subclassing instructions does have two operands, in which the first one is a register.
 *    The second one is either a register or a constant (if constantOp2() returns true).
 * 2) Carry: The instruction is in its carry version when the class is constructed with withCarry = true.
 *    Subclassing to set only the operands results in not using the carry option at all.
 *    This basically means arithmetic operations will want the third argument but logical and move operations won't.
 */
class Instruction::TwoOperandInstruction : public Instruction {
public:
    explicit TwoOperandInstruction(const Operand *op1, const Operand *op2, bool withCarry = false);
    virtual void printInstruction() const;
    inline virtual bool constantOp2() { return true; }
protected:
    const Operand *m_op1;
    const Operand *m_op2;
    bool m_carry;
};

class Instruction::Load : public TwoOperandInstruction {
public:
    explicit Load(const Operand *op1, const Operand *op2)
            : TwoOperandInstruction(op1, op2) {}
    inline virtual const char *name() const { return "LOAD"; }
};

class Instruction::Star : public TwoOperandInstruction {
public:
    explicit Star(const Operand *op1, const Operand *op2)
            : TwoOperandInstruction(op1, op2) {}
    inline virtual const char *name() const { return "STAR"; }
    inline virtual bool constantOp2() { return false; }
};

class Instruction::And : public TwoOperandInstruction {
public:
    explicit And(const Operand *op1, const Operand *op2)
            : TwoOperandInstruction(op1, op2) {}
    inline virtual const char *name() const { return "AND"; }
};

class Instruction::Or : public TwoOperandInstruction {
public:
    explicit Or(const Operand *op1, const Operand *op2)
            : TwoOperandInstruction(op1, op2) {}
    inline virtual const char *name() const { return "OR"; }
};

class Instruction::Xor : public TwoOperandInstruction {
public:
    explicit Xor(const Operand *op1, const Operand *op2)
            : TwoOperandInstruction(op1, op2) {}
    inline virtual const char *name() const { return "XOR"; }
};

class Instruction::Add : public TwoOperandInstruction {
public:
    explicit Add(const Operand *op1, const Operand *op2, bool withCarry = false)
            : TwoOperandInstruction(op1, op2, withCarry) {}
    inline virtual const char *name() const { return "ADD"; }
};

class Instruction::Sub : public TwoOperandInstruction {
public:
    explicit Sub(const Operand *op1, const Operand *op2, bool withCarry = false)
            : TwoOperandInstruction(op1, op2, withCarry) {}
    inline virtual const char *name() const { return "SUB"; }
};

class Instruction::Test : public TwoOperandInstruction {
public:
    explicit Test(const Operand *op1, const Operand *op2, bool withCarry = false)
            : TwoOperandInstruction(op1, op2, withCarry) {}
    inline virtual const char *name() const { return "TEST"; }
};

class Instruction::Compare : public TwoOperandInstruction {
public:
    explicit Compare(const Operand *op1, const Operand *op2, bool withCarry = false)
            : TwoOperandInstruction(op1, op2, withCarry) {}
    inline virtual const char *name() const { return "COMPARE"; }
};

class Instruction::Conditional : public Instruction {
public:
    enum Type {
        Plain,
        Zero,
        NotZero,
        Carry,
        NotCarry,
        Indirect
    };
    virtual void printInstruction() const;
protected:
    Conditional(Type type, const Operand *op1, const Operand *op2) : m_type(type), m_op1(op1), m_op2(op2) {}
    Type m_type;
    const Operand *m_op1;
    const Operand *m_op2;
};

class Instruction::Jump : public Conditional {
public:
    explicit Jump(Conditional::Type type, const Label *op1) : Conditional(type, op1, NULL) {}
    explicit Jump(const Register *op1, const Register *op2) : Conditional(Conditional::Indirect, op1, op2) {}
    inline virtual const char *name() const { return "JUMP"; }
};

class Instruction::Call : public Conditional {
public:
    explicit Call(Conditional::Type type, const Label *op1) : Conditional(type, op1, NULL) {}
    explicit Call(const Register *op1, const Register *op2) : Conditional(Conditional::Indirect, op1, op2) {}
    inline virtual const char *name() const { return "CALL"; }
};

class Instruction::Return : public Conditional {
public:
    explicit Return(Conditional::Type type) : Conditional(type, NULL, NULL) {}
    inline virtual const char *name() const { return "RETURN"; }
};

#endif
