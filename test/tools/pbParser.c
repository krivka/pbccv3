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

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>

#include "pbTypes.h"
#include "pbSymbols.h"
#include "pbLexer.h"
#include "pbErrors.h"

#ifdef _MSC_VER   //Microsoft Visual C doesn't have strcasemcp, but has stricmp instead
#define strcasecmp  stricmp
#endif

#define CODESIZE 4096
#define DATASIZE (CODESIZE*2)

/**
 * pBlazASM work horse
 * @file pBParser.c
 */

/**
 * assemler states
 */
typedef enum _BUILD_STATE {
	bsINIT, bsLABEL, bsLABELED, bsSYMBOL, bsOPCODE, bsDIRECTIVE, bsOPERAND, bsEND, bsDIS
} build_state_e ;

// options
static bool bMode = false ; // KCPSM mode, accepts 'NAMEREG' etc
static bool bActive = true ; // conditional assembly mode
static bool bCode = true ; // list code

// source
static char * gSource ; // source filename for error printer
static int gLinenr = 0 ; // current line number
static int label_field = 20 ;
static int opcode_field = 6 ;
static int comment_column = 60 ;
static int last_column = 120 ;
static int indent = 4 ;

// generated code and data
static uint32_t gCode[ CODESIZE ] ; // code space and scratchpad space (in words)
static uint16_t gData[ DATASIZE ] ; // code space and scratchpad space (in words)

// code
static uint32_t gCodeRange = 1024 ; // selected code size
static uint32_t gPC = 0 ; // current code counter
static uint32_t oPC = 0 ;

// data
static uint32_t gSCR = 0 ;      // current scratchpad counter
static uint32_t gESG[ 256 ] ;   // current scratchpad counter backup
static int pESG = 0 ;           // index in backups
static int32_t gScrLoc = -1 ;   // place where scratchpad is in ROM
static uint32_t gScrSize = 0 ;  // size of scratchpad

/**
 * error strings
 */
static const char * s_errors[] = {
	"<none>", "unexpected tokens", "doubly defined", "undefined", "phasing error", "missing symbol",
	"syntax error", "syntax error in expression", "syntax error in operand", "syntax error in value", "value out of range",
	"syntax error in operator", "syntax error, register expected", "comma expected", "unexpected characters",
	"expression expected", "out of code space", "wrong scratchpad size", "out of scratchpad range", "<not-implemented>", "<internal error>", "syntax error in macro parameter"
} ;

static error_t expr ( uint32_t * result, symbol_t * (*current)(), symbol_t * (*next)(), int file_nbr ) ;
static error_t expression ( uint32_t * result, int file_nbr ) ;

/**
 * processOperator
 * handle 'expression' <operator> 'term'
 * @param result value to operate on
 * @param term value to operate with
 * @param oper ref to operator to be used
 * @return success
 */
static bool processOperator ( unsigned int * result, unsigned int term, symbol_t ** oper ) {
	if ( *oper == NULL ) {
		*result = term ;
		return true ;
	}
	switch ( ( *oper )->subtype ) {
	case stMUL :
		*result *= term ;
		break ;
	case stDIV :
        if ( term != 0 )
            *result /= term ;
        else if ( *result == 0 )
            *result = 1 ;               // by definition
		break ;
	case stMOD :
        if ( term > 0 )                 // only defined for positive term
            *result %= term ;
		break ;
	case stADD :
		*result += term ;
		break ;
	case stSUB :
		*result -= term ;
		break ;
	case stIOR :
		*result |= term ;
		break ;
	case stXOR :
		*result ^= term ;
		break ;
	case stAND :
		*result &= term ;
		break ;
	case stSHL :
		*result <<= term ;
		break ;
	case stSHR :
		*result >>= term ;
		break ;
	default :
		return false ;
	}
	*oper = NULL ;
	return true ;
}

/**
 * convert escaped char's
 * @param p ref to character sequence
 * @return converted character
 */
static int convert_char ( char * * pr, char * * ps ) {
	char * s = *ps ;
	char * r = *pr ;
	unsigned int v ;

	if ( *s == '\\' ) { // '\r' or '\013'
		s++ ;
		switch ( *s++ ) { // \a \b \f \n \r \t \v
		case '\'' :
			*r++ = '\'' ;
			break ;
		case '\\' :
			*r++ = '\\' ;
			break ;
		case '"' :
			*r++ = '"' ;
			break ;
		case 'a' :
		case 'A' :
			*r++ = '\a' ;
			break ;
		case 'b' :
		case 'B' :
			*r++ = '\b' ;
			break ;
		case 'f' :
		case 'F' :
			*r++ = '\f' ;
			break ;
		case 'n' :
		case 'N' :
			*r++ = '\n' ;
			break ;
		case 'r' :
		case 'R' :
			*r++ = '\r' ;
			break ;
		case 't' :
		case 'T' :
			*r++ = '\t' ;
			break ;
		case 'v' :
		case 'V' :
			*r++ = '\v' ;
			break ;
		case 'x' :
		case 'X' :
			if ( sscanf ( s, "%x", &v ) != 1 )
				return etLEX ;
			while ( isxdigit ( *s ) )
				s++ ;
            *r++ = v & 0xff ;
			break ;
		case '0' :
			--s ;
			if ( sscanf ( s, "%o", &v ) != 1 )
				return etLEX ;
			while ( isdigit ( *s ) && *s != '8' && *s != '9' )
				s++ ;
            *r++ = v & 0xff ;
			break ;
		default :
            *r++ = '\\' ;
            *r++ = *s++ ;
		}
	} else
		*r++ = *s++ ;
	*ps = s ;
	*pr = r ;
	return etNONE ;
}

/**
 * convert escaped char's in a string
 * @param s string to be converted
 * @return new string with result (needs to be freeed)
 */
static char * convert_string ( char * s ) {
	char * p = calloc ( 2 * strlen( s ) + 1, sizeof( char ) ) ;
	char *r ;

	for ( r = p ; *s != '\0' ; )
		if ( convert_char ( &r, &s ) != etNONE ) {
		    free( p ) ;
            return NULL ;
		}
	*r++ = '\0' ;
	return p ;
}


/**
 * macro string evaluator
 */

static symbol_t * eval_ptok = 0 ; // pointer to current token, index in 'tokens[]'

static symbol_t * eval_current( void ) {
//    printf( "%s\n", eval_ptok->text ) ;
    return eval_ptok ;
}

static symbol_t * eval_next( void ) {
    return eval_ptok++ ;
}

/**
 * macro string evaluator
 * @param symbol with string value
 * @result evaluated value
 */
static uint32_t eval( uint32_t * result, symbol_t * h, int file_nbr  ) {
    eval_ptok = h->value.tokens ;
	return expr ( result, eval_current, eval_next, file_nbr ) ;
}


/**
 * term processing
 * @param resulting value of term
 * @result error code
 */
