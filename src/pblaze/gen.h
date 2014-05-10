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


inline stringstream& operator<<(stringstream &ss, Register *r) {
    ss << r->getName();
}

inline stringstream& operator<<(stringstream &ss, Operand *o) {
    int i = Emitter::i;
    if (o->isLiteral) {
        ss << "0x" << std::hex << ((o->getValue()->getUnsignedLong() & (0xFF << (i << 3))) >> (i << 3));
    }
    else if (o->isSymOp()) {
        if (!o->getSymbol()->regs[i]) {
            o->getSymbol()->regs[i] = Bank::current()->getFreeRegister();
            o->getSymbol()->regs[i]->occupy(o, i);
        }
        ss << o->getSymbol()->regs[i];
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
    class Star;
    class Fetch;
    class Store;
    class RegBank;
    class Add;
    class Sub;
    class Call;
    class Jump;
    class Ret;
    class Compare;
};

class I::Load : public I {
public:
    Load(Operand *left, Operand *right) : m_l(left), m_r(right) { }
    Load(Register *reg, Operand *right) : m_reg(reg), m_r(right) { }
    virtual string toString() const {
        stringstream s;
        s << "load\t";
        if (m_l)
            s << m_l;
        else {
            s << m_reg;
        }
        s << ",\t";
        s << m_r;
        // COMMENT
        s << "\t\t; " << (m_l ? m_l->friendlyName() : m_reg->getName()) << "=" << m_r->friendlyName();
        return s.str();
    }
private:
    Operand *m_l { nullptr };
    Operand *m_r { nullptr };
    Register *m_reg;
    uint8_t m_value;
};

class I::Star : public I {
public:
    Star(Operand *left, Operand *right) : m_l(left), m_r(right) { }
    Star(Register *reg, Operand *right) : m_reg(reg), m_r(right) { }
    virtual string toString() const {
        stringstream s;
        s << "star\t";
        if (m_l)
            s << m_l;
        else {
            s << m_reg;
        }
        s << ",\t";
        s << m_r;
        // COMMENT
        s << "\t\t; " << (m_l ? m_l->friendlyName() : m_reg->getName()) << "=" << m_r->friendlyName();
        return s.str();
    }
private:
    Operand *m_l { nullptr };
    Operand *m_r { nullptr };
    Register *m_reg;
};

class I::Fetch : public I {
public:
    Fetch(Register *reg, Operand *op) : m_reg(reg), m_op(op) { }
    Fetch(Operand *left, uint8_t addr) : m_op(left), m_addr(addr) { }
    virtual string toString() const {
        stringstream s;
        s << "fetch\t";
        if (m_op)
            s << m_op;
        else
            s << m_reg;
        s << ",\t" << std::hex << (int) m_addr;
        return s.str();
    }
private:
    Register *m_reg;
    Operand *m_op;
    uint8_t m_addr;
};

class I::Store : public I {
public:
    Store(reg_info *reg, uint8_t addr) : m_reg(reg), m_addr(addr) { }
    virtual string toString() const {
        stringstream s;
        s << "store\t" << m_reg->getName() << ",\t" << std::hex << (int) m_addr;
        return s.str();
    }
private:
    reg_info *m_reg;
    uint8_t m_addr;
};

class I::RegBank : public I {
public:
    enum Bank {
        A,
        B
    };
    RegBank(RegBank::Bank bank) : m_bank(bank) { }
    virtual string toString() const {
        stringstream s;
        s << "regbank\t";
        s << (m_bank == Bank::A ? "A" : m_bank == Bank::B ? "B" : "");
        return s.str();
    }
private:
    Bank m_bank;
};

class I::Add : public I {
public:
    Add(Operand *left, Operand *right) : m_l(left), m_r(right) { }
    virtual string toString() const {
        if (*m_l == *m_r)
            return string({});
        stringstream s;
        if (Emitter::i == 0)
            s << "add\t";
        else
            s << "addcy\t";
        s << m_l;
        s << ",\t";
        s << m_r;
        // COMMENT
        s << "\t\t; " << m_l->friendlyName()<< "+=" << m_r->friendlyName();
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
        if (Emitter::i == 0)
            s << "sub\t";
        else
            s << "subcy\t";
        s << m_l;
        s << ",\t";
        s << m_r;
        // COMMENT
        s << "\t\t; " << m_l->friendlyName()<< "-=" << m_r->friendlyName();
        return s.str();
    }
private:
    Operand *m_l;
    Operand *m_r;
};

class I::Call : public I {
public:
    Call(ICode *ic) : m_ic(ic) { }
    virtual string toString() const {
        stringstream s;
        s << "call\t" << m_ic->getLeft()->getSymbol()->rname;
        // COMMENT
        s << "\t\t\t; ";
        if (m_ic->getResult())
            s << m_ic->getResult()->friendlyName() << "=";
        s << m_ic->getLeft()->friendlyName() << "()";
        return s.str();
    }
private:
    ICode *m_ic;
};

class I::Jump : public I {
public:
    enum Type {
        NONE, C, Z, NZ, NC
    };
    Jump(Symbol *label, Type t = NONE) : m_label(label), m_type(t) { }
    virtual string toString() const {
        stringstream s;
        s << "jump\t";
        switch (m_type) {
            case C: s << "C,\t"; break;
            case Z: s << "Z,\t"; break;
            case NC: s << "NC,\t"; break;
            case NZ: s << "NZ,\t"; break;
        }
        s << m_label->getLabelName();
        return s.str();
    }
private:
    Symbol *m_label;
    Type m_type;
};

class I::Ret : public I {
public:
    Ret() { }
    virtual string toString() const {
        return "ret";
    }
};

class I::Compare : public I {
public:
    Compare(Operand *l, Operand *r) : m_l(l), m_r(r) { }
    virtual string toString() const {
        stringstream s;
        if (Emitter::i == 0)
            s << "compare\t";
        else
            s << "comparecy\t";
        s << m_l;
        s << ",\t";
        s << m_r;
        return s.str();
    }
private:
    Operand *m_l;
    Operand *m_r;
};

inline Emitter& operator<<(Emitter &e, const I &i) {
    string code = i.toString();
    if (code.empty())
        return e;
    e << "\t" << code << "\n";
    return e;
}


#endif // __cplusplus
#endif // PBLAZE_GEN_H


    struct B;
