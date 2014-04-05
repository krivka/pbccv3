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





reg_info* Bank::getFreeRegister()
{

}