static error_t term ( uint32_t * result, symbol_t * (*current)(), symbol_t * (*next)(), int file_nbr ) {
	symbol_t * oper = NULL ;
	const char * p = NULL ;
	symbol_t * h = NULL ;
	error_t e = etNONE ;
	char * s = NULL ;
	uint32_t val ;
	char * r = (char  *)&val ;

	// full expression handling
	if ( current()->type == tNONE )
		return etEXPR ;
	if ( current()->type == tOPERATOR )
		oper = next() ;
	s = current()->text ;
	switch ( current()->type ) {
	case tCHAR :
		if ( convert_char( &r, &s ) != etNONE )
            return etEXPR ;
		break ;
	case tBIN :
		// parse a binary value
		val = 0 ;
		for ( p = s ; *p != 0 ; p++ ) {
			if ( *p != '_' ) {
				val <<= 1 ;
				if ( *p == '1' )
					val |= 1 ;
			}
		}
		break ;
	case tOCT :
		// parse an octal value
		val = 0 ;
		for ( p = s ; *p != 0 ; p++ ) {
            switch ( *p ) {
            case '0' :
            case '1' :
            case '2' :
            case '3' :
            case '4' :
            case '5' :
            case '6' :
            case '7' :
                val = ( val << 3 ) + *p -'0' ;
                break ;
            case '_' :
                break ;
            default :
                return etEXPR ;
			}
		}
		break ;
	case tDEC :
		// parse a decimal value
		val = 0 ;
		for ( p = s ; *p != 0 ; p++ ) {
            switch ( *p ) {
            case '0' :
            case '1' :
            case '2' :
            case '3' :
            case '4' :
            case '5' :
            case '6' :
            case '7' :
            case '8' :
            case '9' :
                val = ( val * 10 ) + *p -'0' ;
                break ;
            case '_' :
                break ;
            default :
                return etEXPR ;
			}
		}
		break ;
	case tHEX :
		// parse a hexadecimal value
		val = 0 ;
		for ( p = s ; *p != 0 ; p++ ) {
            switch ( *p ) {
            case '0' :
            case '1' :
            case '2' :
            case '3' :
            case '4' :
            case '5' :
            case '6' :
            case '7' :
            case '8' :
            case '9' :
                val = ( val << 4 ) + *p -'0' ;
                break ;
            case 'A' :
            case 'B' :
            case 'C' :
            case 'D' :
            case 'E' :
            case 'F' :
                val = ( val << 4 ) + *p -'A' + 10 ;
                break ;
            case 'a' :
            case 'b' :
            case 'c' :
            case 'd' :
            case 'e' :
            case 'f' :
                val = ( val << 4 ) + *p -'a' + 10 ;
                break ;
            case '_' :
                break ;
            default :
                return etEXPR ;
            }
		}
		break ;
	case tIDENT :
		h = find_symbol ( s, false, file_nbr ) ;
		if ( h == NULL )
			return etUNDEF ;
		if ( h->type != tVALUE && h->type != tLABEL && h->type != tGLABEL )
			return etVALUE ;
		// label as a moving target
		if ( h->subtype == stDOT && strlen ( h->text ) == 1 )
			val = oPC ;
		else if ( h->subtype == stTOKENS ) {
		    next() ;
            e = eval( &val, h, file_nbr ) ;
            if ( e != etNONE )
                return e ;
		} else
			val = h->value.integer ;
		break ;
    case tAT :
        // evaluate expression in the calling line
        if ( tok_current()->type == tLPAREN ) {
            tok_next() ;
            e = expr( &val, tok_current, tok_next, file_nbr ) ;
            if ( e != etNONE )
                return e ;
            if ( tok_current()->type != tRPAREN )
                return etEXPR ;
        } else
            return etPARAM ;
        break ;
	case tLPAREN :
		next() ;
		e = expr ( &val, current, next, file_nbr ) ;
		if ( e != etNONE )
			return e ;
		if ( current()->type != tRPAREN )
			return etEXPR ;
		break ;
	default :
		return etEXPR ;
	}
    next() ;
	if ( oper != NULL ) {
		switch ( oper->subtype ) {
		case stSUB :
			*result = -val ;
			break ;
		case stTILDA :
			*result = ~val ;
			break ;
		default :
			return etOPERATOR ;
		}
	} else
		*result = val ;
	return etNONE ;
}

/**
 * expression processing
 * depending of current bMode
 * @param result resulting value of expression
 * @return error code
 */
static error_t expr ( uint32_t * result, symbol_t * (*current)(), symbol_t * (*next)(), int file_nbr ) {
	symbol_t * h = NULL ;
	char * s = NULL ;
	symbol_t * oper = NULL ;
	error_t e = etNONE ;
	uint32_t val ;
	*result = 0 ;
	// crippled expression handling
	while ( bMode && current()->type != tNONE ) {
		switch ( current()->type ) {
		case tLPAREN :
			next() ;
			e = expr( &val, current, next, file_nbr ) ;
			if ( e != etNONE )
				return e ;
			if ( ! processOperator ( result, val, &oper ) )
				return etOPERATOR ;
		case tRPAREN :
			next() ;
			return etNONE ;
		case tCOMMA :
			return etNONE ;
		case tIDENT :
			s = current()->text ;
			h = find_symbol ( s, false, file_nbr ) ;
			if ( h != NULL )
				val = h->value.integer ;
			else if ( sscanf ( s, "%X", &val ) != 1 )
				return etEXPR ;
			next() ;
			*result = val ;
			return etNONE ;
		default :
			return etEXPR ;
		}
	}
	if ( current()->type == tNONE )
		return etEMPTY ;
	// full expression handling
	while ( current()->type != tNONE ) {
		switch ( current()->type ) {
		case tLPAREN :
			next() ;
			e = expr( &val, current, next, file_nbr ) ;
			if ( e != etNONE )
				return e ;
			if ( current()->type == tRPAREN )
				next() ;
			else
				return etEXPR ;
			break ;
		case tOPERATOR :
		case tCHAR :
		case tBIN :
		case tOCT :
		case tDEC :
		case tHEX :
		case tIDENT :
        case tAT :
			e = term( &val, current, next, file_nbr ) ;
			if ( e != etNONE )
				return e ;
			break ;
		default :
			return etNONE ;
		}
		if ( oper != NULL ) {
			if ( ! processOperator( result, val, &oper ) )
				return etOPERATOR ;
			oper = NULL ;
		} else
			*result = val ;
		if ( current()->type == tOPERATOR )
			oper = next() ;
		else
			break ;
	}
	return etNONE ;
}

/**
 * expression processing
 * depending of current bMode
 * @param result resulting value of expression
 * @return error code
 */
static error_t expression ( uint32_t * result, int file_nbr  ) {
    return expr( result, tok_current, tok_next, file_nbr  ) ;
}

/**
 * destreg: process destination register
 * @param result value of destination register, shifted already in position
 * @return success
 */
static bool destreg ( uint32_t * result ) {
	symbol_t * h ;
	if ( result == NULL ) {
		return false ;
	}
	*result = 0 ;
	h = find_symbol ( tok_current()->text, false, -1 ) ;
	if ( h != NULL && h->type == tREGISTER ) {
		tok_next() ;
		*result = h->value.integer << 8 ;
		return true ;
	}
	return false ;
}

/**
 * srcreg: process source register, accepts parens
 * @param result value of source register, shifted already in position
 * @return success
 */
static bool srcreg ( uint32_t * result ) {
	symbol_t * h ;
	bool bpar = false ;
	symbol_t * back = tok_current() ;

	if ( result == NULL )
		return false ;

    // allow registers to be embedded in parens, for some obscure reason
	*result = 0 ;
	if ( tok_current()->type == tLPAREN ) {
		bpar = true ;
		tok_next() ;
	}

	h = find_symbol ( tok_current()->text, false, -1 ) ;
	if ( h == NULL || h->type != tREGISTER )
		goto finally ;

	*result = h->value.integer << 4 ;
	tok_next() ;

	if ( bpar ) {
		if ( tok_current()->type == tRPAREN ) {
			tok_next() ;
		} else
			goto finally ;
	}
	return true ;

    // probably some other constant expression
finally: {
        tok_back ( back ) ;
		return false ;
	}
}

/**
 * eat comma in token stream
 * @return success
 */
static bool comma ( void ) {
	if ( tok_current()->type == tCOMMA ) {
		tok_next() ;
		return true ;
	} else {
		return false ;
	}
}

/**
 * process condition token
 * @param result value of condition, already in position
 * @return success
 */
static bool condition ( uint32_t * result ) {
	symbol_t * h ;
	if ( result == NULL ) {
		return false ;
	}
	*result = 0 ;
	if ( tok_current()->type != tNONE ) {
		h = find_symbol ( tok_current()->text, true, -1 ) ;
		if ( h != NULL && h->type == tCONDITION ) {
			tok_next() ;
			*result = h->value.integer ;
			return true ;
		}
	}
	return false ;
}

/**
 * process enable token
 * @param result value of enable, already in position
 * @return success
 */
static bool enadis ( uint32_t * result ) {
	symbol_t * h ;
	if ( result == NULL ) {
		return false ;
	}
	*result = 0 ;
	h = find_symbol ( tok_current()->text, true, -1 ) ;
	if ( h != NULL && h->type == tOPCODE && h->subtype == stINTI ) {
		tok_next() ;
		*result = h->value.integer & 1 ;
		return true ;
	}
	return false ;
}

/**
 * process indexed token
 * @param result value of indexed construct, already in position
 * @return success
 */
static bool indexed ( uint32_t * result ) {
	symbol_t * h ;
	if ( result == NULL ) {
		return false ;
	}
	*result = 0 ;
	if ( tok_current()->type != tNONE ) {
		h = find_symbol ( tok_current()->text, true, -1 ) ;
		if ( h != NULL && h->type == tINDEX ) {
			tok_next() ;
			*result = h->value.integer ;
			return true ;
		}
	}
	return false ;
}


/**
 * first pass of assembler
 * process all source lines and build symbol table
 * @param core type
 * @return error code
 */
