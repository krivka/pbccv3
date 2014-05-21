#!/bin/bash

autoconf
./configure --disable-{pic14,pic16,hc08,stm8,ds390,ds400,z80,mcs51,s08,z180,r2k,r3ka,gbz80,tlcs90}-port --disable-sdbinutils --disable-device-lib --disable-ucsim --disable-sdcdb --disable-packihx
make -j5
