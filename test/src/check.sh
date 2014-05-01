#!/bin/bash

CPP="g++"
TESTSRC="main.cc"
TEMPBINARY="temp"
ASM="./tools/asm"
SIM="./tools/sim"
SDCC="../../bin/sdcc"

function test() {
    rm -f temp "${1/.c/.trl}"
    ${SDCC} -mpblaze "$1" -o "${1/.c/}"
    ${ASM} -l -6 "${1/.c/.psm}" 2>"${1/.c/.err}"
    ${SIM} "${1/.c/.psm}" > "${1/.c/.trl}" 2>>"${1/.c/.err}"
    ${CPP} -DTESTFILE="\"${1/.c/.tst}\"" -DTESTRESULT="\"${1/.c/.trl}\"" "${TESTSRC}" -o ${TEMPBINARY} 2>>${1/.c/.err}
    (./${TEMPBINARY} 2>/dev/null && echo "${1/.c/}" "pass") || echo "${1/.c/}" "fail"
    rm -f temp "${1/.c/.trl}" "${1/.c/.psm}" "${1/.c/.err}" 2>/dev/null
}

if [ -z $1 ]; then
    for i in `find .. -maxdepth 1 -type f -name '*.c'`; do
        test $i
    done
else
    test $1
fi