static error_t build ( int file_nbr ) {
	build_state_e state = bsINIT ;
	symbol_t * symtok = NULL ;
	symbol_t * h = NULL ;
	symbol_t * r = NULL ;
	uint32_t result = 0 ;
	error_t e = etNONE ;
	value_t value ;

	// process statement
	for ( tok_first(), state = bsINIT ; state != bsEND && state != bsDIS ; tok_next() ) {
		oPC = gPC ;
		switch ( tok_current()->type ) {
		case tNONE :
			// empty line?
			if ( state != bsINIT && state != bsLABELED ) {
				return etSYNTAX ;
			}
			state = bsEND ;
			break ;
        // opcode or symbol definition
		case tIDENT :
			// opcode or directive?
			h = find_symbol ( tok_current()->text, false, file_nbr ) ;
			if ( h == NULL ) {
				h = find_symbol ( tok_current()->text, true, file_nbr ) ;
				if ( h == NULL || h->type != tOPCODE )
					h = NULL ;
			}
			if ( h != NULL ) {
				switch ( h->type ) {
				case tLABEL :
					if ( state != bsINIT )
						return etSYNTAX ;
					if ( h->subtype != stDOT )
						return etSYNTAX ;
					h->value.integer = oPC ;    // change label address to current address
					symtok = tok_current() ;
					state = bsLABEL ;
					break ;
				case tOPCODE :
					if ( state != bsINIT && state != bsLABELED )
						return etSYNTAX ;
					if ( bActive )
						gPC += 1 ;
					state = bsEND ; // we know enough for now
					break ;
				case tDIRECTIVE :
					switch ( h->subtype ) {
					case stFI:
						bActive = true ;
						state = bsDIS ;
						break ;
					case stIF:
						if ( state != bsINIT ) {
							return etSYNTAX ;
						}
						tok_next() ;
						if ( ( e = expression ( &result, file_nbr ) ) == etNONE )
							bActive = result != 0 ;
						else
							return e ;
						break ;
					case stPAGE :
						state = bsEND ;
						break ;
                    // .ORG
					case stADDRESS :
					case stORG :
						if ( state != bsINIT )
							return etSYNTAX ;
						tok_next() ;
						if ( ( e = expression ( &result, file_nbr ) ) == etNONE ) {
							if ( result >= gCodeRange )   // within code range
								return etRANGE ;
							if ( bActive )
								gPC = result ;
						} else
							return e ;
						state = bsEND ;
						break ;
                    // .ESG
					case stESG :
                        if ( state != bsINIT && state != bsSYMBOL )
							return etSYNTAX ;
						tok_next() ;
                        if ( symtok != NULL ) {
                            value.integer = gSCR ;
                            if ( !add_symbol ( tVALUE, stINT, symtok->text, value, file_nbr ) )
                                return etDOUBLE ;
                        }
                        if ( bActive ) {
                            if ( pESG <= 0 )
								return etSCRRNG ;
                            gSCR = gESG[ --pESG ] ;
                        }
                        state = bsEND ;
                        break ;
                    // .DSG
					case stDSG :
                        if ( state != bsINIT )
							return etSYNTAX ;
						tok_next() ;
						if ( ( e = expression ( &result, file_nbr ) ) == etNONE ) {
							if ( result >= gScrSize ) // within data range
								return etSCRRNG ;
							if ( bActive ) {
                                if ( pESG >= 255 )
                                    return etSCRRNG ;
								gESG[ pESG++ ] = gSCR ; // backup
								gSCR = result ;
							}
						} else
							return e ;
                        break ;

					case stALIGN :
						if ( state != bsINIT )
							return etSYNTAX ;
						tok_next() ;
						if ( ( e = expression ( &result, file_nbr ) ) == etNONE ) {
							if ( result <= 0 )
								return etRANGE ;
							if ( bActive ) {
								gPC = ( gPC + ( result - 1 ) ) / result * result ;
								if ( gPC >= gCodeRange )      // within code range
									return etRANGE ;
							}
						} else
							return e ;
						state = bsEND ;
						break ;
                    // .END
					case stEND :
						if ( state != bsINIT )
							return etSYNTAX ;
						tok_next() ;
						if ( ( e = expression ( &result, file_nbr ) ) == etNONE ) {
							if ( result >= CODESIZE * 2 )     // within possible code range
								return etRANGE ;
							if ( bActive )
								gCodeRange = result + 1 ;
						} else
							return e ;
						state = bsEND ;
						break ;
                    // .SCR
					case stSCRATCHPAD :
						if ( state != bsINIT )
							return etSYNTAX ;
						tok_next() ;
						if ( ( e = expression ( &result, file_nbr ) ) == etNONE ) {
							if ( bActive ) {
								if ( result >= gCodeRange )    // within possible code range
									return etRANGE ;
								gScrLoc = result * 2  ;
							}
						} else
							return e ;
						if ( comma() ) {
							if ( ( e = expression ( &result, file_nbr ) ) == etNONE ) {
								if ( bActive ) {
									if ( result > 2 * gCodeRange )
										return etSCRSIZE ;
									gScrSize = result ;
									if ( gSCR > gScrSize )  // after the fact
										return etSCRRNG ;
								}
							} else
								return e ;
						} else
							gScrSize = 256 ;
						state = bsEND ;
						break ;

                    case stDEF :
						if ( state != bsSYMBOL )
							return etSYNTAX ;
						tok_next() ;
						if ( symtok != NULL ) {
                            symbol_t * tok = tok_current() ;
                            int c = 2 ;
                            int i ;

                            while ( tok_current()->type != tNONE ) {
                                tok_next() ;
                                c += 1 ;
                            }
                            value.tokens = (symbol_t *) malloc( c * sizeof( symbol_t ) ) ;
                            for ( i = 0 ; i < c ; i += 1 ) {
                                value.tokens[ i ] = tok[ i ] ;
                                value.tokens[ i ].text = strdup( tok[ i ].text ) ;
                            }

                            if ( !add_symbol ( tVALUE, stTOKENS, symtok->text, value, file_nbr ) )
                                return etDOUBLE ;
						} else
							return etMISSING ;
						state = bsEND ;
						break ;

                    // .EQU
					case stEQU :
						if ( state != bsSYMBOL )
							return etSYNTAX ;
						tok_next() ;
						if ( symtok != NULL ) {
							if ( tok_current()->type == tSTRING ) {
								// string value?
								value.string = strdup ( tok_current()->text ) ;
								if ( !add_symbol ( tVALUE, stSTRING, symtok->text, value, file_nbr ) )
									return etDOUBLE ;
							} else {
								r = find_symbol ( tok_current()->text, false, -1 ) ;
								if ( r != NULL && r->type == tREGISTER ) {
									// register clone?
									value = r->value ;
									if ( !add_symbol ( tREGISTER, stCLONE, symtok->text, value, file_nbr ) )
										return etDOUBLE ;
								} else if ( ( e = expression ( &result, file_nbr ) ) == etNONE ) {
									// normal expression?
									value.integer = result ;
                                    if ( h->subtype == stSET ) {
                                        if ( !add_symbol ( tVALUE, stSET, symtok->text, value, file_nbr ) )
                                            return etSYNTAX ;
                                    } else {
                                        if ( !add_symbol ( tVALUE, stINT, symtok->text, value, file_nbr ) )
                                            return etDOUBLE ;
									}
								} else
									return e ;
							}
						} else
							return etMISSING ;
						state = bsEND ;
						break ;
                    // .SET
					case stSET :
						if ( state != bsSYMBOL )
							return etSYNTAX ;
						tok_next() ;
						if ( symtok != NULL ) {
                            if ( ( e = expression ( &result, file_nbr ) ) == etNONE ) {
                                // normal expression?
                                value.integer = result ;
                                if ( !add_symbol ( tVALUE, stSET, symtok->text, value, file_nbr ) )
                                    return etSYNTAX ;
                            } else
                                return e ;
						} else
							return etMISSING ;
						state = bsEND ;
						break ;
					case stCONSTANT :
						if ( state != bsINIT ) {
							return etSYNTAX ;
						}
						tok_next() ;
						symtok = tok_next() ;
						if ( symtok->type != tIDENT )
							return etSYNTAX ;
						if ( !comma() )
							return etCOMMA ;
						// normal expression?
						if ( ( e = expression ( &result, file_nbr ) ) == etNONE ) {
							value.integer = result ;
							if ( !add_symbol ( tVALUE, stINT, symtok->text, value, file_nbr ) )
								return etDOUBLE ;
						} else
							return e ;
						state = bsEND ;
						break ;
					case stNAMEREG :
						if ( state != bsINIT ) {
							return etSYNTAX ;
						}
						tok_next() ;
						symtok = tok_next() ;
						if ( symtok->type != tIDENT ) {
							return etSYNTAX ;
						}
						r = find_symbol ( symtok->text, true, -1 ) ;
						if ( r == NULL || r->type != tREGISTER ) {
							return etREGISTER ;
						}
						if ( !comma() )
							return etCOMMA ;
						if ( tok_current()->type == tIDENT ) {
							value = r->value ;
							if ( !add_symbol ( tREGISTER, stCLONE, tok_current()->text, value, file_nbr ) ) {
								return etDOUBLE ;
							}
						} else
							return etSYNTAX ;
						state = bsEND ;
						break ;
                    // DS, pBlazIDE support
					case stDS :
					case stDSIN :
					case stDSOUT :
					case stDSIO :
					case stDSRAM :
					case stDSROM :
						if ( state != bsSYMBOL ) {
							return etSYNTAX ;
						}
						tok_next() ;
						if ( symtok != NULL ) {
							if ( ( e = expression ( &result, file_nbr ) ) == etNONE ) {
								if ( bActive ) {
									value.integer = result ;
									if ( !add_symbol ( tVALUE, stINT, symtok->text, value, file_nbr ) ) {
										return etDOUBLE ;
									}
								}
							} else
								return e ;
						} else
							return etMISSING ;
						state = bsEND ;
						break ;
						// .BYT etc
					case stBYTE :
					case stWORD_BE :
					case stWORD_LE :
					case stLONG_BE :
					case stLONG_LE :
						if ( state != bsINIT && state != bsSYMBOL ) {
							return etSYNTAX ;
						}
						tok_next() ;
						value.integer = gSCR ;
						if ( symtok != NULL && !add_symbol ( tVALUE, stINT, symtok->text, value, file_nbr ) ) {
							return etDOUBLE ;
						}
						do {
							if ( ( e = expression ( &result, file_nbr ) ) != etNONE ) {
								if ( e == etEMPTY )
									break ;    // allow an empty expression list for generating a symbol only
								else
									return e ;
							}
							if ( bActive ) {
								switch ( h->subtype ) {
								case stLONG_BE :
								case stLONG_LE :
									gSCR += 4 ;
									break;
								case stWORD_BE :
								case stWORD_LE :
									gSCR += 2 ;
									break;
								default :
									gSCR += 1 ;
								}
							}
							if ( gSCR > gScrSize )
								return etSCRRNG ;
						} while ( comma() ) ;
						state = bsEND ;
						break ;
						// .BUF
					case stBUFFER :
						if ( state != bsINIT && state != bsSYMBOL ) {
							return etSYNTAX ;
						}
						tok_next() ;
						value.integer = gSCR & 0xFF ;
						if ( symtok && !add_symbol ( tVALUE, stINT, symtok->text, value, file_nbr ) ) {
							return etDOUBLE ;
						}
						if ( ( e = expression ( &result, file_nbr ) ) == etNONE ) {
							if ( bActive ) {
								gSCR += result ;
								if ( gSCR > gScrSize ) {
									return etSCRRNG ;
								}
							}
						} else
							return e ;
						state = bsEND ;
						break ;
						// .TXT
					case stTEXT :
						if ( state != bsINIT && state != bsSYMBOL )
							return etSYNTAX ;
						tok_next() ;
						value.integer = gSCR & 0xFF ;
						if ( symtok && !add_symbol ( tVALUE, stINT, symtok->text, value, file_nbr ) )
							return etDOUBLE ;
						do {
							if ( tok_current()->type == tSTRING ) {
								char * dup = convert_string ( tok_current()->text ) ;
								if ( bActive )
									gSCR += strlen ( dup ) + 1 ;
								free ( dup ) ;
							} else if ( tok_current()->type == tIDENT ) {
								char * dup = NULL ;
								h = find_symbol ( tok_current()->text, false, -1 ) ;
								if ( h == NULL || h->type != tVALUE || h->subtype != stSTRING )
									return etSYNTAX ;
								dup = convert_string ( h->value.string ) ;
								if ( bActive )
									gSCR += strlen ( dup ) + 1 ;
								free ( dup ) ;
							} else
								return etEXPR ;
							if ( gSCR > gScrSize )
								return etSCRRNG ;
							tok_next() ;
						} while ( comma() ) ;
						state = bsEND ;
						break ;
					case stEXEC :
					case stVHDL :
					case stHEX :
					case stMEM :
					case stCOE :
						if ( state != bsINIT )
							return etSYNTAX ;
						state = bsEND ;
						break ;
					default :
						return etSYNTAX ;
					}
					break ;
				default :
                    if ( h->subtype != stSET )
                        return etDOUBLE ;
                    else {
                        symtok = tok_current() ;
                        state = bsSYMBOL ;
                    }
				}
			} else if ( state == bsINIT ) {
				// not known yet, label or symbol definition?
				symtok = tok_current() ;
				state = bsSYMBOL ;
			} else {
				// maybe opcode mnemonic in lower/mixed case?
				h = find_symbol ( tok_current()->text, true, -1 ) ;
				if ( h != NULL && h->type == tOPCODE ) {
					if ( state != bsINIT && state != bsLABELED ) {
						return etSYNTAX ;
					}
					gPC += 1 ;
					state = bsEND ; // we know enough for now
				}
			}
			break ;
		case tDCOLON :
			value.integer = oPC ;
			if ( state == bsLABEL ) {
			} else if ( state != bsSYMBOL )
				return etSYNTAX ;
			else {
				if ( ! add_symbol ( tGLABEL, symtok->subtype, symtok->text, value, file_nbr ) )
					return etDOUBLE ;
			}
			state = bsLABELED ;
			break ;
		case tCOLON :
			value.integer = oPC ;
			if ( state == bsLABEL ) {
			} else if ( state != bsSYMBOL )
				return etSYNTAX ;
			else {
				if ( ! add_symbol ( tLABEL, symtok->subtype, symtok->text, value, file_nbr ) )
					return etDOUBLE ;
			}
			state = bsLABELED ;
			break ;
		default :
			return etSYNTAX ;
		}
	}
	return etNONE ;
}

