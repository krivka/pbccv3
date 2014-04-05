#include "ralloc.h"
#include "gen.h"
#include <iostream>

Allocator *Allocator::_self = nullptr;

Allocator::Allocator() {
}

void Allocator::assignRegisters(EbbIndex* ebbi) {
    if (!_self)
        _self = new Allocator();

    EbBlock **ebbs = ebbi->getBbOrder();
    int count = ebbi->getCount();
    ICode *ic = ICode::fromEbBlock(ebbs, count);
    genPBlazeCode(ic);
}

void pblaze_assignRegisters(ebbIndex * ebbi) {
    return Allocator::assignRegisters(static_cast<EbbIndex*>(ebbi));
}

void pblaze_genCodeLoop(void) {

}





Memory *Memory::_self = nullptr;





Bank Bank::m_banks[2];
bool Bank::m_first = true;

Register* Bank::getFreeRegister(int seq) {
    // first look for a free register
    for (int i = 0; i < VAR_REG_CNT; i++) {
        if (m_regs[i].isFree())
            return &m_regs[i];
    }

    // then look for a register containing a dead variable
    for (int i = 0; i < VAR_REG_CNT; i++) {
        if (m_regs[i].m_oper->getSymbol()->liveTo < seq) {
            m_regs[i].clear();
            return &m_regs[i];
        }
    }

    // last, look for a variable that is alive for the longest time
    // TODO: not optimal, improve
    int latest = INT_MAX;
    Register *toFree = nullptr;

    for (int i = 0; i < VAR_REG_CNT; i++) {
        if (m_regs[i].m_oper->getSymbol()->liveFrom < latest) {
            toFree = &m_regs[i];
            latest = m_regs[i].m_oper->getSymbol()->liveFrom;
        }
    }
    toFree->clear();
    return toFree;
}
