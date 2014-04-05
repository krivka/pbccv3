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
    Load(reg_info *reg, uint8_t value) : m_reg(reg), m_value(value) { }
    virtual string getName() const {
        stringstream s;
        s << "load " << m_reg->getName() << ", " << (int) m_value << "\n";
        return s.str();
    }
    reg_info *m_reg;
    uint8_t m_value;
};

class I::Fetch : public I {
public:
    virtual string getName() const {
        stringstream s;
        s << "fetch ";
        return s.str();
    }
};

class I::Store : public I {
public:
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

