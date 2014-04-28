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

/**
 * pBlaze assembler
 * @author Henk van Kampen
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pbTypes.h"
#include "pbParser.h"
#include "pbLibgen.h"

#if defined TCC || defined _MSC_VER
#include "getopt.h"
#else
#include <unistd.h>
#endif

#include "version.h"

static void print_version( char * text ) {
	printf ( "\n%s - Picoblaze Assembler V%ld.%ld.%ld (%s) (c) 2003-2013 Henk van Kampen\n", text, MAJOR, MINOR, BUILDS_COUNT, STATUS ) ;
}


/**
 * usage prints usage text
 * @param text application name
 */
static void usage ( char * text ) {
    print_version( text ) ;

    printf ( "\nThis program comes with ABSOLUTELY NO WARRANTY.\n"  ) ;
    printf ( "This is free software, and you are welcome to redistribute it\n"  ) ;
    printf ( "under certain conditions. See <http://www.gnu.org/licenses/>\n"  ) ;

	printf ( "\nUsage:\n" ) ;
	printf ( "   pBlazASM -3|-6 [-k] [-v] [-f] [-c[<MEMfile>]] [-s[<MEMfile>]] [-l[<LSTfile>]] <input file> <input file> <input file> ...\n" ) ;
	printf ( "   where:\n" ) ;
	printf ( "         -3      select Picoblaze-3, mandatory\n"
	         "         -6      select Picoblaze-6, mandatory\n"
	         "         -c/-C   creates a code (.MEM) file (not in combo with -x/-X)\n"
	         "         -x/-X   creates a code (.HEX) file (not in combo with -c/-C)\n" ) ;
	printf ( "         -s/-S   creates a data (.SCR) file\n"
	         "         -l      creates a LST file\n"
	         "         -k      select KCPSM mode with limited expression handling (-3 only)\n"
	         "         -v      generates verbose reporting\n"
	         "         -f      with -l creates a list file without code to replace the source\n" ) ;
	printf ( "\nNote: All (max 255) input files are assembled to one output." ) ;
	printf ( "\nNote: Option -k (KCPSM mode) is not supported in Picoblaze-6 mode." ) ;
	printf ( "\nNote: -C, -S and -X generate fully populated MEM files for 'Data2Mem'.\n" ) ;
}

/**
 * main entry
 * processes command line and
 * calls actual assembler()
 */
