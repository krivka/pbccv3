/*
 *  Copyright � 2003..2013 : Henk van Kampen <henk@mediatronix.com>
 *
 *	This file is part of pBlazASM.
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

#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "pbTypes.h"
#include "pbErrors.h"

// lexer states
typedef enum _LEX_STATE {
    lsBin,
    lsChar,
    lsColon,
    lsComment,
    lsCStyleComment,
    lsDec,
    lsCopy,
    lsError,
    lsHex,
    lsOct,
    lsHexBin,
    lsIdent,
    lsIdle,
    lsInit,
    lsOperator,
    lsDoubleOp,
    lsPunct,
    lsIndex,
    lsString,
    lsParam
} LexState_e ;

// global token list
static symbol_t tokens[ 256 ] ; // global token list
static symbol_t * ptok = NULL ; // pointer to current token, index in 'tokens[]'

symbol_t * tok_first( void ) {
    ptok = tokens ;
    return ptok ;
}

inline symbol_t * tok_current( void ) {
//    printf( "%s\n", ptok->text ) ;
    return ptok ;
}

symbol_t * tok_next( void ) {
    if ( ptok < &tokens[ 256 ] )
        return ptok++ ;
    else {
        ptok->type = tNONE ;
        return ptok ;
    }
}

void tok_back(symbol_t * back ) {
    ptok = back ;
}

void tok_free( void ) {
    for ( ptok = tokens ; ptok->text != NULL ; ptok++ ) {
        free( ptok->text ) ;

        ptok->type = tNONE ;
        ptok->subtype = stNONE ;
        ptok->text = NULL ;
        ptok->value.integer = 0 ;
    }
}

// state machine based lexer
// tokens are recorded in 'tokens', ended by a NONE token
bool lex( char * line, const bool mode ) {
    static char term[ 4096 ] ;
    char * start = NULL, * end = NULL, * s = line, * pterm = NULL ;
    LexState_e state = lsInit ;

    // state machine
    for ( ptok = tokens ; ptok < &tokens[ 256 ] ; ) {
        switch ( state ) {
        case lsInit :
            ptok->type = tNONE ;
            ptok->subtype = stNONE ;
            ptok->value.integer = 0 ;
            ptok->text = NULL ;

            pterm = term ;
            *pterm = '\0' ;
            state = lsIdle ;
            break ;

        case lsIdle :
            // starting characters of tokens to be
            if ( *s == '\0' || *s == '\r' || *s == '\n' ) {
                // end of line
                return true ;
            } else if ( *s == ' ' || iscntrl( *s ) ) {
                // white space, 'space' and all control characters, except \0, \r and \n
                s++ ;
            } else if ( mode && ( isalnum( *s ) ) ) {
                // KCPSM mode, all alphanum is accepted for idents, could be hex values
                // ident
                start = s++ ;
                state = lsIdent ;
            } else if ( !mode && ( isalpha( *s ) || *s == '_' ) ) {
                // ident
                start = s++ ;
                state = lsIdent ;
            } else if ( *s == ';' ) {
                // comment
                start = s++ ;
                state = lsComment ;
            } else if ( *s == '/' ) {
                // comment
                start = s++ ;
                state = lsCStyleComment ;
            } else if ( *s == '0' ) {
                // maybe hex, oct or bin
                start = s++ ;
                state = lsHexBin ;
            } else if ( isdigit( *s ) ) {
                // decimal number
                start = s++ ;
                state = lsDec ;
            } else if ( *s == '$' ) {
                // hexadecimal number
                start = ++s ;
                state = lsHex ;
            } else if ( *s == '#' ) {
                // binary number
                start = ++s ;
                state = lsBin ;
            } else if ( *s == '.' ) {
                // directives, indexing, local labels, etc
                start = s++ ;
                state = lsIndex ;
            } else if ( *s == ',' || *s == '(' || *s == ')' ) {
                // punctuation ',', ':', '(', ')', '[', ']'
                start = s++ ;
                state = lsPunct ;
            } else if ( *s == ':' ) {
                // colon, maybe double colon
                start = s++ ;
                state = lsColon ;
            } else if ( *s == '@' ) {
                // params '@'
                start = s++ ;
                state = lsParam ;
            } else if ( *s == '*' || *s == '/' || *s == '%' || *s == '+' || *s == '-' ||
                        *s == '|' || *s == '&' || *s == '^' || *s == '~' ) {
                // operators
                start = s++ ;
                state = lsOperator ;
            } else if ( *s == '<' || *s == '>' ) {
                // double char operators
                start = s++ ;
                state = lsDoubleOp ;
            } else if ( *s == '\'' ) {
                // 'c'
                start = ++s ;
                state = lsChar ;
            } else if ( *s == '"' ) {
                // "string"
                start = ++s ;
                state = lsString ;
            } else
                state = lsError ;
            break ;

        case lsCStyleComment :
            if ( *s == '/' ) {
                s++ ;
                state = lsComment ;
            } else
                state = lsOperator ;
            break ;

        case lsColon :
            if ( *s == ':' ) {
                end = ++s ;
                ptok->type = tDCOLON ;
                state = lsCopy ;
          } else
                state = lsPunct ;
            break ;

        case lsComment :
            if ( *s != '\0' && *s != '\r' && *s != '\n' )
                // anything till end of line
                s++ ;
            else {
                end = s ;
                ptok->type = tNONE ;
                ptok->subtype = stCOMMENT ;
                state = lsCopy ;
            }
            break ;

        case lsChar :
            if ( *s == '\'' ) {
                ptok->type = tCHAR ;
                end = s++ ;
                state = lsCopy ;
            } else if ( *s == '\\' ) {
                s += 1 ;
                if ( *s != '\0' )
                    s += 1 ;
            } else if ( isgraph( *s ) || *s == ' ' ) {
                s++ ;
            } else
                state = lsError ;
            break ;

        case lsString :
            if ( *s == '"' ) {
                ptok->type = tSTRING ;
                end = s++ ;
                state = lsCopy ;
            } else if ( *s == '\\' ) {
                s += 1 ;
                if ( *s != '\0' )
                    s += 1 ;
            } else if ( isgraph( *s ) || *s == ' ' )
                s++ ;
            else
                state = lsError ;
            break ;

        case lsIdent :
            if ( isalnum( *s ) || *s == '_' )
                s++ ;
            else {
                end = s ;
                ptok->type = tIDENT ;
                ptok->subtype = stNONE ;
                state = lsCopy ;
            }
            break ;

        case lsHexBin :
            if ( *s == 'x' ) {
                start = ++s ;
                state = lsHex ;
            } else if ( *s == 'o' ) {
                start = ++s ;
                state = lsOct ;
            } else if ( *s == 'b' ) {
                start = ++s ;
                state = lsBin ;
            } else
                // missing the first '0' doesn't hurt here
                state = lsDec ;
            break ;

        case lsHex :
            if ( isxdigit( *s ) || *s == '_'  )
                s++ ;
            else {
                end = s ;
                ptok->type = tHEX ;
                state = lsCopy ;
            }
            break ;

        case lsBin :
            if ( *s == '0' || *s == '1' || *s == '_' )
                s++ ;
            else {
                end = s ;
                ptok->type = tBIN ;
                state = lsCopy ;
            }
            break ;

        case lsOct :
            if ( ( *s >= '0' && *s <= '7' ) || *s == '_' )
                s++ ;
            else {
                end = s ;
                ptok->type = tOCT ;
                state = lsCopy ;
            }
            break ;

        case lsDec :
            if ( isdigit( *s ) || *s == '_' )
                s++ ;
            else {
                end = s ;
                ptok->type = tDEC ;
                state = lsCopy ;
            }
            break ;

        case lsParam :
            if ( isdigit( *s ) )
                s++ ;
            else {
                end = s ;
                ptok->type = tAT ;
                state = lsCopy ;
            }
            break ;

        case lsOperator :
            ptok->type = tOPERATOR ;
            switch ( *start ) {
            case '*' :
                ptok->subtype = stMUL ;
                break ;
            case '/' :
                ptok->subtype = stDIV ;
                break ;
            case '%' :
                ptok->subtype = stMOD ;
                break ;
            case '+' :
                ptok->subtype = stADD ;
                break ;
            case '-' :
                ptok->subtype = stSUB ;
                break ;
            case '|' :
                ptok->subtype = stIOR ;
                break ;
            case '&' :
                ptok->subtype = stAND ;
                break ;
            case '^' :
                ptok->subtype = stXOR ;
                break ;
            case '~' :
                ptok->subtype = stTILDA ;
                break ;
            default :
                ptok->subtype = stNONE ;
            }
            end = s ;
            state = lsCopy ;
            break ;

        case lsDoubleOp :
            if ( *start == *s ) { // << or >>
                ptok->type = tOPERATOR ;
                switch ( *start ) {
                case '<' :
                    ptok->subtype = stSHL ;
                    break ;
                case '>' :
                    ptok->subtype = stSHR ;
                    break ;
                default :
                    ptok->subtype = stNONE ;
                }
                end = ++s ;
                state = lsCopy ;
            } else
                state = lsError ;
            break ;

        case lsPunct :
            end = s ;
            state = lsCopy ;
            switch ( *start ) {
            case ':' :
                ptok->type = tCOLON ;
                break ;
            case ',' :
                ptok->type = tCOMMA ;
                break ;
            case '(' :
                ptok->type = tLPAREN ;
                break ;
            case ')' :
                ptok->type = tRPAREN ;
                break ;
            default :
                state = lsError ;
            }
            break ;

        case lsIndex :
            if ( isalnum( *s ) )
                s++ ;
            else {
                end = s ;
                ptok->type = tIDENT ;
                ptok->subtype = stDOT ;
                state = lsCopy ;
            }
            break ;

            // final token collector
        case lsCopy :
            pterm = term ;
            while ( start < end )
                *pterm++ = *start++ ;
            *pterm = '\0' ;
            ptok->text = strdup( term ) ;
            ptok++ ;
            state = lsInit ;
            break ;

        // any errors
        case lsError :
            *pterm = '\0' ;
        default :
            ptok->type = tERROR ;
            return false ;
        }
    }
    return false ;
}
