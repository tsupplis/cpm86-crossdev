#!/bin/sh
# Exercise the CP/M-86 compiler through its native BDOS interface.
set -eu
emu=$1
compiler=$2
original=$3
work=$4
mkdir -p "$work"
cd "$work"
rm -f hello.cmd hello.chn bad.cmd parent.cmd result.txt
printf "program hello;\r\nvar f: text;\r\nbegin assign(f,'result.txt'); rewrite(f);\r\nwriteln(f,'CPM runtime OK'); close(f) end.\r\n\032" > hello.pas
printf "program bad;\r\nbegin broken end.\r\n\032" > bad.pas
printf "program parent;\r\nvar f: file;\r\nbegin assign(f,'hello.chn'); chain(f) end.\r\n\032" > parent.pas

# These cover the complete CMD header, generated runtime/configuration and
# the initialized data group. Only the IDE portion of the code group changes.
test "$(wc -c < "$compiler")" -eq "$(wc -c < "$original")"
dd if="$original" of=one.bin bs=1 count=10240 2>/dev/null
dd if="$compiler" of=two.bin bs=1 count=10240 2>/dev/null
cmp one.bin two.bin
dd if="$original" of=one.bin bs=1 skip=37504 count=256 2>/dev/null
dd if="$compiler" of=two.bin bs=1 skip=37504 count=256 2>/dev/null
cmp one.bin two.bin

case_number=0
run()
{
    expected=$1
    shift
    status=0
    case_number=$((case_number + 1))
    log=$(printf 'compile-%02d.log' "$case_number")
    printf '%s\n' "$*" > "$log"
    "$emu" "$compiler" "$@" >> "$log" 2>&1 || status=$?
    if test "$status" -ne 0; then
        cat "$log"
        echo "Emulator failed with status $status: $*" >&2
        exit 1
    fi
    case "$expected" in
        0) if grep -Eq 'TPC: (compilation failed|invalid arguments)' "$log"; then
               cat "$log"
               exit 1
           fi ;;
        1) grep -q 'TPC: compilation failed' "$log" ;;
        2) grep -q 'TPC: invalid arguments' "$log" ;;
    esac
}

bytes()
{
    actual=$(od -An -tx1 -j "$1" -N "$2" hello.cmd | tr -d ' \n')
    if test "$actual" != "$3"; then
        echo "CMD header offset $1: expected $3, got $actual" >&2
        exit 1
    fi
}

run 0 -h
grep -q '^Turbo Pascal 3\.01A for CP/M-86 compiler' "$log"
grep 'Usage: TPC' "$log"
run 2
run 2 -cc -ch hello.pas
run 2 -sc G000 hello.pas
run 2 -sc 0D89 hello.pas
run 2 -sd 0FD4 hello.pas
run 2 -mi 2000 -ma 1000 hello.pas
run 2 toolongfn.pas
run 2 '*.pas'
run 2 -unknown hello.pas
run 2 -sc
run 2 -ma 10000 hello.pas
run 2 -mi -1 hello.pas
run 2 hello.pas bad.pas
run 1 missing.pas
grep -q 'File not found' "$log"
run 1 bad.pas
grep -q 'Error' "$log"
test ! -e bad.cmd
run 0 -cc hello.pas
test -s hello.cmd
test ! -e result.txt
"$emu" hello.cmd > runtime.log 2>&1
grep -aq 'CPM runtime OK' result.txt
rm -f result.txt
run 0 hello
run 0 c:hello.pas
run 0 -CC hello.pas -SC 0300 -SD 0200 -MI 0040 -MA 0080
bytes 5 4 78057805
bytes 14 4 2d022d02
bytes 23 4 40008000
run 0 -sc 0 -sd 0 -mi 0 -ma FFFF hello.pas
bytes 23 4 0000ffff
run 0 -sc 0D88 -sd 0FD3 -mi FFFF -ma FFFF hello.pas
bytes 5 4 00100010
bytes 14 4 00100010
bytes 23 4 ffffffff
run 0 -ch -sc 0100 -sd 0100 -mi 0400 -ma A000 hello.pas
test -s hello.chn
run 0 parent.pas
"$emu" parent.cmd > chain-runtime.log 2>&1
grep -aq 'CPM runtime OK' result.txt
echo 'Turbo CP/M CLI checks passed'
echo "Outputs: $work"
