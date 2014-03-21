#include "ralloc.h"
#include "gen.h"
#include <iostream>
#include <boost/concept_check.hpp>

Allocator *Allocator::_self = nullptr;
Memory *Memory::_self = nullptr;
Bank Bank::m_banks[2];
char Bank::m_current = 0;


static Set<EbbIndex> *codeSet { nullptr };

Allocator::Allocator() {
}

Register* Allocator::getTempReg() {
    Register *reg = Bank::current()->getRegWithIdx(PBLAZE_NREGS + m_tempRegCounter);

    m_tempRegCounter++;
    if (m_tempRegCounter >= SEND_REG_CNT)
        m_tempRegCounter = 0;

    return reg;
}

Register* Allocator::getRegOper(ICode* ic, Operand* op, int offset) {
    Bank *bank = Bank::current();
    Register *reg;

    // clearUnusedOpFromReg
    if (bank->getFreeCount() == 0) {
        bank->spillRegsIntoMem(ic, op, offset, 1);
    }
}

Register* Allocator::getReg(ICode* ic, Operand* op, int offset) {
    Register *reg;
    MemoryCell *mem;
    if (op->type != SYMBOL)
        return nullptr;

    int size = op->getSymbol()->getType()->getSize();
    if (offset >= size)
        return nullptr;

    reg = &op->getSymbol()->regs[offset]->r;
    mem = Memory::containsOffset(op, offset);

    if (reg->getIndex() < PBLAZE_NREGS) {
        return reg;
    }
    else if (mem != nullptr) {
        if (mem->onlyInMem()) {
            reg = getTempReg();
        }
        else {
            reg = getRegOper(ic, op, offset);
            reg->m_ptrOffset = mem->m_ptrOffset;
        }

        if (reg->getIndex() >= PBLAZE_NREGS && op->isGlobalVolatile()) {
            mem->m_onlyInMem = true;
            op->getSymbol()->regs[offset] = (struct reg_info*) reg;
        }

        if (!mem->onlyInMem()) {
            reg->m_free = false;
            reg->m_oper = op;
            reg->m_offset = offset;

            op->getSymbol()->regs[offset] = (struct reg_info*) reg;
        }

        if (ic->getPrev() || ic->getNext()) {
            emit << I::Fetch(reg, mem);
        }

        if (!op->isGlobal && op->liveTo() && !mem->onlyInMem()) {
            op->freeOffsetFromMem(offset);
        }

        return reg;
    }
    else {
        reg = getRegOper(ic, op, offset);

        if (reg->getIndex() < PBLAZE_NREGS) {
            reg->m_free = false;
            reg->m_oper = op;
            reg->m_offset = offset;
            reg->m_ptrOffset = 0;

            op->getSymbol()->regs[offset] = (struct reg_info*) reg;
            op->getSymbol()->nRegs = size;
        }
        else {
            mem = Memory::instance()->getFirstFree();
            if (!mem) {
                std::cerr << "TODO ERROR PLACEHOLDER" << __FILE__ << __LINE__ << std::endl;
            }

            op->getSymbol()->nRegs = size;
            op->getSymbol()->regs[offset] = (struct reg_info*) reg;
            mem->m_currOper = op;
            mem->m_offset = offset;
            mem->m_ptrOffset = 0;
            mem->m_onlyInMem = true;
            mem->m_free = false;
            mem->m_global = op->isOpGlobal();

            MemoryCell *memTmp = Memory::containsOffset(op, offset + 1);
            mem->m_nextPart = memTmp ? -1 : memTmp->m_addr;
            memTmp = Memory::containsOffset(op, offset - 1);
            if (memTmp)
                memTmp->m_nextPart = mem->m_addr;
        }

        return reg;
    }
    return nullptr;
}

