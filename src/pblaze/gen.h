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

void genPBlazeCode(EbbIndex *eb);


inline stringstream& operator<<(stringstream &ss, Register *r) {
    ss << r->getName();
}

stringstream& operator<<(stringstream &ss, Operand *o);

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
    class Or;
    class And;
    class Xor;
    class Call;
    class Jump;
    class Ret;
    class Compare;
    class Test;
    class Output;
    class OutputK;
    class Input;
};

class I::Load : public I {
public:
    Load(Operand *left, Operand *right) : m_l(left), m_r(right) { }
    Load(Operand *left, Register *rreg) : m_l(left), m_rreg(rreg) { }
    Load(Register *reg, Operand *right) : m_reg(reg), m_r(right) { }
    Load(Operand *left, uint8_t value) : m_l(left), m_value(value) { }
    virtual string toString() const {
        stringstream s;
        s << "load\t\t";
        if (m_l)
            s << m_l;
        else {
            s << m_reg;
        }
        s << ",\t";
        if (m_r)
            s << m_r;
        else if (m_rreg)
            s << m_rreg;
        else
            s << "0x" << std::hex << std::uppercase << (unsigned) m_value;
        // COMMENT
        s << "\t\t; " << (m_l ? m_l->friendlyName() : m_reg->getName()) << "[" << Emitter::i << "]=" << (m_r ? m_r->friendlyName() : m_rreg ? m_rreg->getName() : "(value)") << "[" << Emitter::i << "]";
        return s.str();
    }
private:
    Operand *m_l { nullptr };
    Operand *m_r { nullptr };
    Register *m_reg { nullptr };
    Register *m_rreg { nullptr };
    uint8_t m_value;
};

class I::Star : public I {
public:
    Star(Operand *left, Operand *right) : m_l(left), m_r(right) { }
    Star(Register *reg, Operand *right) : m_reg(reg), m_r(right) { }
    Star(Register *reg, Register *right) : m_reg(reg), m_rightreg(right) { }
    virtual string toString() const {
        stringstream s;
        s << "star\t\t";
        if (m_l)
            s << m_l;
        else {
            s << m_reg;
        }
        s << ",\t";
        if (m_r)
            s << m_r;
        else
            s << m_rightreg;
        // COMMENT
        s << "\t\t; (" << (m_l ? m_l->friendlyName() : m_reg->getName()) << ")=" << (m_r ? m_r->friendlyName() : m_rightreg->getName());
        if (m_r)
            s << "[" << Emitter::i << "]";
        return s.str();
    }
private:
    Operand *m_l { nullptr };
    Operand *m_r { nullptr };
    Register *m_reg { nullptr };
    Register *m_rightreg { nullptr };
};

class I::Fetch : public I {
public:
    Fetch(Operand *res, Operand *op) : m_res(res), m_op(op) { }
    Fetch(Register *reg, Operand *op) : m_reg(reg), m_op(op) { }
    Fetch(Register *reg, Register *rreg) : m_reg(reg), m_rreg(rreg) { }
    Fetch(Operand *res, uint8_t addr) : m_res(res), m_addr(addr) { }
    virtual string toString() const {
        stringstream s;
        s << "fetch\t\t";
        if (m_res) {
            s << m_res;
        }
        else {
            s << m_reg;
        }
        if (m_op) {
            s << ",\t(";
            s << m_op;
            s << ")";
        }
        else if (m_rreg) {
            s << ",\t(";
            s << m_rreg;
            s << ")";
        }
        else {
            s << ",\t";
            s << m_addr;
        }
        // COMMENT
        if (m_res) {
            s << "\t\t; ";
            s << m_res->friendlyName();
            s << " < {stack}";
        }
        return s.str();
    }
private:
    Register *m_reg { nullptr };
    Register *m_rreg { nullptr };
    Operand *m_op { nullptr };
    Operand *m_res { nullptr };
    uint8_t m_addr;
};

