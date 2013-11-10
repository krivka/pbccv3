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

#include <vector>
#include <map>

class TCode {
public:
    static TCode *instance() {
        if (!_self)
            _self = new TCode();
        return _self;
    }
    bool insertInstruction(Instruction *instr);

    bool insertLabel(const char *name);
    Instruction *requestLabel(const char *name);
private:
    TCode() {}
    static TCode *_self;

    std::vector<Instruction *> m_instructions;
    std::map<const char *, Label *> m_labels;
};

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
    
    class Register;
    class Constant;
    class Port;
};

class Operand::Register : public Operand {
public:
    enum Bank {
        BANK_A,
        BANK_B
    };
    Register(Bank bank, uint8_t no) : m_bank(bank), m_number(no), m_free(true) {}
    virtual Type type() const { return Operand::Register; }
    virtual void printOperand() const;
private:
    bool m_bank;
    uint8_t m_number;
    bool m_free;
};

//TBD
class Operand::Constant : public Operand {
    
};

class Operand::Port : public Operand {
    
};

class Instruction {
public:
    virtual const char *name() const {
        return "reimplement this!";
    }
    virtual void printInstruction() const {
        fprintf(stderr, "\t%s\n", name());
    }

    // The classes are in the same order as in the KCPSM manual
protected:
    class TwoOperandInstruction;
public: // inheriting from TwoOperandInstruction
        class Load;
        class Star;
        class And;
        class Or;
        class Xor;
        class Add;
        class Sub;
        class Test;
        class Compare;

public: // direct children
        class Shift;
        class Regbank;
        class Input;
        class Output;
        class Store;
        class Fetch;
        class Interrupt;
        class ReturnI;

protected:
    class Conditional;
public: // inheriting from Conditional
        class Jump;
        class Call;
        class Return;

public: // direct children
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
 *    The second one is either a register or a constant.
 * 2) Carry: The instruction is in its carry version when the class is constructed with withCarry = true.
 *    Subclassing to set only the operands results in not using the carry option at all.
 *    This basically means arithmetic operations will want the third argument but logical and move operations won't.
 */
class Instruction::TwoOperandInstruction : public Instruction {
public:
    explicit TwoOperandInstruction(const Operand *op1, const Operand *op2, bool withCarry = false);
    virtual void printInstruction() const;
protected:
    const Operand *m_op1;
    const Operand *m_op2;
    bool m_carry;
};

class Instruction::Load : public TwoOperandInstruction {
public:
    explicit Load(const Operand *op1, const Operand *op2)
            : TwoOperandInstruction(op1, op2) {}
    virtual const char *name() const { return "LOAD"; }
};

class Instruction::Star : public TwoOperandInstruction {
public:
    explicit Star(const Operand::Register *op1, const Operand::Register *op2)
            : TwoOperandInstruction(op1, op2) {}
    virtual const char *name() const { return "STAR"; }
    virtual bool constantOp2() { return false; }
};

class Instruction::And : public TwoOperandInstruction {
public:
    explicit And(const Operand *op1, const Operand *op2)
            : TwoOperandInstruction(op1, op2) {}
    virtual const char *name() const { return "AND"; }
};

class Instruction::Or : public TwoOperandInstruction {
public:
    explicit Or(const Operand *op1, const Operand *op2)
            : TwoOperandInstruction(op1, op2) {}
    virtual const char *name() const { return "OR"; }
};

class Instruction::Xor : public TwoOperandInstruction {
public:
    explicit Xor(const Operand *op1, const Operand *op2)
            : TwoOperandInstruction(op1, op2) {}
    virtual const char *name() const { return "XOR"; }
};

class Instruction::Add : public TwoOperandInstruction {
public:
    explicit Add(const Operand *op1, const Operand *op2, bool withCarry = false)
            : TwoOperandInstruction(op1, op2, withCarry) {}
    virtual const char *name() const { return "ADD"; }
};

class Instruction::Sub : public TwoOperandInstruction {
public:
    explicit Sub(const Operand *op1, const Operand *op2, bool withCarry = false)
            : TwoOperandInstruction(op1, op2, withCarry) {}
    virtual const char *name() const { return "SUB"; }
};

