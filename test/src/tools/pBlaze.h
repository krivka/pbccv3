/*
 *  Copyright © 2003..2013 : Henk van Kampen <henk@mediatronix.com>
 *
 *  This file is part of pBlazASM.
 *
 *  pBlazASM is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  pBlazASM is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with pBlazASM.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PBLAZE_H
#define PBLAZE_H

#include <stdio.h>
#include <stdint.h>

#define MAXMEM 4096
#define MAXSCR 256
#define MAXIO 256
#define MAXSTK 32
#define MAXREG 16

class IODevice {
public:
    virtual ~IODevice(){}

    virtual uint32_t getValue ( uint32_t address ) { return address ; }
    virtual void setValue ( uint32_t address, uint32_t value ) { (void)address ; (void)value ; }
    virtual void update( void ) {}

    void * w ;

protected :
   int addr ;
} ;


// Picoblaze
class Picoblaze {
    friend class IODevice ;

public:
    Picoblaze( void );
    ~Picoblaze();

    void clearCode( void ) ;
    void clearScratchpad( void ) ;

    void setCore( bool bCore ) {
        bPB3 = bCore ;
    }

    void setExtended( bool bExt ) {
        bExtended = bExt ;
    }

    void setInterruptVector( int address ) {
        intvect = address ;
    }

    void setHWbuild( uint8_t val ) {
        hwbuild = val ;
    }

    bool onBarrier( void ) {
        return pc == barrier ;
    }

    void resetBarrier( void) {
        barrier = -1 ;
    }

    void setBarrier( void ) {
        barrier = pc + 1 ;
    }

    bool onBreakpoint( void ) {
        return Code[ pc ].breakpoint ? true : false ;
    }

    bool getBreakpoint( uint32_t address ) {
        return Code[ address ].breakpoint ? true : false ;
    }

    void resetBreakpoint( int address ) {
        Code[ address ].breakpoint &= 0xFE ;
    }

    void setBreakpoint( int address ) {
        Code[ address ].breakpoint |= 0x01 ;
    }

    void setScratchpadData( uint32_t cell, uint32_t value ) {
        if ( cell < MAXSCR )
            scratchpad[ cell ].value = value ;
    }

    void * getCodeItem( uint32_t address ) {
        return Code[ address ].item ;
    }

    void * getCurrentCodeItem( void ) {
        if ( pc < MAXMEM )
            return Code[ pc ].item ;
        else
            return NULL ;
    }

    uint64_t getCodeCount( uint32_t address ) {
        return Code[ address ].count ;
    }

    void setCodeItem( uint32_t address, uint32_t code, uint32_t line, void * item ){
        Code[ address ].code = code ;
        Code[ address ].line = line ;
        if ( ( ( code & 0xFF000 ) == 0x23000 ) && !bPB3 )
            Code[ address ].breakpoint = 0x02 ; // BREAK
        else
            Code[ address ].breakpoint = 0x00 ;
        Code[ address ].count = 0ll ;
        Code[ address ].item = item ;
    }

    uint32_t getPcValue( void ) {
        return pc ;
    }

    uint32_t getSpValue( void ) {
        return sp ;
    }

    bool getZero() {
        return zero ;
    }

    bool getCarry() {
        return carry ;
    }

    bool getEnable() {
        return enable ;
    }

    void setInterrupt( bool irq ) {
        interrupt = irq ;
    }

    bool getAcknowledge() {
        return acknowledge ;
    }

    int getBank() {
        return bank ;
    }

    uint32_t getStackPcValue( uint32_t address ) {
        return stack[ address ].pc ;
    }

    void * getStackItem ( uint32_t sp ) {
        return stack[ sp ].item ;
    }

    void setStackItem ( uint32_t sp, void * item ) {
        stack[ sp ].pc = 0 ;
        stack[ sp ].zero = false ;
        stack[ sp ].carry = false ;
        stack[ sp ].item = item ;
    }

    void * getScratchpadItem ( uint32_t cell ) {
        return scratchpad[ cell ].item ;
    }

    void setScratchpadItem ( uint32_t cell, void * item ) {
        scratchpad[ cell ].value = 0 ;
        scratchpad[ cell ].item = item ;
    }

    void setScratchpadValue ( uint32_t cell, uint32_t value ) {
        scratchpad[ cell ].value = value ;
    }

    uint32_t getScratchpadValue ( uint32_t cell ) {
        return scratchpad[ cell ].value ;
    }

    void * getRegisterItem( uint32_t reg ) {
        return registers[ 1 ][ reg ].item ;
    }

    void setRegisterItem ( uint32_t reg, void * item ) {
        registers[ 0 ][ reg ].value = 0 ;
        registers[ 0 ][ reg ].item = item ;
        registers[ 1 ][ reg ].value = 0 ;
        registers[ 1 ][ reg ].item = item ;
    }

    void setRegisterValue ( uint32_t cell, uint32_t value ) {
        registers[ bank ][ cell ].value = value ;
    }

    uint32_t getRegisterValue ( uint32_t cell ) {
        return registers[ bank ][ cell ].value ;
    }

    bool isRegisterDefined( int reg ) {
        return registers[ bank ][ reg ].defined ;
    }

    void setIODevice ( void * w, int addr_l, int addr_h, IODevice * device ) {
        device->w = w ;
        for ( int addr = addr_l ; addr <= addr_h ; addr +=1 )
            IO[ addr ].device = device ;
    }

    IODevice * getIODevice( int addr ) {
        return IO[ addr ].device ;
    }

    void initPB( void ) ;
    void resetPB ( void ) ;
    inline bool step ( void ) {
        if ( !enable )
            acknowledge = false ;
        if ( enable && interrupt ) {
            if ( sp > 30 )
                return false ;
            stack[ sp ].pc = pc ;
            stack[ sp ].carry = carry ;
            stack[ sp ].zero = zero ;
            stack[ sp ].bank = bank ;
            sp += 1 ;
            pc = intvect ;
            enable = false ;
            acknowledge = true ;
        }
        return ( bPB3 ) ? stepPB3() : stepPB6() ;
    }

    void dump( void ) {
        printf("////// PicoBlaze processor state dump\n");
        printf("// This file is generated. Any changes done will be overwritten\n\n");
        printf("#include <vector>\n\n");
        printf("std::vector<std::vector<uint8_t>> registers = {\n");
        for (char bank = 0; bank <= 1; bank++) {
            printf("\t{", bank);
            for(int i = 0; i < MAXREG; i++) {
                printf("0x%02hhx", registers[bank][i]);
                if (i < MAXREG - 1)
                    printf(", ");
                else
                    printf("}");
            }
            if (!bank)
                printf(",\n");
            else
                printf("\n};\n");
        }
        printf("\nstd::vector<uint8_t> scratchpad_memory = {\n", MAXSCR);
        printf("///////////////  0x0   0x1   0x2   0x3   0x4   0x5   0x6   0x7   0x8   0x9   0xA   0xB   0xC   0xD   0xE   0xF");
        for (int i = 0; i < MAXSCR; i++) {
            if (i % 16 == 0)
                printf("\n/* 0x%02hhx-0x%02hhx */ ", i, i+16);
            printf("0x%02hhx", scratchpad[i]);
            if (i < MAXSCR - 1)
                printf(", ");
            else
                printf("\n};\n");
        }
        printf("\nbool carry = %s;\n", carry ? "true" : "false");
        printf("bool zero = %s;\n", zero ? "true" : "false");
    }

    uint64_t CycleCounter ;