class I::Store : public I {
public:
    Store(Operand *result, Operand *right) : m_op(result), m_r(right) { }
    Store(Operand *op) : m_op(op) { }
    virtual string toString() const {
        stringstream s;
        s << "store\t\t";
        if (m_r) {
            s << m_r;
            s << ",\t(";
            s << m_op;
            s << ")";
        }
        else {
            s << m_op;
            s << ",\t(";
            s << Bank::currentStackPointer();
            s << ")";
        }
        // COMMENT
        if (m_op) {
            s << "\t\t; {stack} < ";
            s << m_op->friendlyName() << "[" << Emitter::i << "]";
        }
        return s.str();
    }
private:
    uint8_t m_addr;
    Operand *m_op { nullptr };
    Operand *m_r { nullptr };
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
        s << "regbank\t\t";
        s << (m_bank == Bank::A ? "A" : m_bank == Bank::B ? "B" : "");
        return s.str();
    }
private:
    Bank m_bank;
};

class I::Add : public I {
public:
    Add(Operand *left, Operand *right) : m_l(left), m_r(right) { }
    Add(Register *reg, uint8_t val) : m_reg(reg), m_val(val) { }
    virtual string toString() const {
        stringstream s;
        if (*m_l == *m_r)
            return string({});
        if ((!m_val && !m_r)|| (m_r && !m_r->isSymOp() && !((m_r->getValue()->getUnsignedLong() & (0xFF << (Emitter::i << 3))) >> (Emitter::i << 3))))
            return s.str();
        if (Emitter::i == 0 || !m_r)
            s << "add\t\t";
        else
            s << "addcy\t\t";
        if (m_l)
            s << m_l;
        else
            s << m_reg;
        s << ",\t";
        if (m_r)
            s << m_r;
        else
            s << "0x" << std::hex << std::uppercase << (unsigned) m_val;
        // COMMENT
        if (m_reg && 0 == strcmp(m_reg->getName().c_str(), "sF"))
            s << "\t\t; SP+=" << (unsigned) m_val;
        else
            s << "\t\t; " << (m_l ? m_l->friendlyName() : m_reg->getName()) << "[" << Emitter::i << "]+=" << (m_r ? m_r->friendlyName() : "(value)") << "[" << Emitter::i << "]";
        return s.str();
    }
private:
    Operand *m_l { nullptr };
    Operand *m_r { nullptr };
    Register *m_reg { nullptr };
    uint8_t m_val;
};

class I::Sub : public I {
public:
    Sub(Operand *left, Operand *right) : m_l(left), m_r(right) { }
    Sub(Register *reg, uint8_t val) : m_reg(reg), m_val(val) { }
    virtual string toString() const {
        stringstream s;
        if (*m_l == *m_r)
            return string({});
        if ((!m_val && !m_r)|| (m_r && !m_r->isSymOp() && !((m_r->getValue()->getUnsignedLong() & (0xFF << (Emitter::i << 3))) >> (Emitter::i << 3))))
            return s.str();
        if (Emitter::i == 0 || !m_r)
            s << "sub\t\t";
        else
            s << "subcy\t\t";
        if (m_l)
            s << m_l;
        else
            s << m_reg;
        s << ",\t";
        if (m_r)
            s << m_r;
        else
            s << "0x" << std::hex << std::uppercase << (unsigned) m_val;
        // COMMENT
        if (m_reg && 0 == strcmp(m_reg->getName().c_str(), "sF"))
            s << "\t\t; SP-=" << (unsigned) m_val;
        else
            s << "\t\t; " << (m_l ? m_l->friendlyName() : m_reg->getName()) << "[" << Emitter::i << "]+=" << (m_r ? m_r->friendlyName() : "(value)") << "[" << Emitter::i << "]";
        return s.str();
    }
private:
    Operand *m_l { nullptr };
    Operand *m_r { nullptr };
    Register *m_reg { nullptr };
    uint8_t m_val;
};

