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
#include <iostream>

#define REG_CNT 16
#define SEND_REG_CNT 4
#define VAR_REG_CNT REG_CNT - SEND_REG_CNT - 1
#define PBLAZE_FREG 0

class Operand;
class EbbIndex;
class ICode;
class EbBlock;
struct reg_info;
class Register;

class Allocator {
public:
    static void assignRegisters(EbbIndex *ebbi);
private:
    Allocator();
    static Allocator *_self;
};

#define Register reg_info
struct reg_info {
    bool isFree() {
        return m_free;
    }
    void clear() {
        m_free = true;
        m_oper = nullptr;
        m_index = 0;
    }
    void occupy(Operand *o, int index) {
        m_free = false;
        m_oper = o;
        m_index = index;
    }
    bool m_free { true };
    uint8_t m_index { -1 };
    Operand *m_oper { nullptr };
};

class Bank {
public:
    static Bank *current() {
        return m_first ? m_banks : m_banks + 1;
    }
    Register *getFreeRegister(int seq = -1);
private:
    Register m_regs[REG_CNT];

    Bank() { }
    static Bank m_banks[2];
    static bool m_first;
};

#endif // __cplusplus

#endif // PBLAZE_RALLOC_H