class Instruction::Test : public TwoOperandInstruction {
public:
    explicit Test(const Operand *op1, const Operand *op2, bool withCarry = false)
            : TwoOperandInstruction(op1, op2, withCarry) {}
    virtual const char *name() const { return "TEST"; }
};

class Instruction::Compare : public TwoOperandInstruction {
public:
    explicit Compare(const Operand *op1, const Operand *op2, bool withCarry = false)
            : TwoOperandInstruction(op1, op2, withCarry) {}
    virtual const char *name() const { return "COMPARE"; }
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
    explicit Jump(Conditional::Type type, const Label *op1)
            : Conditional(type, op1, NULL) {}
    explicit Jump(const Operand::Register *op1, const Operand::Register *op2)
            : Conditional(Conditional::Indirect, op1, op2) {}
    virtual const char *name() const { return "JUMP"; }
};

class Instruction::Call : public Conditional {
public:
    explicit Call(Conditional::Type type, const Label *op1)
            : Conditional(type, op1, NULL) {}
    explicit Call(const Operand::Register *op1, const Operand::Register *op2)
            : Conditional(Conditional::Indirect, op1, op2) {}
    virtual const char *name() const { return "CALL"; }
};

class Instruction::Return : public Conditional {
public:
    explicit Return(Conditional::Type type)
            : Conditional(type, NULL, NULL) {}
    virtual const char *name() const { return "RETURN"; }
};

class Instruction::Shift : public Instruction {
public:
    Shift(const Operand::Register *op1, Direction dir, Style style = Fill0)
            : Instruction(), m_op1(op1), m_direction(dir), m_style(style) {}
    virtual const char *name() const {
        if (m_direction == Right)
            switch(m_style) {
                case Fill0: return "SR0";
                case Fill1: return "SR1";
                case FillMSB: return "SRX";
                case FillCarry: return "SRA";
                case Rotate: return "RR";
            }
        else
            switch(m_style) {
                case Fill0: return "SL0";
                case Fill1: return "SL1";
                case FillMSB: return "SLX";
                case FillCarry: return "SLA";
                case Rotate: return "RL";
            }
    }
    enum Direction {
        Right,
        Left
    };
    enum Style {
        Fill0,
        Fill1,
        FillMSB,
        FillCarry,
        Rotate
    };
private:
    const Operand::Register *m_op1;
    Direction m_direction;
    Style m_style;
};

class Instruction::Input : public TwoOperandInstruction {
public:
    explicit Input(const Operand::Register *op1, const Operand *op2)
            : TwoOperandInstruction(op1, op2) {}
    virtual const char *name() const { return "INPUT"; }
};

class Instruction::Output : public TwoOperandInstruction {
public:
    explicit Output(const Operand::Register *op1, const Operand *op2)
            : TwoOperandInstruction(op1, op2) {}
    explicit Output(const Operand::Constant *op1, const Operand::Port *op2)
            : TwoOperandInstruction(op1, op2) {}
    virtual const char *name() const { return m_op2->type() == Operand::Port4b ? "OUTPUTK" : "OUTPUT"; }
};

class Instruction::Store : public TwoOperandInstruction {
public:
    explicit Store(const Operand *op1, const Operand *op2)
            : TwoOperandInstruction(op1, op2) {}
    virtual const char *name() const { return "STORE"; }
};

class Instruction::Fetch : public TwoOperandInstruction {
    explicit Fetch(const Operand *op1, const Operand *op2)
            : TwoOperandInstruction(op1, op2) {}
    virtual const char *name() const { return "FETCH"; }
};

class Instruction::ReturnI : public Instruction {
public:
    explicit ReturnI(bool state)
            : Instruction(), m_state(state) {}
    virtual const char *name() const { return m_state ? "RETURNI ENABLE" : "RETURNI DISABLE"; }
private:
    bool m_state;
};

class Instruction::Interrupt : public Instruction {
public:
    explicit Interrupt(bool state)
            : Instruction(), m_state(state) {}
    virtual const char *name() const { return m_state ? "ENABLE INTERRUPT" : "DISABLE INTERRUPT"; }
private:
    bool m_state;
};

class Instruction::Hwbuild : public Instruction {
public:
    explicit Hwbuild(const Operand::Register *op1)
            : Instruction(), m_op1(op1) {}
    virtual const char *name() const { return "HWBUILD"; }
private:
    const Operand::Register *m_op1;
};

#endif

struct C;