class I::Or : public I {
public:
    Or(Operand *l, Operand *r) : m_l(l), m_r(r) { }
    virtual string toString() const {
        stringstream s;
        s << "or\t\t";
        s << m_l;
        s << ",\t";
        s << m_r;
        return s.str();
    }
private:
    Operand *m_l { nullptr };
    Operand *m_r { nullptr };
};

class I::And : public I {
public:
    And(Operand *l, Operand *r) : m_l(l), m_r(r) { }
    virtual string toString() const {
        stringstream s;
        s << "and\t\t";
        s << m_l;
        s << ",\t";
        s << m_r;
        return s.str();
    }
private:
    Operand *m_l { nullptr };
    Operand *m_r { nullptr };
};

class I::Xor : public I {
public:
    Xor(Operand *l, Operand *r) : m_l(l), m_r(r) { }
    virtual string toString() const {
        stringstream s;
        s << "xor\t\t";
        s << m_l;
        s << ",\t";
        s << m_r;
        return s.str();
    }
private:
    Operand *m_l { nullptr };
    Operand *m_r { nullptr };
};

class I::Call : public I {
public:
    Call(ICode *ic) : m_ic(ic) { }
    virtual string toString() const {
        stringstream s;
        if (m_ic->op == PCALL) {
            Emitter::i = 1;
            s << "call\t\tsD,\tsE";
        }
        else {
            s << "call\t\t" << m_ic->getLeft()->getSymbol()->rname << "\t";
        }
        // COMMENT
        s << "\t\t; ";
        if (m_ic->getResult())
            s << m_ic->getResult()->friendlyName() << "=";
        s << m_ic->getLeft()->friendlyName() << "()";
        return s.str();
    }
private:
    ICode *m_ic { nullptr };
};

class I::Jump : public I {
public:
    enum Type {
        NONE, C, Z, NZ, NC
    };
    Jump(Symbol *label, Type t = NONE) : m_label(label), m_type(t) { }
    virtual string toString() const {
        stringstream s;
        s << "jump\t\t";
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
    Symbol *m_label { nullptr };
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
            s << "compare\t\t";
        else
            s << "comparecy\t";
        s << m_l;
        s << ",\t";
        s << m_r;
        return s.str();
    }
private:
    Operand *m_l { nullptr };
    Operand *m_r { nullptr };
};

class I::Test : public I {
public:
    Test(Operand *l, Operand *r) : m_l(l), m_r(r) { }
    virtual string toString() const {
        stringstream s;
        if (Emitter::i == 0)
            s << "test\t\t";
        else
            s << "testcy\t\t";
        s << m_l;
        s << ",\t";
        s << m_r;
        return s.str();
    }
private:
    Operand *m_l { nullptr };
    Operand *m_r { nullptr };
};

class I::Output : public I {
public:
    Output(Operand *l, Operand *r) : m_l(l), m_r(r) { }
    virtual string toString() const {
        stringstream s;
        s << "output\t\t";
        s << m_l;
        s << ",\t";
        s << m_r;
        return s.str();
    }
private:
    Operand *m_l {nullptr};
    Operand *m_r {nullptr};
};

class I::OutputK : public I {
public:
    OutputK(Operand *l, Operand *r) : m_l(l), m_r(r) { }
    virtual string toString() const {
        stringstream s;
        s << "outputk\t\t";
        s << m_l;
        s << ",\t";
        s << m_r;
        return s.str();
    }
private:
    Operand *m_l {nullptr};
    Operand *m_r {nullptr};
};

class I::Input : public I {
public:
    Input(Operand *l, Operand *r) : m_l(l), m_r(r) { }
    virtual string toString() const {
        stringstream s;
        s << "input\t\t";
        s << m_l;
        s << ",\t";
        s << m_r;
        return s.str();
    }
private:
    Operand *m_l {nullptr};
    Operand *m_r {nullptr};
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
