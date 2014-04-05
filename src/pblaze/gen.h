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


inline Emitter& operator<<(Emitter &e, Operand *o) {
    if (o->isLiteral) {
        e << ((o->getValue()->getUnsignedLong() & (0xFF << (e.i * 8))) >> e.i * 8);
    }
    else if (o->isSymOp()) {
        if (!o->getSymbol()->regs[e.i]) {
            o->getSymbol()->regs[e.i] = Bank::current()->getFreeRegister();
            o->getSymbol()->regs[e.i]->occupy(o, e.i);
        }
        e << o->getSymbol()->regs[e.i]->getName();
    }
    else {
        std::cerr << "Unknown Emitter Operand type!\n";
    }
}

class I {
public:
    virtual void emitSelf() const = 0;

    class Load;
    class Fetch;
    class Store;
    class Add;
    class Sub;
    class Call;
};

class I::Load : public I {
public:
    Load(Operand *left, Operand *right) : m_l(left), m_r(right) { }
    Load(Operand *left, uint8_t value) : m_l(left), m_value(value) { }
    virtual void emitSelf() const {
        emit << "load " << m_l << ", ";
        if (m_r) {
            emit << m_r;
        }
        else {
            emit << (int) m_value;
        }
    }
private:
    Operand *m_l { nullptr };
    Operand *m_r { nullptr };
    uint8_t m_value;
};

class I::Fetch : public I {
public:
    Fetch(Operand *left, uint8_t addr) : m_l(left), m_addr(addr) { }
    virtual void emitSelf() const {
        stringstream s;
        s << std::hex << (int) m_addr;
        emit << "fetch " << m_l << ", " << s.str();
    }
private:
    Operand *m_l;
    uint8_t m_addr;
};

class I::Store : public I {
public:
    Store(reg_info *reg, uint8_t addr) : m_reg(reg), m_addr(addr) { }
    virtual void emitSelf() const {
        stringstream s;
        s << "store " << m_reg->getName() << ", " << std::hex << (int) m_addr;
        emit << s.str();
    }
private:
    reg_info *m_reg;
    uint8_t m_addr;
};

class I::Add : public I {
public:
    Add(Operand *left, Operand *right) : m_l(left), m_r(right) { }
    virtual void emitSelf() const {
        emit << "add " << m_l << ", " << m_r;
    }
private:
    Operand *m_l;
    Operand *m_r;
};

class I::Sub : public I {
public:
    Sub(ICode *ic, unsigned long value) {

    }
    virtual void emitSelf() const {
        emit << "sub";
    }
};

class I::Call : public I {
public:
    Call(Symbol *func) : m_func(func) { }
    virtual void emitSelf() const {
        stringstream s;
        s << "call " << m_func->rname;
        emit << s.str();
    }
private:
    Symbol *m_func;
};

inline Emitter& operator<<(Emitter &e, const I &i) {
    e << "\t";
    i.emitSelf();
    e << "\n";
}


#endif // __cplusplus
#endif // PBLAZE_GEN_H

