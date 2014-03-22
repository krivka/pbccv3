#ifndef PBLAZE_GEN_H
#define PBLAZE_GEN_H
#ifdef __cplusplus

#include "common.h"
#include "wrap.h"
void genPBlazeCode(ICode *lic);

class I {
public:
    virtual const char *getName() const = 0;

    class Load;
    class Fetch;
    class Store;
    class Add;
    class Sub;
};

class I::Load : public I {
public:
    Load(Register *r, Operand *o) {

    }
    virtual const char *getName() const { return "load"; }
};

class I::Fetch : public I {
public:
    Fetch(Register *r, MemoryCell *m) {

    }
    virtual const char *getName() const { return "fetch"; }
};

class I::Store : public I {
public:
    Store(Register *r, MemoryCell *m) {

    }
    virtual const char *getName() const { return "store"; }
};

class I::Add : public I {
public:
    Add(ICode *ic, unsigned long value) {

    }
    virtual const char *getName() const { return "add"; }
};

class I::Sub : public I {
public:
    Sub(ICode *ic, unsigned long value) {

    }
    virtual const char *getName() const { return "sub"; }
};

inline Emitter& operator<<(Emitter &e, const I &i) {
    e << i.getName();
}


#endif // __cplusplus
#endif // PBLAZE_GEN_H