void Allocator::putVal(ICode* ic, Operand* op, int offset) {
    Register *regTmp, *res;
    MemoryCell *m;

    if(op->type == SYMBOL) {
        if (op->isGlobalVolatile() || (op->getType()->isPtr())) {
            m = Memory::instance()->containsOffset(op, offset);
            if (m) {
                regTmp = ic->getRegister();
                regTmp->lock();
                emit << I::Load(regTmp, op);
                emit << I::Store(regTmp, m);
                regTmp->unlock();
            }
            else {
                std::cerr << op->getSymbol()->name << " is not in memory";
            }
        }
        if (op->isOpGlobal()) {

        }
    }
}

void Allocator::allocOpInMem(Operand* op) {
    if (op->isInMem())
        return;

    int next = -1;
    int pos = Memory::instance()->getBlock(op->getType()->getSize());
    MemoryCell *m;

    for (int i = 0; i < op->getType()->getSize(); i++) {
        m = &Memory::instance()->cells()[i];
        m->m_currOper = op;
        m->m_free = false;
        m->m_global = op->isOpGlobal();
        m->m_offset = i;
        m->m_ptrOffset = 0;
        m->m_nextPart = next;
        next = pos--;
    }
}

void Allocator::assignRegisters(EbbIndex* ebbi) {
    if (!_self)
        _self = new Allocator();

    EbBlock **ebbs = ebbi->getBbOrder();
    int count = ebbi->getCount();
    ICode *ic = ICode::fromEbBlock(ebbs, count);
    genPBlazeCode(ic);
}

void Allocator::allocGlobals(ICode* ic) {
    Operand *op;

    if (!ic)
        return;

    for ( ; ic; ic = ic->getNext()) {
        if (!ic->skipNoOp()) {
            if (ic->getLeft() && ic->getLeft()->isOpGlobal()) {
                op = ic->getLeft();
                if (!op->getType()->isFunc() && !op->isInMem())
                    _self->allocOpInMem(op);
            }

            if (ic->getRight() && ic->getRight()->isOpGlobal()) {
                op = ic->getRight();
                if (!op->getType()->isFunc() && !op->isInMem())
                    _self->allocOpInMem(op);
            }

            if (ic->getResult() && ic->getResult()->isOpGlobal()) {
                op = ic->getResult();
                if (!op->getType()->isFunc() && !op->isInMem())
                    _self->allocOpInMem(op);
            }
        }
    }
}

void pblaze_assignRegisters(ebbIndex * ebbi) {
    return Allocator::assignRegisters(static_cast<EbbIndex*>(ebbi));
}

void pblaze_genCodeLoop(void) {
    EbbIndex *ebbi;
    EbBlock **ebbs;
    int count;
    ICode *ic;

    if (codeSet) {
        for (ebbi = codeSet->firstItem(); ebbi; codeSet->nextItem()) {
            ebbs = ebbi->getBbOrder();
            count = ebbi->getCount();

            ic = ICode::fromEbBlock(ebbs, count);
            Allocator::allocGlobals(ic);
        }
        for (ebbi = codeSet->firstItem(); ebbi; codeSet->nextItem()) {
            ebbs = ebbi->getBbOrder();
            count = ebbi->getCount();

            ic = ICode::fromEbBlock(ebbs, count);
            genPBlazeCode(ic);
        }
    }
}


MemoryCell::MemoryCell(int addr) : m_addr(addr) { }

void MemoryCell::setFree() {
    m_currOper = nullptr;
    m_offset = 0;
    m_ptrOffset = 0;
    m_nextPart = 0;
    m_reserved = false;
    m_global = false;
    m_free = true;
    m_onlyInMem = false;
}

Memory::Memory() {
    for (int i = 0; i < size; i++) {
        m_cells.emplace_back(MemoryCell(i));
    }
}

MemoryCell* Memory::getFirstFree() {
    for (int i = 0; i < size; i++) {
        if (m_cells[i].m_free)
            return &m_cells[i];
    }
    return nullptr;
}

