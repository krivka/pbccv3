#!/bin/sh
# Usage: ./$0 [SDCC_DIR]
#   Adds pblaze port into SDCC
#   Revision: 2013-08-15 (SDCC 3.3)
#   Authors: Martin Briza, Zbynek Krivka

# update by ZK (2013-08-16) - give relative directory for SDCC as $1 
#   or use $SDCC_HOME environment variable
#   or use implicit "sdcc" prefix (original behavior)
if [ $# -eq 1 ]; then
  SDCC_HOME=$1
elif [ -z $SDCC_HOME ]; then
   SDCC_HOME=sdcc
fi

echo Using SDCC path prefix: $SDCC_HOME 

# there should be a block of macros that looks like this:
#       #define TARGET_ID_MCS51    1
#       #define TARGET_ID_GBZ80    2
#       ...
glue_port_h() {
    awk -- '
    BEGIN{
        highest = 1;
        state = 0;
    }
    {
        if (/#define[ \t]+TARGET_ID_[A-Z0-9a-z_]+[ \t].+/) {
            print;
            if (state == 0) {
                state = 1;
            }
            else if (state == 1) {
                if ($3 > highest) {
                    highest = $3;
                }
                #if (/#define[ \t]+TARGET_ID_PBLAZE[ \t].+/) {
                #  exit 3;    #PBLAZE macro already defined
                #}
            }
            else {
                exit 1;
            }
        }
        else {
            if (state == 1) {
                state = 2;
                print "#define TARGET_ID_PBLAZE\t" highest + 1;
            }
            if (/#endif [/][*] PORT_INCLUDE [*][/]/) {
                if (state != 2) {
                    exit 2;
                }
                print "#if !OPT_DISABLE_PBLAZE"
                print "extern PORT pblaze_port;"
                print "#endif"
                print ""
            }
            print;
        }
        # update by ZK (2013-08-16) - macro to check new target port
        if (/#define[ \t]+TARGET_IS_STM8[ \t]+.port->id == TARGET_ID_STM8.+/) {
            print "#define TARGET_IS_PBLAZE\t(port->id == TARGET_ID_PBLAZE)"
        }        
    }' $SDCC_HOME/src/port.h
    if [ $? -eq 1 ]; then
        echo "All the TARGET define macros have to be at one place in src/port.h" >&2
        exit 1
    elif [ $? -eq 2 ]; then
        echo "There wasn't any TARGET define macro before the end of the include block" >&2
        exit 1
    elif [ $? -eq 3 ]; then
        echo "PBLAZE TARGET macro already defined in src/port.h" >&2
        exit 1
    fi
}

# looking for a structure that looks like this:
#       static PORT *_ports[] = {
#       #if !OPT_DISABLE_MCS51
#         &mcs51_port,
#       #endif
#       ...
# will add our structure to the end, just in case, even though it shouldn't matter
glue_sdccmain_c() {
    awk -- '
    BEGIN{
        state = 0;
    }
    {
        if (/[a-zA-Z_0-9]*[ \t]*PORT[ \t]*[*][ \t]*_ports[ \t]*[[][]][ \t]*=[ \t]*{[ \t]*/) {
            print
            if (state == 0) {
                state = 1;
            }
            else {
                exit 1;
            }
        }
        else {
            if (state == 1) {
                if (/[}][;]/) {
                    print "#if !OPT_DISABLE_PBLAZE"
                    print "  &pblaze_port,"
                    print "#endif"
                    state = 2;
                }
            }
            print
        }
    }' $SDCC_HOME/src/SDCCmain.c
    if [ $? -eq 1 ]; then
        echo "_ports structure in src/SDCCmain.c is defined twice" >&2
        exit 1
    fi
}

# orientation by STM8 port in code is no absolutelly stable, but better then comments (still used also)
glue_configure_in() {
    awk -- '
    BEGIN{
        state = 0;
    }
    {
        if (/^# Supported targets$/) {
            state = 1;
        }
        else if (state == 1 && /^[ \t]*$/) {
            print "AC_DO_PORT(pblaze, pblaze, PBLAZE, [Excludes the PBLAZE port])";
            state = 2;
        } else if (state == 2 && /# Generating output files/) {
            state = 3;
        } else if (state == 3 && /if test \$OPT_DISABLE_STM8 = 0\; then/) {
            state = 4;
        } else if (state == 4 && /^[ \t]*$/) {
            print ""
            print "if test $OPT_DISABLE_PBLAZE = 0; then"
            print "  AC_CONFIG_FILES([src/pblaze/Makefile])"
            print "fi"
            state = 5;
        } else if (state == 5 && /[ \t]*ENABLED Ports:[ \t]*/) {
            state = 6
        } else if (state == 6 && /^[ \t]*$/) {
            state = 7
            print "    pblaze              ${enable_pblaze_port}" 
        }
        print
    }' $SDCC_HOME/configure.in
}

echo Processing configuration files...
content_port_h="$(glue_port_h)"
content_sdccmain_c=$(glue_sdccmain_c)
content_configure_in=$(glue_configure_in)
echo Creating backups...
cp "$SDCC_HOME/src/port.h" "$SDCC_HOME/src/port.h.bak-glue"
cp "$SDCC_HOME/src/SDCCmain.c" $SDCC_HOME/src/SDCCmain.c.bak-glue
cp "$SDCC_HOME/configure.in" "$SDCC_HOME/configure.in.bak-glue"
echo Saving changes to configuration files... 
echo "$content_port_h" > "$SDCC_HOME/src/port.h"
echo "$content_sdccmain_c" > "$SDCC_HOME/src/SDCCmain.c"
echo "$content_configure_in" > "$SDCC_HOME/configure.in"