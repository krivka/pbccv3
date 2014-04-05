#ifndef PBLAZE_GEN_H
#define PBLAZE_GEN_H
#ifdef __cplusplus

#include "common.h"
#include "wrap.h"
#include "ralloc.h"

#include <string>
#include <sstream>

using std::string;
using std::stringstream;

void genPBlazeCode(ICode *lic);


inline stringstream& operator<<(stringstream &ss, Operand *o) {
    int i = Emitter::i;
    if (o->isLiteral) {
        ss << ((o->getValue()->getUnsignedLong() & (0xFF << (i * 8))) >> i * 8);
    }
    else if (o->isSymOp()) {
        if (!o->getSymbol()->regs[i]) {
            o->getSymbol()->regs[i] = Bank::current()->getFreeRegister();
            o->getSymbol()->regs[i]->occupy(o, i);
        }
        ss << o->getSymbol()->regs[i]->getName();
    }
    else {
        std::cerr << "Unknown Emitter Operand type!\n";
    }
    Emitter::i = i;
}

class I {
public:
    virtual string toString() const = 0;

    class Load;
    class Fetch;
    class Store;
    class Add;
    class Sub;
    class Call;
    class Jump;
};

class I::Load : public I {
public:
    Load(Operand *left, Operand *right) : m_l(left), m_r(right) { }
    Load(Operand *left, uint8_t value) : m_l(left), m_value(value) { }
    virtual string toString() const {
        stringstream s;
        s << "load ";
        s << m_l;
        s << ", ";
        if (m_r)
            s << m_r;
        else
            s << (int) m_value;
        return s.str();
    }
private:
    Operand *m_l { nullptr };
    Operand *m_r { nullptr };
    uint8_t m_value;
};

class I::Fetch : public I {
public:
    Fetch(Operand *left, uint8_t addr) : m_l(left), m_addr(addr) { }
    virtual string toString() const {
        stringstream s;
        s << "fetch " << m_l << ", " << std::hex << (int) m_addr;
        return s.str();
    }
private:
    Operand *m_l;
    uint8_t m_addr;
};

class I::Store : public I {
public:
    Store(reg_info *reg, uint8_t addr) : m_reg(reg), m_addr(addr) { }
    virtual string toString() const {
        stringstream s;
        s << "store " << m_reg->getName() << ", " << std::hex << (int) m_addr;
        return s.str();
    }
private:
    reg_info *m_reg;
    uint8_t m_addr;
};

class I::Add : public I {
public:
    Add(Operand *left, Operand *right) : m_l(left), m_r(right) { }
    virtual string toString() const {
        stringstream s;
        s << "add ";
        s << m_l;
        s << ", ";
        s << m_r;
        return s.str();
    }
private:
    Operand *m_l;
    Operand *m_r;
};

class I::Sub : public I {
public:
    Sub(Operand *left, Operand *right) : m_l(left), m_r(right) { }
    virtual string toString() const {
        stringstream s;
        s << "sub " << m_l << ", " << m_r;
        return s.str();
    }
private:
    Operand *m_l;
    Operand *m_r;
};

class I::Call : public I {
public:
    Call(Symbol *func) : m_func(func) { }
    virtual string toString() const {
        stringstream s;
        s << "call " << m_func->rname;
        return s.str();
    }
private:
    Symbol *m_func;
};

class I::Jump : public I {
public:
    Jump(Symbol *label) : m_label(label) { }
    virtual string toString() const {
        stringstream s;
        s << "jump " << m_label->getLabelName();
        return s.str();
    }
private:
    Symbol *m_label;
};

inline Emitter& operator<<(Emitter &e, const I &i) {
    e << "\t" << i.toString() << "\n";
}


#endif // __cplusplus
#endif // PBLAZE_GEN_H