/**
 * second pass of assembler
 * process all source lines and build code and scratchpad contents
 * @param addr ref to address value for listing
 * @param code ref to code value for listing
 * @param data ref to data value for listing
 * @param core type
 * @return error code
 */
static error_t assemble ( int file_nbr, uint32_t * addr, uint32_t * code, uint32_t * data, bool b6 ) {
	build_state_e state = bsINIT ;
	symbol_t * h = NULL ;
	symbol_t * symtok = NULL ;
	uint32_t result = 0 ;
	uint32_t operand1 = 0 ;
	uint32_t operand2 = 0 ;
	uint32_t opcode = 0 ;
	error_t e = etNONE ;
	bool bInParen = false ;

	*addr = 0xFFFFFFFF ;
	*code = 0xFFFFFFFF ;
	*data = 0xFFFFFFFF ;

	// process statement
	for ( tok_first(), state = bsINIT ; state != bsEND && state != bsDIS ; ) {
		oPC = gPC ;
		switch ( tok_current()->type ) {
		case tNONE :
			// empty line?
			if ( state != bsINIT && state != bsLABELED )
				return etSYNTAX ;
			state = bsEND ;
			break ;
		case tIDENT :
			h = find_symbol ( tok_current()->text, false, file_nbr ) ;
			// opcode mnemonic in lower/mixed case?
			if ( h == NULL ) {
				h = find_symbol ( tok_current()->text, true, -1 ) ;
				if ( h == NULL || h->type != tOPCODE ) {
					h = NULL ;
				}
			}
			if ( h != NULL ) {
				switch ( h->type ) {
                // opcodes
				case tOPCODE :
					if ( state != bsINIT && state != bsLABELED ) {
						return etSYNTAX ;
					}
					tok_next() ;
					opcode = 0xFFFFFFFF ;
					operand1 = 0 ;
					operand2 = 0 ;
					if ( bActive )
						gPC += 1 ;
					switch ( h->subtype ) {
					case stMOVE3 :
						if ( !destreg ( &operand1 ) )
							return etREGISTER ;
						if ( !comma() ) {
							return etCOMMA ;
						}
						if ( !srcreg ( &operand2 ) ) {
							if ( ( e = expression ( &operand2, file_nbr ) ) != etNONE )
								return e ;
							opcode = h->value.integer | operand1 | ( operand2 & 0xFF ) ;
						} else
							opcode = h->value.integer | operand1 | ( operand2 & 0xFF ) | 0x01000 ;
						break ;
					case stMOVE6 :
						if ( !destreg ( &operand1 ) )
							return etREGISTER ;
						if ( !comma() )
							return etCOMMA ;
						if ( !srcreg ( &operand2 ) ) {
							if ( ( e = expression ( &operand2, file_nbr ) ) != etNONE )
								return e ;
							opcode = h->value.integer | operand1 | ( operand2 & 0xFF ) | 0x01000 ;
						} else
							opcode = h->value.integer | operand1 | ( operand2 & 0xFF ) ;
						break ;
					case stCJMP3 :
						if ( condition ( &operand1 ) ) {
							if ( !comma() )
								return etCOMMA ;
						}
						if ( ( e = expression ( &operand2, file_nbr ) ) != etNONE )
							return e ;
						opcode = h->value.integer | operand1 | ( operand2 & 0x3FF ) ;
						break ;
					case stCJMP6 :
						if ( tok_current()->type == tLPAREN ) {
							tok_next() ;
							bInParen = true ;
						}
						if ( condition ( &operand1 ) ) {        // JUMP NZ, DONE
							if ( !comma() )
								return etCOMMA ;
							if ( ( e = expression ( &operand2, file_nbr ) ) != etNONE )
								return e ;
							if ( operand2 >= gCodeRange )
								return etRANGE ;
							opcode = h->value.integer | operand1 | ( operand2 ) ;
						} else if ( destreg ( &operand1 ) ) {   // JUMP s0, s1
							if ( !comma() )
								return etCOMMA ;
							if ( !srcreg ( &operand2 ) )
								return etREGISTER ;
							opcode = h->value.integer | operand1 | operand2 | 0x4000 ;
						} else if ( ( e = expression ( &operand2, file_nbr ) ) != etNONE ) { // JUMP DONE
							return e ;
						} else {
							if ( operand2 >= gCodeRange )
								return etRANGE ;
							opcode = h->value.integer | operand2 ;
						}
						if ( bInParen ) {
							if ( tok_current()->type == tRPAREN ) {
								tok_next() ;
							} else
								return etSYNTAX ;
						}
						bInParen = false ;
						break ;
					case stCSKP :
						operand1 = 0 ;
						operand2 = oPC + 2 ;
						if ( condition ( &operand1 ) ) {
							if ( comma() ) {
								if ( ( e = expression ( &operand2, file_nbr ) ) != etNONE )
									return e ;
								operand2 += oPC + 1 ;
							}
						} else if ( tok_current()->type != tNONE ) {
							if ( ( e = expression ( &operand2, file_nbr ) ) != etNONE )
								return e ;
							operand2 += oPC + 1 ;
						}
						opcode = h->value.integer | operand1 | operand2 ;
						break ;
					case stBREAK :
						opcode = h->value.integer + oPC + 1 ;
						break ;
					case stCRET3 :
						condition ( &operand1 ) ;
						opcode = h->value.integer | operand1 ;
						break ;
					case stCRET6 :
						// RET opcodes are not similar to JUMP/CALL
						if ( condition ( &operand1 ) ) {        // RET NZ
							opcode = h->value.integer | operand1 ;
						} else if ( destreg ( &operand1 ) ) {   // RET s0, $FF
							if ( !comma() )
								return etCOMMA ;
							if ( ( e = expression ( &operand2, file_nbr ) ) != etNONE )
								return e ;
							opcode =  h->value.integer | operand1 | ( operand2 & 0xFF ) ;
						} else
							opcode = 0x25000 ;    // RET
						break ;
					case stINT :
						opcode = h->value.integer ;
						break ;
					case stINTI :
						if ( !bMode )
							return etNOTIMPL ;
						opcode = h->value.integer ;
						if ( tok_current()->type != tIDENT || strcasecmp ( tok_current()->text, "INTERRUPT" ) != 0 )
							return etMISSING ;
						tok_next() ;
						break ;
					case stINTE :
						opcode = h->value.integer ;
						if ( enadis ( &operand1 ) )
							opcode = ( h->value.integer & 0xFFFFE ) | operand1 ;
						break ;
					case stIO3 :
						if ( !destreg ( &operand1 ) )
							return etREGISTER ;
						if ( !comma() )
							return etCOMMA ;
						if ( !srcreg ( &operand2 ) ) {
							if ( !indexed ( &operand2 ) ) {
								if ( ( e = expression ( &operand2, file_nbr ) ) != etNONE ) {
									return e ;
								}
								opcode = h->value.integer | operand1 | ( operand2 & 0xFF ) ;
							} else {
								opcode = h->value.integer | operand1 | operand2 ;
							}
						} else
							opcode = h->value.integer | operand1 | ( operand2 & 0xFF ) | 0x01000 ;
						break ;
					case stIO6 :
						if ( !destreg ( &operand1 ) )
							return etREGISTER ;
						if ( !comma() )
							return etCOMMA ;
						if ( !srcreg ( &operand2 ) ) {  // IN sX, kk
							if ( !indexed ( &operand2 ) ) {
								if ( ( e = expression ( &operand2, file_nbr ) ) != etNONE )
									return e ;
								opcode = h->value.integer | operand1 | ( operand2 & 0xFF ) | 0x01000 ;
							} else
								opcode = h->value.integer | operand1 | operand2 ;
						} else {                        // IN sX, sY
							opcode = h->value.integer | operand1 | ( operand2 & 0xFF ) ;
                            if ( comma() ) {            // IN sX, sY, k
								if ( ( e = expression ( &operand2, file_nbr ) ) != etNONE )
									return e ;
                                if ( operand2 < 15 )
                                    opcode |= operand2 ;
                                else
                                    return etOVERFLOW ;
                            }
						}
						break ;
					case stSHIFT :
						if ( !destreg ( &operand1 ) ) {
							return etREGISTER ;
						}
						opcode = h->value.integer | operand1 ;
						break ;
					case stINST :
						if ( ( e = expression ( &opcode, file_nbr ) ) != etNONE ) {
							return e ;
						}
						if ( opcode > 0x3FFFF ) {
							return etOVERFLOW ;
						}
						break ;
					case stBANK :
						if ( !b6 )
							return etSYNTAX ;
						if ( tok_current()->text != NULL ) {
							if ( strcmp ( tok_current()->text, "A" ) == 0 ) {
								tok_next() ;
								opcode = h->value.integer ;
							} else if ( strcmp ( tok_current()->text, "B" ) == 0 ) {
								tok_next() ;
								opcode = h->value.integer + 1 ;
							} else
								return etSYNTAX ;
						} else
							return etSYNTAX ;
						break ;
					case stOUTK :
						if ( !b6 )
							return etSYNTAX ;
						if ( ( e = expression ( &operand1, file_nbr ) ) != etNONE )
							return e ;
						if ( !comma() )
							return etCOMMA ;
						if ( ( e = expression ( &operand2, file_nbr ) ) != etNONE )
							return e ;
						if ( operand1 > 0xFF )
							return etOVERFLOW ;
						if ( operand2 > 0x0F )
							return etOVERFLOW ;
						opcode = h->value.integer | ( operand1 << 4 ) | operand2 ;
						break ;
					default :
						return etNOTIMPL ;
					}
					if ( opcode == 0xFFFFFFFF )
						return etINTERNAL ;
					if ( oPC < gCodeRange ) {
						if ( bActive ) {
							gCode[ oPC ] = opcode ;
							*addr = oPC ;
							*code = opcode ;
						}
					} else
						return etRANGE ;
					state = bsEND ;
					break ;
					// directives
				case tDIRECTIVE :
					tok_next() ;
					switch ( h->subtype ) {
					case stFI:
						bActive = true ;
						state = bsDIS ;
						break ;
					case stIF:
						if ( ( e = expression ( &result, file_nbr ) ) == etNONE ) {
							bActive = result != 0 ;
						} else
							return e ;
						*data = result ;
						state = bsEND ;
						break ;
					case stPAGE :
						state = bsEND ;
						break ;
					case stADDRESS :
						if ( !bMode )
							return etNOTIMPL ;
                    // fallthrough
					case stORG :
						if ( state != bsINIT )
							return etSYNTAX ;
						if ( ( e = expression ( &result, file_nbr ) ) == etNONE ) {
							*addr = result ;
							if ( result >= gCodeRange ) // within code range
								return etRANGE ;
							if ( bActive )
								gPC = result ;
						} else
							return e ;
						break ;
                    // .ESG
					case stESG :
                        if ( state != bsINIT && state != bsSYMBOL )
							return etSYNTAX ;
                        if ( bActive ) {
							if ( pESG <= 0 )
                                return etSCRRNG ;
							*data = gSCR ;
                            gSCR = gESG[ --pESG ] ;
							*addr = gSCR ;
                        }
                        break ;
                    case stDSG :
						if ( state != bsINIT )
							return etSYNTAX ;
						if ( ( e = expression ( &result, file_nbr ) ) == etNONE ) {
							*addr = result ;
							if ( result >= gScrSize )// within data range
								return etSCRRNG ;
							if ( bActive ) {
                                if ( pESG >= 255 )
                                    return etSCRRNG ;
                                gESG[ pESG++ ] = gSCR ; // backup
								gSCR = result ;
							}
						} else
							return e ;
                        break ;
					case stALIGN :
						if ( state != bsINIT ) {
							return etSYNTAX ;
						}
						if ( ( e = expression ( &result, file_nbr ) ) == etNONE ) {
							if ( result <= 0 ) {
								return etRANGE ;
							}
							if ( bActive ) {
								gPC = ( gPC + ( result - 1 ) ) / result * result ;
							}
							*addr = gPC ;
						} else {
							return e ;
						}
						if ( gPC >= gCodeRange ) { // within code range
							return etRANGE ;
						}
						state = bsEND ;
						break ;
					case stEND :
						if ( state != bsINIT )
							return etSYNTAX ;
						if ( ( e = expression ( &result, file_nbr ) ) == etNONE ) {
							if ( result >= CODESIZE ) {
								return etRANGE ;
							}
							if ( bActive ) {
								gCodeRange = result + 1 ;
								if ( gCodeRange == 64 )
									;
								else if ( gCodeRange == 256 )
									;
								else if ( gCodeRange == 1024 )
									;
								else if ( gCodeRange == 2048 )
									;
								else if ( gCodeRange == 4096 )
									;
								else {
									return etRANGE ;
								}
							}
							*addr = result ;
						} else {
							return e ;
						}
						break ;
					case stSCRATCHPAD :
						if ( state != bsINIT )
							return etSYNTAX ;
						if ( ( e = expression ( &result, file_nbr ) ) == etNONE ) {
							if ( bActive ) {
								gScrLoc = result * 2 ;
                                *addr = result ;
							}
						} else
							return e ;
						if ( comma() ) {
							if ( ( e = expression ( &result, file_nbr ) ) == etNONE ) {
								if ( bActive ) {
									if ( result > CODESIZE )
										return etSCRSIZE ;
									gScrSize = result ;
									if ( gSCR > gScrSize )
										return etSCRRNG ;
                                    *data = gScrSize ;
								}
							} else
								return e ;
						} else if ( bActive )
							gScrSize = 256 ;
						break ;
					case stDEF :
						if ( state != bsSYMBOL )
							return etSYNTAX ;
                        while ( tok_current() -> type != tNONE )
							tok_next() ;
                        *data = 0xFFFFFFFF ;
						break ;
					case stEQU :
						if ( state != bsSYMBOL )
							return etSYNTAX ;
						// NO-OP, just eat tokens in an orderly way
						e = etSYNTAX ;
						if ( tok_current()->type == tSTRING ) {
							tok_next() ;
							*data = 0xFFFFFFFF ;
						} else if ( destreg ( &result ) )
							*data = result ;
						else if ( ( e = expression ( &result, file_nbr ) ) == etNONE )
							*data = result ;
						else
							return e ;
						break ;
					case stSET :
						if ( state != bsSYMBOL )
							return etSYNTAX ;
						e = etSYNTAX ;
                        if ( ( e = expression ( &result, file_nbr ) ) == etNONE ) {
                            // normal expression?
                            if ( symtok != NULL && symtok->subtype == stSET ) {
                                symtok->value.integer = result ;
                                *data = result ;
                            } else
                                return etSYNTAX ;
                        } else
                            return e ;
						state = bsEND ;
						break ;
					case stDS :
					case stDSIN :
					case stDSOUT :
					case stDSIO :
					case stDSRAM :
					case stDSROM :
						if ( state != bsSYMBOL ) {
							return etSYNTAX ;
						}
						// NO-OP, just eat tokens in an orderly way
						do {
							if ( ( e = expression ( &result, file_nbr ) ) == etNONE ) {
							} else {
								return e ;
							}
						} while ( comma() ) ;
						break ;
					case stBYTE :
						if ( state != bsINIT && state != bsSYMBOL ) {
							return etSYNTAX ;
						}
						if ( bActive ) {
							*addr = gSCR ;
						}
						*data = 0xFFFFFFFF;
						do {
							if ( ( e = expression ( &result, file_nbr ) ) == etNONE ) {
								if ( result > 0xFF ) {
									return etOVERFLOW ;
								}
								if ( bActive ) {
									gData[ gSCR++ ] = result ;
								}
							} else if ( e == etEMPTY ) {
								// allow an empty expression list for generating a symbol only
								break ;
							} else {
								return e ;
							}
							if ( bActive ) {
								if ( *data == 0xFFFFFFFF ) {
									*data = result ;
								}
							}
							if ( gSCR > gScrSize ) {
								return etSCRRNG ;
							}
						} while ( comma() ) ;
						break ;
					case stWORD_BE :
					case stWORD_LE :
						if ( state != bsINIT && state != bsSYMBOL ) {
							return etSYNTAX ;
						}
						if ( bActive ) {
							*addr = gSCR ;
						}
						*data = 0xFFFFFFFF ;
						do {
							if ( ( e = expression ( &result, file_nbr ) ) == etNONE ) {
								if ( result > 0xFFFF ) {
									return etOVERFLOW ;
								}
								result &= 0xFFFF ;
								if ( h->subtype == stWORD_BE ) {
									if ( bActive ) {
										gData[ gSCR++ ] = ( result >> 8 ) & 0x00FF ;
										gData[ gSCR++ ] = ( result >> 0 ) & 0x00FF ;
										if ( *data == 0xFFFFFFFF ) {
											*data = ( result >> 8 ) & 0x00FF  ;
											*data |= ( result << 8 ) & 0xFF00  ;
										}
									}
								} else {
									if ( bActive ) {
										gData[ gSCR++ ] = ( result >> 0 ) & 0x00FF ;
										gData[ gSCR++ ] = ( result >> 8 ) & 0x00FF ;
										if ( *data == 0xFFFFFFFF ) {
											*data = ( result >> 0 ) & 0x00FF  ;
											*data |= ( result << 0 ) & 0xFF00  ;
										}
									}
									if ( gSCR > gScrSize ) {
										return etSCRRNG ;
									}
								}
							} else if ( e == etEMPTY ) {
								// allow an empty expression list for generating a symbol only
								break ;
							} else {
								return e ;
							}
						} while ( comma() ) ;
						break ;
					case stLONG_BE :
					case stLONG_LE :
						if ( state != bsINIT && state != bsSYMBOL ) {
							return etSYNTAX ;
						}
						if ( bActive ) {
							*addr = gSCR ;
						}
						*data = 0xFFFFFFFF;
						do {
							if ( ( e = expression ( &result, file_nbr ) ) == etNONE ) {
								if ( h->subtype == stLONG_BE ) {
									if ( bActive ) {
										gData[ gSCR++ ] = ( result >> 24 ) & 0x00FF ;
										gData[ gSCR++ ] = ( result >> 16 ) & 0x00FF ;
										gData[ gSCR++ ] = ( result >> 8 ) & 0x00FF ;
										gData[ gSCR++ ] = ( result >> 0 ) & 0x00FF ;
										if ( *data == 0xFFFFFFFF ) {
											*data = ( result >> 24 ) & 0xFF  ;
										}
									}
								} else {
									if ( bActive ) {
										gData[ gSCR++ ] = ( result >> 0 ) & 0x00FF ;
										gData[ gSCR++ ] = ( result >> 8 ) & 0x00FF ;
										gData[ gSCR++ ] = ( result >> 16 ) & 0x00FF ;
										gData[ gSCR++ ] = ( result >> 24 ) & 0x00FF ;
										if ( *data == 0xFFFFFFFF ) {
											*data = ( result >> 0 ) & 0xFF  ;
										}
									}
								}
							} else if ( e == etEMPTY ) {
								// allow an empty expression list for generating a symbol only
								break ;
							} else {
								return e ;
							}
							if ( *data == 0xFFFFFFFF ) {
								*data = result ;
							}
							if ( gSCR > gScrSize ) {
								return etSCRRNG ;
							}
						} while ( comma() ) ;
						break ;
						// .BUF
					case stBUFFER :
						if ( state != bsINIT && state != bsSYMBOL ) {
							return etSYNTAX ;
						}
						*data = 0xFFFFFFFF;
						if ( ( e = expression ( &result, file_nbr ) ) == etNONE ) {
							if ( bActive ) {
								*addr = gSCR ;
								*data = result ;
								while ( result-- )
									gData [ gSCR++ ] = 0 ;
								if ( gSCR > gScrSize ) {
									return etSCRRNG ;
								}
							}
						} else {
							return e ;
						}
						break ;
						// .TXT
					case stTEXT :
						if ( state != bsINIT && state != bsSYMBOL )
							return etSYNTAX ;
						if ( bActive )
							*addr = gSCR ;
						*data = 0xFFFFFFFF ;
						do {
							if ( tok_current()->type == tSTRING || tok_current()->type == tIDENT ) {
								char * dup ;
								if ( tok_current()->type == tIDENT ) {
									h = find_symbol ( tok_current()->text, false, -1 ) ;
									if ( h == NULL || h->type != tVALUE || h->subtype != stSTRING ) {
										return etSYNTAX ;
									}
									dup = convert_string ( h->value.string ) ;
								} else
									dup = convert_string ( tok_current()->text ) ;
								if ( bActive ) {
                                    int i = 0 ;
									if ( *data == 0xFFFFFFFF && strlen ( dup ) > 0 )
										*data = dup[ 0 ] ;
									for ( i = 0 ; i < (int)strlen ( dup ) + 1 ; i += 1 )
										gData[ gSCR++ ] = dup[ i ] & 0x00FF ;
								}
								free ( dup ) ;
								if ( gSCR > gScrSize )
									return etSCRRNG ;
							} else
								return etEXPR ;
							tok_next();
						} while ( comma() ) ;
						break ;
					case stEXEC :
					case stVHDL :
					case stHEX :
					case stMEM :
					case stCOE :
						if ( state != bsINIT ) {
							return etSYNTAX ;
						}
						return etNOTIMPL ;
						break ;
					case stCONSTANT :
						if ( state != bsINIT ) {
							return etSYNTAX ;
						}
						tok_next() ;
						if ( !comma() ) {
							return etCOMMA ;
						}
						// normal expression?
						if ( ( e = expression ( &result, file_nbr ) ) == etNONE ) {
//                           if ( bActive )
							*code = result ;
						} else {
							return e ;
						}
						break ;
					case stNAMEREG :
						if ( state != bsINIT ) {
							return etSYNTAX ;
						}
						if ( !bMode ) {
							return etNOTIMPL ;
						}
						tok_next() ;
						tok_next() ;
						tok_next() ;
						break ;
					default :
						return etSYNTAX ;
					}
					state = bsEND ;
					break ;
					// labels
				case tGLABEL :
				case tLABEL :
					if ( state != bsINIT ) {
						return etSYNTAX ;
					}
					if ( h->value.integer != (int)oPC && h->subtype != stDOT ) {
						return etPHASING ;
					}
					tok_next()->type = tLABEL ; // just for formatting
					h->value.integer = oPC ; // for dotted labels
					if ( bActive ) {
						*addr = h->value.integer ;
					}
					state = bsLABEL ;
					break ;
					// equated values
				case tVALUE :
				case tREGISTER :
					if ( state != bsINIT )
						return etSYNTAX ;
					symtok = h ;
					tok_next()->subtype = stEQU ; // just for formatting
					*data = h->value.integer & 0xFFFF ;
					state = bsSYMBOL ;
					break ;
				default :
					return etSYNTAX ;
				}
			} else if ( bActive ) {
				return etUNDEF ;
			} else {
				tok_next()->type = tLABEL ; // just for formatting
				tok_next()->subtype = stEQU ; // just for formatting
				state = bsDIS ;
			}
			break ;
		case tDCOLON :
		case tCOLON :
			// if we have a potential label, we need a ':'
			if ( state != bsLABEL )
				return etSYNTAX ;
			tok_next() ;
			state = bsLABELED ;
			break ;
		default :
			return etSYNTAX ;
		}
	}
    // only comment may follow
	if ( state != bsDIS && tok_current()->type != tNONE ) {
		return etEND ;
	}
	return etNONE ;
}

