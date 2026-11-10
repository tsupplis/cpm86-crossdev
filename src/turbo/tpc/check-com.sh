#!/bin/sh
# Compile and execute a program exercising includes, heap, arithmetic and I/O.
set -eu
emu=$1
compiler=$2
extension=$3
fixtures=$4
work=$5
mkdir -p "$work"
cd "$work"
rm -f "comtest.$extension" comtest.txt
awk '{ printf "%s\r\n", $0 }' "$fixtures/comtest.pas" > comtest.pas
awk '{ printf "%s\r\n", $0 }' "$fixtures/values.inc" > values.inc
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
echo "Outputs: $work"
