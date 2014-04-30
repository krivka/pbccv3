/*
 *  Copyright � 2003..2013 : Henk van Kampen <henk@mediatronix.com>
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

/*! \file
 * Implements a closed hash table, with linear probing, with an increment of 1
 */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "pbTypes.h"
#include "pbSymbols.h"
#include "pbOpcodes.h"
#include "pbDirectives.h"

// our symbol table
#define SIZE (4096UL * 4)
static symbol_t symbols[ SIZE ] ;
static int clash = 0 ;

symbol_t stamps[] = {
    { tVALUE, stHOURS,   "__hours__",   {0}, -1  },
    { tVALUE, stMINUTES, "__minutes__", {1}, -1  },
    { tVALUE, stSECONDS, "__seconds__", {2}, -1  },
    { tVALUE, stYEAR,    "__year__",    {3}, -1  },
    { tVALUE, stMONTH,   "__month__",   {4}, -1  },
    { tVALUE, stDAY,     "__day__",     {5}, -1  },

    { tVALUE, stSTRING,  "__time__",    {0}, -1  },
    { tVALUE, stSTRING,  "__date__",    {0}, -1  }
} ;

// fast bytewide crc16 (ITU)
static inline void crc16( uint8_t data, uint16_t * crc ) {
  uint16_t c = ( ( *crc >> 8 ) ^ data ) & 0xff ;
  c ^= c >> 4 ;
  *crc = ( *crc << 8 ) ^ ( c << 12 ) ^ ( c << 5 ) ^ c ;
}



//! \fn static uint32_t hash( const char * text )
//  \param text string to hash
static uint32_t hash ( const char * text ) {
    uint16_t crc = 0xFFFF ;
    char * p = (char *)text ;

    if ( p != NULL ) {
        while ( *p != 0 )
            crc16( *p++, &crc ) ;
        return crc & ( SIZE - 1 ) ;
    } else
        return 0xFFFFFFFF ;
}

// add all known words, our keywords
static void add_keyword ( const symbol_t * sym ) {
    add_symbol ( sym->type, sym->subtype, sym->text, sym->value, sym->filenbr ) ;
}

void init_symbol ( bool b6 ) {
    time_t timer ;
    struct tm * ptime ;
    value_t zero ;
    int h ;

    // clear table
    for ( h = 0 ; h < (int)SIZE ; h += 1 ) {
        symbols[ h ].type = tNONE ;
        symbols[ h ].subtype = stNONE ;
        symbols[ h ].text = NULL ;
        symbols[ h ].value.integer = 0 ;
        symbols[ h ].filenbr = -1 ;
    }
    clash = 0 ;
    // add keywords
    if ( b6 )
        for ( h = 0 ; h < (int)sizeof ( opcodes6 ) / (int)sizeof ( symbol_t ) ; h += 1 )
            add_keyword ( &opcodes6[ h ] ) ;
    else
        for ( h = 0 ; h < (int)sizeof ( opcodes3 ) / (int)sizeof ( symbol_t ) ; h += 1 )
            add_keyword ( &opcodes3[ h ] ) ;

    if ( b6 )
        for ( h = 0 ; h < (int)sizeof ( conditions6 ) / (int)sizeof ( symbol_t ) ; h += 1 )
            add_keyword ( &conditions6[ h ] ) ;
    else
        for ( h = 0 ; h < (int)sizeof ( conditions3 ) / (int)sizeof ( symbol_t ) ; h += 1 )
            add_keyword ( &conditions3[ h ] ) ;

    for ( h = 0 ; h < (int)sizeof ( directives ) / (int)sizeof ( symbol_t ) ; h += 1 )
        add_keyword ( &directives[ h ] ) ;

    for ( h = 0 ; h < (int)sizeof ( registers ) / (int)sizeof ( symbol_t ) ; h += 1 )
        add_keyword ( &registers[ h ] ) ;

    time ( &timer ) ;
    ptime = localtime ( &timer ) ;
    stamps[ 0 ].value.integer = ptime->tm_hour ;
    stamps[ 1 ].value.integer = ptime->tm_min ;
    stamps[ 2 ].value.integer = ptime->tm_sec ;
    stamps[ 3 ].value.integer = ptime->tm_year - 100 ;
    stamps[ 4 ].value.integer = ptime->tm_mon + 1 ;
    stamps[ 5 ].value.integer = ptime->tm_mday ;
    stamps[ 6 ].value.string = strdup ( __TIME__ ) ;
    stamps[ 7 ].value.string = strdup ( __DATE__ ) ;

    for ( h = 0 ; h < (int)sizeof ( stamps ) / (int)sizeof ( symbol_t ) ; h += 1 )
        add_keyword ( &stamps[ h ] ) ;

    zero.integer = 0 ;
    add_symbol ( tVALUE, stDOT, ".", zero, -1 ) ;
}

