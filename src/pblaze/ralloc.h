#ifndef PBLAZE_RALLOC_H
#define PBLAZE_RALLOC_H
#ifdef __cplusplus

extern "C" {

#include <stdio.h>

#include "main.h"

#include "SDCCsymt.h"
#include "SDCCicode.h"
#include "SDCCBBlock.h"

#endif // __cplusplus

void pblaze_assignRegisters(ebbIndex *ebbi);
void pblaze_genCodeLoop(void);

#define MEMORY_SIZE 256

#define REG_CNT 16
#define ARG_REG_CNT 8
#define VAR_REG_START (Function::registerSize)
#define VAR_REG_END (REG_CNT - 1)
#define VAR_REG_CNT (VAR_REG_END - VAR_REG_START)
#define PBLAZE_FREG 0

#ifdef __cplusplus
}

#include "util.h"
#include "wrap.h"

#include <vector>
#include <iostream>
#include <sstream>
#include <list>

using namespace std;

class Operand;
class EbbIndex;
class ICode;
class EbBlock;
struct reg_info;

class Allocator {
public:
    static void assignRegisters(EbbIndex *ebbi);
    static int seq;
private:
    Allocator();
    static Allocator *_self;
};

struct Byte {
    void clear() {
        m_free = true;
        m_oper = nullptr;
        m_index = 0;
    }
    Byte *occupy(Operand *o, int index) {
        m_free = false;
        m_index = index;
        m_oper = o;
        return this;
    }
    bool isFree() {
        return m_free;
    }

    bool m_free { true };
    uint8_t m_index { 0 };
    Operand *m_oper { nullptr };
};

struct MemoryCell : public Byte {
    uint8_t m_pos { 0 };
};

class Memory {
public:
    static Memory *get() {
        return !_self ? (_self = new Memory()) : _self;
    }
    MemoryCell *occupy(Operand *o, int index) {
        for (int i = MEMORY_SIZE - 1; i >= 0; i--) {
            if (m_cells[i].isFree()) {
                if (m_cells[i].occupy(o, index))
                    return &m_cells[i];
            }
        }
        throw "Ran out of static memory";
    }
    MemoryCell *containsStatic(Operand *o, int index);
    void allocateGlobal(Operand *o);

private:
    MemoryCell m_cells[MEMORY_SIZE];

    Memory() {
        for (int i = 0; i < MEMORY_SIZE; i++) {
            m_cells[i].m_pos = i;
        }
    }
    static Memory *_self;
};

class StackCell : public Byte {
public:
    int m_pos;
};

class Stack {
public:
    static Stack *instance();
    void pushVariable(Operand *op, int index);
    void fetchVariable(Operand *op, int index);
    void preCall();
    void functionStart();
    void functionEnd();
    void loadAddress(Operand *res, Operand *obj);
    void insert(Operand *o, int pos, int offset);
    StackCell *contains(Operand *o, int index);
private:
    static Stack *_self;
    Stack() {
        int i = 0;
        for (StackCell &c: m_mem) {
            c.m_pos = i;
            i++;
        }
    }
    int m_offset;
    int m_lastValue;
    StackCell m_mem[MEMORY_SIZE];
};

#define Register reg_info
struct reg_info : public Byte {
    void clear();
    void purge();
    void occupy(Operand *o, int index) {
//         MemoryCell *cell = Memory::get()->contains(o, index);
        Byte::occupy(o, index);
    }
    bool containsLive(ICode *ic);
    void moveToMemory();

    string getName() {
        stringstream ss;
        ss << "s" << std::hex << std::uppercase << (int) sX;
        return ss.str();
    }
    uint8_t sX { 0 };
};

class Bank {
public:
    static Bank *current() {
        return m_first ? m_banks : m_banks + 1;
    }
    static Bank *other() {
        return m_first ? m_banks + 1 : m_banks;
    }
    static void choose(bool first) {
        m_first = first;
    }
    static void swap();
    Register *getFreeRegister();
    static Register *currentStackPointer() {
        return &(current()->m_regs[REG_CNT - 1]);
    }
    Register *regs() {
        return m_regs;
    }
    Register *contains(Operand *o, int index);
    void purge();
    static void star(Operand *o, unsigned *firstReg);
private:
    Register m_regs[REG_CNT];

    Bank() {
        for (int i = 0; i < REG_CNT; i++)
            m_regs[i].sX = i;
    }
    static Bank m_banks[2];
    static bool m_first;
};

#endif // __cplusplus

#endif // PBLAZE_RALLOC_H
