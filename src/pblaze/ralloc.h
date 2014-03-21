#ifndef PBLAZE_RALLOC_H
#define PBLAZE_RALLOC_H
#ifdef __cplusplus

extern "C" {

#include <stdio.h>

#include "SDCCsymt.h"
#include "SDCCicode.h"
#include "SDCCBBlock.h"

#endif // __cplusplus

void pblaze_assignRegisters(ebbIndex *ebbi);
void pblaze_genCodeLoop(void);

#ifdef __cplusplus
}

#include "util.h"
#include "wrap.h"

#include <vector>

#define REG_CNT 16
#define SEND_REG_CNT 4

class Operand;
class EbbIndex;
class ICode;
class EbBlock;
struct reg_info;
class Register;

class Allocator {
public:
    static void putVal(ICode *ic, Operand *op, int offset);
    Register *getReg(ICode *ic, Operand *op, int offset);
    Register *getTempReg();
    Register *getRegOper(ICode *ic, Operand *op, int offset);
    static void assignRegisters(EbbIndex *ebbi);
    void allocOpInMem(Operand *op);
    static void allocGlobals(ICode *ic);
private:
    Allocator();
    static Allocator *_self;
    int m_tempRegCounter { 0 };
};

class MemoryCell {
public:
    MemoryCell(int addr);
    bool onlyInMem() const { return m_onlyInMem; }

    unsigned int m_addr { UINT_MAX };
    operand *m_currOper { nullptr };
    short m_offset { 0 };
    short m_ptrOffset { 0 };
    short m_nextPart { -1 };
    bool m_reserved { false };
    bool m_global { false };
    bool m_free { true };
    bool m_onlyInMem { false };
};

class Memory {
public:
    const int size { 256 };

    static Memory *instance();

    std::vector<MemoryCell>& cells();
    int getBlock(int size);
    MemoryCell *containsOffset(Operand *op, int offset);
private:
    Memory();

    std::vector<MemoryCell> m_cells;
    static Memory *_self;
};

class Register {
public:
    void lock() {
        m_reserved = true;
    }
    void unlock() {
        if (m_index < REG_CNT - SEND_REG_CNT - 1)
            m_reserved = false;
    }
    unsigned char getIndex() {
        if (this)
            return m_index;
        return 0xFF;
    }
    operator Register*() {
        return nullptr;
    }

    short m_type;
    short m_index;
    char m_name[3];
    Operand *m_oper;
    short m_offset;
    short m_ptrOffset;
    short m_changed;
    bool m_free;
    bool m_reserved;
};

struct reg_info {
    Register r; // :'(
};

class Bank {
public:
    static Bank *current();
    Register *getFirstFree();
    Register *getRegWithIdx(int idx);
    int spillRegsIntoMem(ICode *lic, Operand *op, int offset, int free);

private:
    Register m_regs[REG_CNT];
    static Bank m_banks[2];
    static char m_current;
};

#endif // __cplusplus

#endif // PBLAZE_RALLOC_H
