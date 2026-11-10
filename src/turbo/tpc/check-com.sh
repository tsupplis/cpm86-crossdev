#!/bin/sh
# Compile and execute a program exercising includes, heap, arithmetic and I/O.
set -eu
emu=$1
compiler=$2
extension=$3
fixtures=$4
work=$5
variant=$6
case "$variant" in
    dos200|dos302|dms301|c86301) ;;
    *) echo "Unknown compiler variant: $variant" >&2; exit 1 ;;
esac
mkdir -p "$work"
cd "$work"
rm -f "comtest.$extension" comtest.txt
awk '{ sub(/\r$/, ""); printf "%s\r\n", $0 }' "$fixtures/comtest.pas" > comtest.pas
awk '{ sub(/\r$/, ""); printf "%s\r\n", $0 }' "$fixtures/values.inc" > values.inc
printf '17\r\n\032' > input.txt
"$emu" "$compiler" -cc comtest.pas > compile.log 2>&1 || { cat compile.log; exit 1; }
if grep -Eq 'TPC: (compilation failed|invalid arguments)' compile.log; then
    cat compile.log
    exit 1
fi
test -s "comtest.$extension"
test ! -e comtest.txt
"$emu" "comtest.$extension" > runtime.log 2>&1 || { cat runtime.log; exit 1; }
if test "$(tr -d '\015\032\000' < comtest.txt)" != 'COM=11457;REAL=10'; then
    cat runtime.log comtest.txt
    exit 1
fi
echo "Turbo $extension execution: includes, procedures, heap, real math and file I/O passed"
case "$variant" in
    dos302|dms301)
        rm -f redirect.com redirect.txt
        awk '{ sub(/\r$/, ""); printf "%s\r\n", $0 }' "$fixtures/redirect.pas" > redirect.pas
        "$emu" "$compiler" -cc redirect.pas > compile-redirect.log 2>&1 || { cat compile-redirect.log; exit 1; }
        if grep -Eq '^TPC: (compilation failed|invalid arguments)' compile-redirect.log; then
            cat compile-redirect.log
            exit 1
        fi
        test -s redirect.com
        "$emu" redirect.com < input.txt > redirect.txt 2> redirect-runtime.log || { cat redirect-runtime.log; exit 1; }
        if test "$(tr -d '\015\032\000' < redirect.txt)" != 'REDIRECT=59'; then
            cat redirect-runtime.log redirect.txt
            exit 1
        fi
        echo "Turbo $variant: redirected standard input/output passed"
        ;;
esac
echo "Outputs: $work"