// find a symbol, returns the found symbol, or NULL
symbol_t * find_symbol ( const char * text, bool bUpper, int file_nbr ) {
    int h ;
    uint32_t p ;
    char buf[ 256 ], *s ;

    (void) file_nbr ;

    if ( !text )
        return NULL ;

    // uppercase only?
    strcpy ( buf, text ) ;
    for ( s = buf ; bUpper && *s != '\0' ; s++ )
        *s = toupper ( *s ) ;

    // compute 1st entry
    p = hash( buf ) ;
    if ( p == 0xFFFFFFFF )
        return NULL ;
    // if empty spot, not found
    if ( symbols[ p ].text == NULL )
        return NULL ;
    // if text is equal, found
    if ( strcmp ( symbols[ p ].text, buf ) == 0 /* && ( file_nbr == -1 || symbols[ p ].type != tLABEL || symbols[ p ].filenbr == -1 || symbols[ p ].filenbr == file_nbr ) */ )
        return &symbols[ p ] ;

    // else maybe next entry
    for ( h = ( p + 1 ) & ( SIZE - 1 ) ; h != (int)p ; h = ( h + 1 ) & ( SIZE - 1 ) ) {
        if ( symbols[ h ].text == NULL )
            return NULL ;
        if ( strcmp ( symbols[ h ].text, buf ) == 0 /* && ( file_nbr == -1 || symbols[ h ].type != tLABEL || symbols[ h ].filenbr == -1 || symbols[ h ].filenbr == file_nbr ) */ )
            return &symbols[ h ] ;
    }
    return NULL ; // unlikely
}

// add a symbol, rehashing is by linear probing
// returns false if we want to add an already known symbol
bool add_symbol ( const type_e type, const subtype_e subtype, const char * text, const value_t value, int file_nbr ) {
    uint32_t p = hash( text ) ;
    int h = p ;

    if ( p == 0xFFFFFFFF )
        return false ;
    do {
        if ( symbols[ h ].text == NULL ) { // if empty spot, put it here
            symbols[ h ].type = type ;
            symbols[ h ].subtype = subtype ;
            symbols[ h ].text = strdup ( text ) ;
            symbols[ h ].value = value ;
            symbols[ h ].filenbr = file_nbr ;
            return true ;
        } else if ( strcmp ( symbols[ h ].text, text ) == 0 ) { // if text is equal, already there?
            if ( symbols[ h ].type == type && symbols[ h ].subtype == stSET )
                symbols[ h ].value = value ;
            else if ( symbols[ h ].type == type && symbols[ h ].subtype == subtype && symbols[ h ].value.integer == value.integer )
                return false ; // really same?
        }
        h = ( h + 1 ) & ( SIZE - 1 ) ; // wrap
        clash += 1 ;
    } while ( h != (int)p ) ; // full ?
    return false ;
}

// dump the recorded symbols
#ifdef _DEBUG_
void dump_map ( void ) {
    unsigned int h = 0 ;
    int count = 0 ;

    for ( h = 0 ; h < SIZE ; h += 1 )
        if ( symbols[ h ].type != tNONE && symbols[ h ].text != NULL )
            printf (
                "%4d-%5d: %-64s, v:0x%08x, tp:%2d, sb:%2d\n", count += 1, h, symbols[ h ].text, symbols[ h ].value.integer, symbols[ h ].type, symbols[ h ].subtype ) ;
}
#endif

// free any allocated storage
void free_symbol ( void ) {
    int h, i, c = 0 ;

#ifdef _DEBUG_MAP_
    dump_map();
#endif
    for ( h = 0 ; h < (int)SIZE ; h += 1 ) {
        if ( symbols[ h ].type != tNONE )
            c += 1 ;

        if ( symbols[ h ].text != NULL )
            free ( symbols[ h ].text ) ;
        if ( symbols[ h ].subtype == stSTRING && symbols[ h ].value.string != NULL )
            free ( symbols[ h ].value.string ) ;
        else if ( symbols[ h ].subtype == stTOKENS && symbols[ h ].value.tokens != NULL ) {
            symbol_t * s = symbols[ h ].value.tokens ;
            for ( i = 0 ; s[ i ].text != NULL ; i += 1 )
                free ( s[ i ].text ) ;
            free ( s ) ;
        }
        symbols[ h ].type = tNONE ;
        symbols[ h ].text = NULL ;
        symbols[ h ].value.integer = 0 ;
    }
#ifdef _DEBUG_
    printf( "pBlazASM: %d symbols used, %d clashes\n", c, clash ) ;
#endif
}