MemoryCell* Memory::containsOffset(Operand* op, int offset) {
    Memory *mem = instance();

    if (!op || op->type != SYMBOL)
        return nullptr;

    for (int i = 0; i < size; i++) {
        if (mem->m_cells[i].m_currOper && op == mem->m_cells[i].m_currOper)
            if (mem->m_cells[i].m_offset == offset)
                return &mem->m_cells[i];
    }
}

std::vector<MemoryCell>& Memory::cells() {
    return m_cells;
}

int Memory::getBlock(int size) {
    int count = 0;

    for (int i = 0; i < this->size; i++) {
        if (m_cells[i].m_free && !m_cells[i].m_global) {
            count++;
            if (size <= count)
                return i;
        }
        else {
            count = 0;
        }
    }

    return -1;
}

Memory* Memory::instance() {
    if (!_self)
        _self = new Memory();
    return _self;
}

void Register::setFree() {
    m_free = true;
    m_oper = nullptr;
    m_offset = 0;
    m_ptrOffset = 0;
    m_reserved = 0;
    // m_changed = SAME_VAL; TODO
}

Bank* Bank::current() {
    return &m_banks[m_current];
}

Register* Bank::getFirstFree() {
    for (int i = 0; i < REG_CNT; i++) {
        if (m_regs[i].m_free)
            return m_regs + i;
    }
    return NULL;
}

int Bank::getFreeCount() {
    int count = 0;
    for (int i = PBLAZE_FREG; i < PBLAZE_NREGS; i++) {
        if (m_regs[i].m_free && !m_regs[i].m_reserved) {
            count++;
        }
    }
    return count;
}

Register* Bank::getRegWithIdx(int idx) {
    for (int i = 0; i < PBLAZE_NREGS; i++) {
        if (m_regs[i].m_index == idx)
            return &m_regs[i];
    }
    std::cerr << "TODO PLACEHOLDER " << __FILE__ << __LINE__ << std::endl;
    return nullptr;
}

int Bank::spillRegsIntoMem(ICode* lic, Operand* op, int offset, int free) {
    if (!lic || free <= 0)
        return 0;

    ICode *ic, *temp;
    bitVect *rUse;
    int remainingOp, bStart;

    rUse = newBitVect(PBLAZE_NREGS);

    temp = lic->getFirst();
    if (temp)
        bStart = temp->seq;
    else
        bStart = lic->seq;

    for (int i = PBLAZE_FREG; i < PBLAZE_NREGS; i++) {
        if (m_regs[i].m_reserved
            || m_regs[i].m_free
            || (bStart > m_regs[i].m_oper->liveFrom())
            || lic->isUsedInCurrentInstruction(m_regs[i].m_oper)) {
            bitVectUnSetBit(rUse, i);
        }
        else {
            bitVectSetBit(rUse, i);
        }
    }

    if (bitVectIsZero(rUse)) {
        return 1;
    }

    for (ic = lic; ic; ic = ic->getNext()) {
        if (!ic->skipNoOp()) {
            if (ic->getLeft())
                ic->getLeft()->testOperand(free, rUse);
            if (ic->getRight())
                ic->getRight()->testOperand(free, rUse);
            if (ic->getResult())
                ic->getResult()->testOperand(free, rUse);
        }
        remainingOp = bitVectnBitsOn(rUse);
        if (remainingOp == free)
            break;
    }

    for (int i = PBLAZE_FREG; i < PBLAZE_NREGS; i++) {
        if (bitVectBitValue(rUse, i) && m_regs[i].m_oper->isOpGlobal() && free > getFreeCount()) {
            m_regs[i].m_oper->moveToMemory();
        }
    }

    for (int i = PBLAZE_FREG; i < PBLAZE_NREGS; i++) {
        if (bitVectBitValue(rUse, i) && free > getFreeCount()) {
            m_regs[i].m_oper->moveOffsetToMemory(m_regs[i].m_offset);
        }
    }

    if (remainingOp == free)
        return 0;

    if (getFreeCount() < free)
        return 1;

    return 0;
}