int main ( int argc, char ** argv ) {
	char * src_filenames[ 256 ] = { NULL } ;
	char code_filename[ 256 ] = { 0 } ;
	char data_filename[ 256 ] = { 0 } ;
	char list_filename[ 256 ] = { 0 } ;
	char * pfile, *ppath ;
	int result = 0 ;
	int nInputfile = 0 ;

	bool bKCPSM6 = true ;
	bool bMandatory = true ;
	bool bKCPSM_mode = false ; // KCPSM mode, accepts 'NAMEREG' etc
	bool bList_mode = true ;
	bool bOptErr = false ;
	bool bGlobals = false ;
	bool bWantMEM = false ;
	bool bWantHEX = false ;
	bool bWantSCR = false ;
	bool bWantZEROs = false ;
	bool bWantLST = false ;
	bool bVerbose = false ;

	extern char * optarg ;
	extern int optind, optopt ;
	int optch ;

	while ( ( optch = getopt ( argc, argv, "36c::C::fghkl::m::M::s::S::vx::X::" ) ) != -1 ) {
		switch ( optch ) {
		case '3' :
			bKCPSM6 = false ;
			if ( bKCPSM_mode ) {
				bOptErr = true ;
			}
			bMandatory = false ;
			break ;
		case '6' :
			bKCPSM6 = true ;
			bMandatory = false ;
			if ( bKCPSM_mode ) {
				bOptErr = true ;
			}
			break ;
		case 'f' :
			bList_mode = false ;
			break ;
		case 'g' :
			bGlobals = true ;
			break ;
		case 'h' :
			bOptErr = true ;
			break ;
		case 's' :
			bWantSCR = true ;
			if ( optarg != NULL ) {
				strcpy ( data_filename, optarg ) ;
			}
			break ;
		case 'S' :
			bWantSCR = true ;
			bWantZEROs = true ;
			if ( optarg != NULL ) {
				strcpy ( data_filename, optarg ) ;
			}
			break ;
		case 'x' :
			if ( bWantMEM ) {
				fprintf ( stderr, "? conflicting option -%c\n", optch ) ;
				bOptErr = true ;
			} else {
				bWantHEX = true ;
				if ( optarg != NULL ) {
					strcpy ( code_filename, optarg ) ;
				}
			}
			break ;
		case 'X' :
			if ( bWantMEM ) {
				fprintf ( stderr, "? conflicting option -%c\n", optch ) ;
				bOptErr = true ;
			} else {
				bWantHEX = true ;
				bWantZEROs = true ;
				if ( optarg != NULL ) {
					strcpy ( code_filename, optarg ) ;
				}
			}
		case 'c' :
		case 'm' :
			if ( bWantHEX ) {
				fprintf ( stderr, "? conflicting option -%c\n", optch ) ;
				bOptErr = true ;
			} else {
				bWantMEM = true ;
				if ( optarg != NULL ) {
					strcpy ( code_filename, optarg ) ;
				}
			}
			break ;
		case 'C' :
		case 'M' :
			if ( bWantHEX ) {
				fprintf ( stderr, "? conflicting option -%c\n", optch ) ;
				bOptErr = true ;
			} else {
				bWantMEM = true ;
				bWantZEROs = true ;
				if ( optarg != NULL ) {
					strcpy ( code_filename, optarg ) ;
				}
			}
			break ;
			break ;
		case 'l' :
			bWantLST = true ;
			if ( optarg != NULL ) {
				strcpy ( list_filename, optarg ) ;
			}
			break ;
		case 'k' :
			bKCPSM_mode = true ;
			if ( bKCPSM6 ) {
				bOptErr = true ;
			}
			break ;
		case 'v' :
			bVerbose = true ;
			break ;
		case ':' :
			fprintf ( stderr, "? missing option -%c\n", optopt ) ;
			bOptErr = true ;
			break ;
		default :
			fprintf ( stderr, "? unknown option -%c\n", optopt ) ;
			bOptErr = true ;
			break ;
		}
	}

	if ( bOptErr || bMandatory ) {
		usage ( basename ( argv[ 0 ] ) ) ;
		result = -1 ;
		goto finally ;
	}

	if ( bVerbose ) {
        print_version( basename ( argv[ 0 ] ) ) ;
		if ( bKCPSM6 ) {
			printf ( "! PB6 option chosen\n" ) ;
		} else {
			printf ( "! PB3 option chosen\n" ) ;
		}
	}

	if ( bVerbose && bKCPSM_mode ) {
		fprintf ( stdout, "! KCPSM compatible mode selected\n" ) ;
	}

	if ( bVerbose && bGlobals ) {
		fprintf ( stdout, "! Global label definition selected\n" ) ;
	}

	if ( argv[ optind ] == NULL ) {
		fprintf ( stderr, "? source file(s) missing\n" ) ;
		usage ( basename ( argv[ 0 ] ) ) ;
		result = -1 ;
		goto finally ;
	}

	for ( nInputfile = 0 ; argv[ optind ] != NULL && nInputfile < 256 ; nInputfile += 1, optind += 1 ) {
		src_filenames[ nInputfile ] = calloc ( strlen ( argv[ optind ] ) + 16, sizeof ( char ) ) ;
		strcpy ( src_filenames[ nInputfile ], argv[ optind ] ) ;

		if ( strrchr ( src_filenames[ nInputfile ], '.' ) == NULL ) {
			strcat ( src_filenames[ nInputfile ], ".psm" ) ;
		}
		if ( bVerbose )
			fprintf ( stdout, "! Sourcefile: %s\n", src_filenames[ nInputfile ] ) ;
	}

	if ( bWantMEM || bWantHEX ) {
		if ( strlen ( code_filename ) == 0 ) {
			pfile = filename ( src_filenames[ nInputfile - 1 ] ) ;
			ppath = dirname ( src_filenames[ nInputfile - 1 ] ) ;
			strcpy ( code_filename, ppath ) ;
#ifdef __MINGW32__
			strcat ( code_filename, "\\" ) ;
#else
			strcat ( code_filename, "/" ) ;
#endif
			strcat ( code_filename, pfile ) ;
			strcat ( code_filename, bWantHEX ? ".hex" : ".mem" ) ;
			free ( ppath ) ;
			free ( pfile ) ;
		}
		if ( strrchr ( code_filename, '.' ) == NULL )
			strcat ( code_filename, bWantHEX ? ".hex" : ".mem" ) ;
		if ( bVerbose )
			fprintf ( stdout, "! MEM file: %s\n", code_filename ) ;
	}

	if ( bWantSCR ) {
		if ( strlen ( data_filename ) == 0 ) {
			pfile = filename ( src_filenames[ nInputfile - 1 ] ) ;
			ppath = dirname ( src_filenames[ nInputfile - 1 ] ) ;
			strcpy ( data_filename, ppath ) ;
#ifdef __MINGW32__
			strcat ( data_filename, "\\" ) ;
#else
			strcat ( data_filename, "/" ) ;
#endif
			strcat ( data_filename, pfile ) ;
			strcat ( data_filename, ".scr" ) ;
			free ( ppath ) ;
			free ( pfile ) ;
		}
		if ( strrchr ( data_filename, '.' ) == NULL )
			strcat ( data_filename, ".scr" ) ;
		if ( bVerbose )
			fprintf ( stdout, "! SCR file: %s\n", data_filename ) ;
	}

	if ( bWantLST ) {
		if ( strlen ( list_filename ) == 0 ) {
			pfile = filename ( src_filenames[ nInputfile - 1 ] ) ;
			ppath = dirname ( src_filenames[ nInputfile - 1 ] ) ;
			strcpy ( list_filename, ppath ) ;
#ifdef __MINGW32__
			strcat ( list_filename, "\\" ) ;
#else
			strcat ( list_filename, "/" ) ;
#endif
			strcat ( list_filename, pfile ) ;
			strcat ( list_filename, ".lst" ) ;
			free ( ppath ) ;
			free ( pfile ) ;
		}
		if ( strrchr ( list_filename, '.' ) == NULL )
			strcat ( list_filename, ".lst" ) ;
		if ( bVerbose )
			fprintf ( stdout, "! LST file: %s\n", list_filename ) ;
	}

	if ( assembler ( src_filenames, code_filename, data_filename, list_filename, bKCPSM_mode, bKCPSM6, bList_mode, bWantHEX, bWantZEROs, bGlobals ) )
		result = 0 ;
	else
		result = -1 ;

finally: {
		int i ;

		for ( i = 0 ; i < nInputfile ; i += 1 )
			free ( src_filenames[ i ] ) ;
	}
	exit( result ) ;
}
