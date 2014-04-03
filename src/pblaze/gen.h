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

class I {
public:
    virtual string getName() const = 0;

    class Load;
    class Fetch;
    class Store;
    class Add;
    class Sub;
};

class I::Load : public I {
public:
    Load(Register *r, Operand *o)
            : m_reg(r), m_op(o) {

    }
    virtual string getName() const {
        stringstream s;
        s << "load " << "s" << (int) m_reg->getIndex() <<  ", " << m_op->getValue()->getUnsignedLong() << std::endl;
        return s.str();
    }
    Register *m_reg;
    Operand *m_op;
};

class I::Fetch : public I {
public:
    Fetch(Register *r, MemoryCell *m)
            : m_reg(r), m_cell(m) {
        if (!m || !r)
            std::cerr << "NOOOO " << r << " " << m << std::endl;
    }
    virtual string getName() const {
        stringstream s;
        s << "fetch " << "s" << (int) m_reg->getIndex() << ", " << m_cell->m_addr << std::endl;
        return s.str();
    }
    Register *m_reg;
    MemoryCell *m_cell;
};

class I::Store : public I {
public:
    Store(Register *r, MemoryCell *m) {

    }
    virtual string getName() const { return "store"; }
};

class I::Add : public I {
public:
    Add(ICode *ic, unsigned long value) {

    }
    virtual string getName() const { return "add"; }
};

class I::Sub : public I {
public:
    Sub(ICode *ic, unsigned long value) {

    }
    virtual string getName() const { return "sub"; }
};

inline Emitter& operator<<(Emitter &e, const I &i) {
    e << i.getName();
}


#endif // __cplusplus
#endif // PBLAZE_GEN_H

