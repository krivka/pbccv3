#!/bin/bash

function test() {
    rm -f temp ${1/.c/.trl}
    ./tools/asm -l -6 $1 2>${1/.c/.err}
    ./tools/sim ${1/.c/.psm} > ${1/.c/.trl} 2>>${1/.c/.err}
    gcc -DTESTFILE="\"${1/.c/.tst}\"" -DTESTRESULT="\"${1/.c/.trl}\"" main.c -o temp 2>>${1/.c/.err}
    (./temp 2>/dev/null && echo ${1/.c/} "pass") || echo ${1/.c/} "fail"
    rm -f temp ${1/.c/.trl} 2>/dev/null
}

if [ -z $1 ]; then
    for i in `find .. -maxdepth 1 -type f -name '*.c'`; do
        test $i
    done
else
    test $1
fi