// error printer
static bool error ( const error_t e ) {
	if ( e != etNONE ) {
		fprintf ( stdout, "%s:%d: %s\n", gSource, gLinenr, s_errors[ e ] ) ;
		return false ;
	} else {
		return true ;
	}
}

// dump code in mem file format
static void dump_code ( FILE * f, bool hex, bool zeros ) {
	int h, l = 0 ;
	bool b_addr = true ;
	if ( hex ) {
		// find last used entry
		for ( h = 0 ; h < (int)gCodeRange && ! zeros ; h += 1 )
			if ( gCode[ h ] != 0xFFFC0000 ) {
				l = h ;
			}
		// list all
		for ( h = 0 ; h <= l ; h += 1 ) {
			fprintf ( f, "%05X\n", gCode[ h ] & 0x3FFFF ) ;
		}
	} else {
		// list used entries, prepend an origin
		for ( h = 0 ; h < (int)gCodeRange  ; h += 1 ) {
			if ( gCode[ h ] == 0xFFFC0000 && ! zeros ) {
				b_addr = true ;
			} else {
				if ( b_addr ) {
					fprintf ( f, "@%08X\n", h ) ;
					b_addr = false ;
				}
				fprintf ( f, "%05X\n", gCode[ h ] & 0x3FFFF ) ;
			}
		}
	}
}