private:
    typedef struct _inst {
        uint32_t code ;
        unsigned line ;
        int breakpoint ;
        uint64_t count ;
        void * item ;
    } INST_t ;

    typedef struct _stack {
        uint32_t pc ;
        bool zero ;
        bool carry ;
        int bank ;
        void * item ;
    } STACK_t ;

    typedef struct _register {
        uint32_t value ;
        bool defined ;
        void * item ;
    } REG_t ;

    typedef struct _data {
        uint32_t value ;
        void * item ;
    } DATA_t ;

    typedef struct _io {
        IODevice * device ;
        void * item ;
    } IO_t ;

    bool bPB3 ;
    bool bExtended ;

    uint32_t pc, npc, barrier, intvect ;
    uint32_t sp, nsp ;
    int bank ;
    uint8_t hwbuild ;

    INST_t Code[ MAXMEM ] ;
    DATA_t scratchpad[ MAXSCR ] ;
    STACK_t stack[ 32 ] ;
    REG_t registers[ 2 ][ 16 ] ;
    IO_t IO[ MAXSCR ] ;

    bool carry, zero, enable, interrupt, acknowledge ;

    inline uint32_t DestReg ( const int code ) {
        return ( code >> 8 ) & 0xF ;
    }

    inline uint32_t SrcReg ( const int code ) {
        return ( code >> 4 ) & 0xF ;
    }

    inline uint32_t Offset ( const int code ) {
        return ( code >> 0 ) & 0xF ;
    }

    inline uint32_t Constant ( const int code ) {
        return code & 0xFF ;
    }

    inline uint32_t Address12 ( const int code ) {
        if ( bPB3 )
            return code & 0x3FF ;
        else
            return code & 0xFFF ;
    }

    bool stepPB6 ( void ) ;
    bool stepPB3 ( void ) ;
} ;

#endif // PBLAZE_H
