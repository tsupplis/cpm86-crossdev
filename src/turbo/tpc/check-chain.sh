#!/bin/sh
# CHN is not a standalone executable: enter it through a generated launcher.
set -eu
emu=$1
compiler=$2
extension=$3
fixtures=$4
work=$5
mkdir -p "$work"
cd "$work"
rm -f "launch.$extension" chain.chn before.txt chain.txt return.txt
awk '{ sub(/\r$/, ""); printf "%s\r\n", $0 }' "$fixtures/launch.pas" > launch.pas
awk '{ sub(/\r$/, ""); printf "%s\r\n", $0 }' "$fixtures/chain.pas" > chain.pas
awk '{ sub(/\r$/, ""); printf "%s\r\n", $0 }' "$fixtures/shared.inc" > shared.inc
"$emu" "$compiler" -ch chain.pas > compile-chain.log 2>&1 || { cat compile-chain.log; exit 1; }
if grep -Eq 'TPC: (compilation failed|invalid arguments)' compile-chain.log; then
    cat compile-chain.log
    exit 1
fi
test -s chain.chn
"$emu" "$compiler" -cc launch.pas > compile-launch.log 2>&1 || { cat compile-launch.log; exit 1; }
if grep -Eq 'TPC: (compilation failed|invalid arguments)' compile-launch.log; then
    cat compile-launch.log
    exit 1
fi
test -s "launch.$extension"
test ! -e before.txt
test ! -e chain.txt
"$emu" "launch.$extension" > runtime.log 2>&1 || { cat runtime.log; exit 1; }
if test "$(tr -d '\015\032\000' < before.txt)" != 'BEFORE=1234' ||
   test "$(tr -d '\015\032\000' < chain.txt)" != 'CHAIN=1276' ||
   test -e return.txt; then
    cat runtime.log
    echo "Chain must preserve shared data and not return to its launcher" >&2
    exit 1
fi
echo "Turbo $extension -> CHN execution: transfer and shared data passed"
echo "Outputs: $work"
