#!/bin/sh
# Exercise the original editor through a real console, not redirected stdin.
set -eu
emu=$1
editor=$2
work=$3
console=$(dirname "$0")/editor-console.py
console=$(cd "$(dirname "$console")" && pwd)/editor-console.py
mkdir -p "$work"
cd "$work"
work=$(pwd)
trap 'if test -d "$work/readonly"; then chmod u+rwx "$work/readonly"; fi' 0
printf 'original line\r\n\032' > original.txt
case "$editor" in
    *.com) printf 'hello\r' > expected.txt ;;
    *.cmd) printf 'hello' > expected.txt ;;
    *) echo "Unknown editor format: $editor" >&2; exit 1 ;;
esac
printf 'hellooriginal line\r\n' > edited.txt
printf '\013d' > exit.keys
printf 'hello\013dy' > save.keys
printf 'hello\013dn' > discard.keys
printf 'hello\013dy' > edit.keys
printf 'hello\013dy\033\013dn' > failure.keys
printf '\033' > escape.keys
case_number=0
run()
{
    keys=$1
    shift
    case_number=$((case_number + 1))
    log=$(printf '%s/editor-%02d.log' "$work" "$case_number")
    status=0
    python3 "$console" "$log" "$keys" "$emu" "$editor" "$@" \
        > "$log.console" 2>&1 || status=$?
    if test "$status" -ne 0; then
        echo "Editor check $case_number failed (status $status): $*" >&2
        exit 1
    fi
}
valid()
{
    run "$@"
    if grep -Eq 'TPE: (invalid arguments|file operation failed)' "$log.text"; then
        echo "Unexpected editor diagnostic in $work/$log" >&2
        exit 1
    fi
}
content()
{
    # Native saves may pad the final CP/M record with EOF bytes.
    LC_ALL=C tr -d '\032' < "$1" > actual.txt
    cmp "$2" actual.txt
}
cp original.txt exist.pas
valid exit.keys exist.pas
cmp original.txt exist.pas
cp original.txt exist.pas
rm -f exist.bak
valid edit.keys exist.pas
content exist.pas edited.txt
cmp original.txt exist.bak
rm -f new new.pas new.bak
valid save.keys new
content new expected.txt
test ! -e new.pas
cp original.txt plain
cp original.txt plain.pas
rm -f plain.bak
valid edit.keys plain
content plain edited.txt
cmp original.txt plain.bak
cmp original.txt plain.pas
rm -f absent absent.pas absent.bak
valid exit.keys absent
test ! -e absent
test ! -e absent.pas
rm -f empty.pas empty.bak
valid exit.keys empty.pas
test ! -e empty.pas
rm -f discard.pas discard.bak
valid discard.keys discard.pas
test ! -e discard.pas
cp original.txt exist.pas
valid discard.keys exist.pas
cmp original.txt exist.pas
awk 'BEGIN { for (i = 0; i < 1000; i++) printf "%058d\r\n", i }' > large.txt
cp large.txt large.pas
printf '\032' >> large.pas
cp large.pas large-original.txt
{ printf hello; cat large.txt; } > large-edited.txt
valid edit.keys large.pas
content large.pas large-edited.txt
cmp large-original.txt large.bak
awk 'BEGIN { for (i = 0; i < 1400; i++) printf "%058d\r\n", i }' > huge.pas
cp huge.pas huge.txt
run escape.keys huge.pas
grep -Fq 'File too big' "$log.text"
cmp huge.txt huge.pas
mkdir -p readonly
chmod u+rwx readonly
cp original.txt readonly/locked.pas
chmod a-w readonly/locked.pas readonly
cd readonly
run "$work/failure.keys" locked.pas
cd "$work"
grep -Eq '(Directory|Disk) full' "$log.text"
grep -Fq 'hellooriginal line' "$log.text"
test "$(grep -o 'not saved. Save' "$log.text" | wc -l)" -eq 2
cmp original.txt readonly/locked.pas
chmod u+rwx readonly
chmod u+rw readonly/locked.pas
invalid()
{
    run exit.keys "$@"
    grep -Fq 'TPE: invalid arguments' "$log.text"
}
invalid
invalid one.pas two.pas
invalid '*.pas'
invalid ../bad.pas
invalid abcdefghi.pas
invalid bad.abcd
invalid .pas
invalid bad.
invalid 'bad+name.pas'
invalid Q:bad.pas
echo "Turbo editor: $case_number console cases passed (files/capacity/save recovery/arguments)"
