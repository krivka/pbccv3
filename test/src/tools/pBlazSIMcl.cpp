/*
 *  Copyright © 2013 : Maarten Brock <sourceforge.brock@dse.nl>
 *
 *  This file is part of pBlazSIMcl.
 *
 *  pBlazSIMcl is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  pBlazSIMcl is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with pBlazASM.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "pBlaze.h"
#include "version.h"

class UARTDevice : public IODevice
{
public:
    virtual void setValue ( uint32_t address, uint32_t value )
    {
        putchar ( value ) ;
    }
    virtual uint32_t getValue ( uint32_t address )
    {
        return 0 ;
    }
} ;

class MULTDevice : public IODevice
{
public:
    MULTDevice ( ) ;
    virtual void setValue ( uint32_t address, uint32_t value )
    {
        int idx = ( address & 0x02 ) >> 1 ;
        if ( address & 0x01 )
        {
            values[ idx ] &= 0xFF00 ;
            values[ idx ] |= ( value & 0xFF ) << 8;
        }
        else
        {
            values[ idx ] = ( value & 0xFF ) << 0 ;
        }
    }
    virtual uint32_t getValue ( uint32_t address )
    {
        return ( ( values[ 0 ] * values[ 1 ] ) >> ( address & 0x03 ) ) & 0xFF ;
    }
private:
    uint32_t values[ 2 ] ;
} ;

MULTDevice::MULTDevice()
{
    values[ 0 ] = 0 ;
    values[ 1 ] = 0 ;
}

static void usage( const char * text )
{
	printf("\n%s - Picoblaze command line Simulator V%ld.%ld.%ld (%s)\n", text, MAJOR, MINOR, BUILDS_COUNT, STATUS);
	printf("  (c) 2003-2014 Henk van Kampen\n");
    printf(" %s <filename[.lst]>\n", text);
    printf("  Reads filename.lst and if present filename.scr and starts executing.\n");
    printf("  Execution stops on the first breakpoint. Use BREAK opcode to set it.\n");
    printf(" Peripherals:\n");
    printf("  0xE8-0xEB: 16x16 to 32 bit multiplier (little endian)\n");
    printf("  0xEC-0xED: output to console (UART)\n");
}

bool isHexN(const char * Line, int nChars)
{
    int i;
    for (i=0; i<nChars; i++)
    {
        if (!isxdigit(Line[i]))
            return false;
    }
    return (isspace(Line[i]) != 0);
}

int main(int argc, char *argv[])
{
    Picoblaze pBlaze;     // our simulated core
    FILE * file;

    if (argc != 2)
    {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    char * filename = (char *)malloc(strlen(argv[1]) + 8);
    strcpy(filename, argv[1]);
    char * dot = strrchr(filename, '.');
    if (!dot || strcmp(dot, ".lst") != 0)
        strcat(filename, ".lst");
    dot = strrchr(filename, '.');
    file = fopen(filename, "rt");
    if (!file)
    {
        fprintf(stderr, "Could not open %s\n", filename);
        return EXIT_FAILURE;
    }

    int row = 1;
    size_t linesize = 256;
    char * line = (char *)malloc(linesize);
    while (!feof(file))
    {
        long pos = ftell(file);
        fgets(line, linesize, file);
        while (strlen(line) > linesize - 2)
        {
            linesize *= 2;
            line = (char *)realloc(line, linesize);
            fseek(file, pos, SEEK_SET);
            fgets(line, linesize, file);
        }

        int address, code;
        if (strncmp(line, "PB3", 3) == 0)
            pBlaze.setCore(true);
        else if (strncmp(line, "PB6", 3) == 0)
            pBlaze.setCore(false);
        else if (isHexN(line, 3) && isHexN(&line[4], 5))
        {
            if (sscanf(line, "%3x %5x", &address, &code) == 2)
                pBlaze.setCodeItem(address, code, row++, NULL);
        }
    }
    fclose(file);

    dot[1] = 's';
    dot[2] = 'c';
    dot[3] = 'r';
    // read scratchpad data (.scr) associated with .lst
    pBlaze.clearScratchpad();
    int addr = -1;
    file = fopen(filename, "rt");
    if (file)
    {
        while (!feof(file))
        {
            fgets(line, linesize, file);
            if (line[0] == '@')
            {
                sscanf(line, "@%x", &addr);
                if (addr >= MAXMEM)
                {
                    fprintf(stderr, "Out of scratchpad memory\n");
                    return EXIT_FAILURE;
                }
                addr &= 0xFF;
            }
            else
            {
                int value;
                sscanf(line, "%x", &value);
                pBlaze.setScratchpadData(addr++, value);
            }
        }
        fclose(file);
    }

    pBlaze.initPB();
    pBlaze.setIODevice(NULL, 0xE8, 0xEB, new MULTDevice());
    pBlaze.setIODevice(NULL, 0xEC, 0xED, new UARTDevice());
    pBlaze.resetPB();
    while (pBlaze.step() && !pBlaze.onBreakpoint())
    {// run, forest, run
    }
    fprintf(stderr, "%d instructions read\n", row-1);
    if (pBlaze.onBreakpoint())
    {
        fprintf(stderr, "executed in %d clock ticks\n", pBlaze.CycleCounter);
        pBlaze.dump();
        return EXIT_SUCCESS;
    }
    fprintf(stderr, "failed after %d clock ticks\n", pBlaze.CycleCounter);
    pBlaze.dump();
    return EXIT_FAILURE;
}