// dump data in mem file format
static void dump_data ( FILE * f, bool hex ) {
	uint32_t h ;
	if ( ! hex )
        fprintf ( f, "@%08X\n", gScrLoc ) ;
	for ( h = 0 ; h < gScrSize ; h += 1 ) {
		fprintf ( f, "%02X\n", gData[ h ] & 0xFF ) ;
	}
}

// format list file
static void print_line ( FILE * f, error_t e, uint32_t addr, uint32_t code, uint32_t data ) {
	int n = 0 ;
	char * s = NULL ;

	if ( f == NULL )
		return ;

	tok_first() ;
	if ( e != etNONE ) {
		fprintf ( f, "?? %s:\n", s_errors[ e ] ) ;
	}
	if ( tok_current()->type == tDIRECTIVE && tok_current()->subtype == stPAGE ) {
		fprintf ( f, "\f" ) ;
		return ;
	}
	if ( bCode ) {                              // listing with binary info or just a formatted source?
		if ( data != 0xFFFFFFFF ) {
			if ( data > 0xFFFF ) {
				n += fprintf ( f, " %08X  ", data ) ;
//			} else if ( data > 0xFF ) {
//				n += fprintf ( f, "     %04X  ", data ) ;
			} else {
				if ( addr != 0xFFFFFFFF ) {
					if ( addr >= 0x100 ) {
						n += fprintf ( f, "%03X ", addr ) ;
					} else {
						n += fprintf ( f, " %02X ", addr ) ;
					}
				} else {
					n += fprintf ( f, "    " ) ;
				}
				if ( data > 0xFF ) {
					n += fprintf ( f, " %04X  ", data & 0xFFFF ) ;
				} else {
					n += fprintf ( f, "   %02X  ", data & 0xFF ) ;
				}
			}
		} else if ( code != 0xFFFFFFFF ) {
			if ( addr != 0xFFFFFFFF ) {
				n += fprintf ( f, "%03X ", addr ) ;
			} else {
				n += fprintf ( f, "   " ) ;
			}
			n += fprintf ( f, "%05X  ", code ) ;
		} else {
			if ( addr != 0xFFFFFFFF ) {
				// address info
				n += fprintf ( f, "%03X        ", addr ) ;
			} else {
				n += fprintf ( f, "           " ) ;
			}
		}
	}
	if ( tok_current()->type == tLABEL ) {
		// labels in the margin
		n += fprintf ( f, "%*s", -label_field, tok_next()->text ) ;
		n += fprintf ( f, "%s", tok_next()->text ) ;
	} else if ( tok_current()->subtype == stEQU ) {
		// print EQUates in the label margin
		n += fprintf ( f, "%*s", - ( label_field + 1 ), tok_next()->text ) ;
	} else if ( tok_current()->type != tNONE )
		// else print a blank margin
	{
		n += fprintf ( f, "%*s", label_field + 1, "" ) ;
	}
	// opcode
	if ( tok_current()->type != tNONE && tok_current()->text != NULL ) {
		for ( s = tok_current()->text ; s != NULL && isalpha ( *s ) ; s++ ) {
			*s = toupper ( *s ) ;
		}
		n += fprintf ( f, " %*s", -opcode_field, tok_next()->text ) ;
	}
	// operand
	for ( ; tok_current()->type != tNONE ; tok_next() ) {
		if ( tok_current()->text != NULL ) {
			if ( tok_current()->type != tCOMMA )
				n += fprintf ( f, " " ) ;
			switch ( tok_current()->type ) {
			case tHEX :
				n += fprintf ( f, "0x%s", tok_current()->text ) ;
				break ;
			case tOCT :
				n += fprintf ( f, "0o%s", tok_current()->text ) ;
				break ;
			case tBIN :
				n += fprintf ( f, "0b%s", tok_current()->text ) ;
				break ;
			case tCHAR :
				n += fprintf ( f, "'%s'", tok_current()->text ) ;
				break ;
			case tSTRING :
				n += fprintf ( f, "\"%s\"", tok_current()->text ) ;
				break ;
			default :
				n += fprintf ( f, "%s", tok_current()->text ) ;
				break ;
			}
		}
		if ( n > last_column )
            n = fprintf ( f, "\n%*s", 12 + label_field + 1 + opcode_field + indent, "" ) ;
	}
	// comment
	if ( tok_current()->type == tNONE && tok_current()->subtype == stCOMMENT ) {
		if ( tok_current()->text != NULL ) {
			if ( n < 12 )
				// at the start
			{
				fprintf ( f, "%s", tok_current()->text ) ;
			} else if ( n < comment_column ) {
				// at column 60
				fprintf ( f, "%*s", comment_column - n, "" ) ;
				fprintf ( f, "%s", tok_current()->text ) ;
			} else
				// after the rest
			{
				fprintf ( f, " %s", tok_current()->text ) ;
			}
		}
	}
	fprintf ( f, "\n" ) ;
}

