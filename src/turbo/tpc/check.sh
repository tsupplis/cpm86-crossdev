#!/bin/sh
# Exercise the local patched compiler and its generated runtime.
set -eu
emu=$1
compiler=$2
work=$3
variant=$4
case "$variant" in
    dos200)
        banner='Turbo Pascal 2.00B for DOS compiler'
        invalid_code=0D7E
        invalid_data=0FE8
        ;;
    dos302)
        banner='Turbo Pascal 3.02A for DOS compiler'
        invalid_code=0D2C
        invalid_data=0FDC
        ;;
    dms301)
        banner='Turbo Pascal 3.01A for MS-DOS compiler'
        invalid_code=0D5B
        invalid_data=0FDF
        ;;
    *) echo "Unknown DOS compiler variant: $variant" >&2; exit 1 ;;
esac
mkdir -p "$work"
cd "$work"
rm -f hello.com hello.chn bad.com result.txt
printf "program hello;\r\nvar f: text;\r\nbegin assign(f,'result.txt'); rewrite(f);\r\nwriteln(f,'CLI runtime OK'); close(f) end.\r\n" > hello.pas
printf "program bad;\r\nbegin broken end.\r\n" > bad.pas

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
    if test "$variant" != dos302; then
        if test "$status" -ne 0; then
            cat "$log"
            echo "Emulator failed with status $status: $*" >&2
            exit 1
        fi
        case "$expected" in
            0) if grep -Eq '^TPC: (compilation failed|invalid arguments)' "$log"; then
                   cat "$log"
                   exit 1
               fi ;;
            1) grep -q '^TPC: compilation failed' "$log" ;;
            2) grep -q '^TPC: invalid arguments' "$log" ;;
        esac
    elif test "$status" -ne "$expected"; then
        cat "$log"
        echo "Expected status $expected, got $status: $*" >&2
        exit 1
    fi
}

run 0 -h
grep -Fq "$banner" "$log"
grep 'Usage: TPC' "$log"
run 2
run 2 -cc -ch hello.pas
run 2 -sc G000 hello.pas
run 2 -sc "$invalid_code" hello.pas
run 2 -sd "$invalid_data" hello.pas
run 2 -mi 2000 -ma 1000 hello.pas
run 1 missing.pas
run 1 bad.pas
run 0 -cc hello.pas
test -s hello.com
test ! -e result.txt
"$emu" hello.com > runtime.log 2>&1
grep 'CLI runtime OK' result.txt
run 0 -ch -sc 0100 -sd 0100 -mi 0400 -ma A000 hello.pas
test -s hello.chn
echo "Turbo $variant CLI checks passed"
echo "Outputs: $work"