static char * file_gets( char * s, int n, FILE * stream ) {
    char * l = s ;

    do {
        l = fgets( l, n, stream ) ;
        if ( l == NULL )
            return l ;
        gLinenr += 1 ;
        // maybe a '\' at the end? if so concatenate
        while ( *l++ != '\000' )
            ;
        l -= 3 ;
    } while ( *l == '\\' ) ;
    return s ;
}

// main entry for the 2-pass assembler
bool assembler ( char ** sourcefilenames, char * codefilename, char * datafilename, char * listfilename, bool mode, bool b6, bool listcode, bool hex, bool zeros, bool globals ) {
	FILE * fsrc = NULL ;
	FILE * fmem = NULL ;
	FILE * flist = NULL ;
	char ** Sources = NULL ;
	char line[ 4096 ] ;
	error_t e = etNONE ;
	uint32_t h = 0 ;
	bool result = true ;
	uint32_t addr, code, data ;
	int file_nbr ;

	// set up symbol table with keywords
	init_symbol ( b6 ) ;
	// clear code
	for ( h = 0 ; h < CODESIZE ; h += 1 )
		gCode[ h ] = 0xFFFC0000 ;
	// clear data
	for ( h = 0 ; h < DATASIZE ; h += 1 )
		gData[ h ] = 0xDE00 ;

	Sources = sourcefilenames ;
	gCodeRange = 1024 ;
	gPC = 0 ;
	gSCR = 0 ;
	gScrLoc = -1 ;
	gScrSize = 0x100 ;
	bMode = mode ;
	bActive = true ;
	for ( gSource = *Sources++, file_nbr = 1 ; gSource != NULL ; gSource = *Sources++, file_nbr += 1 ) {
		// open source file
		fsrc = fopen ( gSource, "r" ) ;
		if ( fsrc == NULL ) {
			fprintf ( stderr, "? unable to open source file '%s'\n", gSource ) ;
			result = false ;
			goto finally ;
		}
		// pass 1, add symbols from source
		for ( gLinenr = 0 ; file_gets ( line, sizeof ( line ), fsrc ) != NULL ; ) {
			if ( lex ( line, mode ) ) {
				result &= error ( build ( globals ? file_nbr : -1 ) ) ;
				tok_free() ;
			} else {
				result &= error ( etLEX ) ;
			}
		}
		fclose ( fsrc ) ;
	}
	// give up if errors in pass 1
	if ( !result )
		goto finally ;
	if ( strlen ( listfilename ) > 0 ) {
		flist = fopen ( listfilename, "w" ) ;
		if ( flist == NULL ) {
			fprintf ( stderr, "? unable to create LST file '%s'\n", listfilename ) ;
			result = false ;
		}
	}

	bCode = listcode ;
	Sources = sourcefilenames ;
	gCodeRange = 1024 ;
	gPC = 0 ;
	gSCR = 0 ;
	gScrLoc = -1 ;
	gScrSize = 0x100 ;
	bMode = mode ;
	bActive = true ;

	if ( bCode && flist != NULL ) {
		if ( b6 )
			fprintf ( flist, "PB6\n" ) ;
		else
			fprintf ( flist, "PB3\n" ) ;
	}
	for ( gSource = *Sources++, file_nbr = 1 ; gSource != NULL ; gSource = *Sources++, file_nbr += 1 ) {
		fsrc = fopen ( gSource, "r" ) ;
		if ( fsrc == NULL ) {
			fprintf ( stderr, "? unable to re-open source file '%s'\n", gSource ) ;
			result = false ;
			goto finally ;
		}
		if ( flist != NULL ) {
			fprintf ( flist, "---------- source file: %-75s\n", gSource ) ;
		}
		// pass 2, build code and scratchpad
		for ( gLinenr = 0 ; file_gets ( line, sizeof ( line ), fsrc ) != NULL ; ) {
			if ( lex ( line, mode ) ) {
				result &= error ( e = assemble ( globals ? file_nbr : -1, &addr, &code, &data, b6 ) ) ;
				print_line ( flist, e, addr, code, data ) ;
			} else {
				result &= error ( etLEX ) ;
				print_line ( flist, etLEX, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF ) ;
			}
			tok_free() ;
		}
		fclose ( fsrc ) ;
	}

	// dump scratch pad
	if ( strlen ( datafilename ) > 0 ) { // We want a separate data file?
		fmem = fopen ( datafilename, "w" ) ;
		if ( fmem == NULL ) {
			fprintf ( stderr, "? unable to create data MEM file '%s'\n", datafilename ) ;
			result = false ;
		} else {
			dump_data ( fmem, hex ) ;
			fclose ( fmem ) ;
		}
    // merge scratch pad in code
	} else if ( gScrLoc > -1 ) { // Merge data into code
		for ( h = 0 ; h < gScrSize ; h += 2 ) {
			if ( ( ( gData [ h ] & 0xFF00 ) != 0xDE00 ) || ( ( gData [ h + 1 ] & 0xFF00 ) != 0xDE00 ) ) {
                if ( gCode[ ( gScrLoc + h ) / 2 ] == 0xFFFC0000 ) { // Data not overlapping used code
                    gCode[ ( gScrLoc + h ) / 2 ] = ( gData[ h + 1 ] << 8 ) | gData[ h ];
                } else {
                    fprintf ( stderr, "? data and code overlap at: 0x%03x\n", ( gScrLoc + h ) / 2 ) ;
                }
            }
		}
	} else if ( gSCR > 0 )
		fprintf ( stderr, "? data section discarded, no .SCR given\n" ) ;

	// dump code (and optionally, scratch pad)
	if ( strlen ( codefilename ) > 0 ) { // We want a code file?
		fmem = fopen ( codefilename, "w" ) ;
		if ( fmem == NULL ) {
			fprintf ( stderr, "? unable to create code MEM file '%s'\n", codefilename ) ;
			result = false ;
		} else {
			dump_code ( fmem, hex, zeros ) ;
			fclose ( fmem ) ;
		}
	}
finally: {
		if ( flist != NULL )
			fclose ( flist ) ;
		free_symbol() ;
	}
	return result ;
}
