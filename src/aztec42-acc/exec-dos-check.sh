#!/bin/sh
set -eu
acc=$1
emu=$2
root=$3
examples=$4
work=$(mktemp -d "${TMPDIR:-/tmp}/acc-dos-check.XXXXXX")
trap 'rm -rf "$work"' EXIT

printf 'int main(void) { return 0; }\n' > "$work/main.c"
printf 'int main( {\n' > "$work/bad.c"
for part in 1 2 3 4 5 6 7 8; do
    printf 'int part%s(void) { return %s; }\n' "$part" "$part" > "$work/part$part.c"
done
export EMU2_DRIVE_C=$work
export EMU2_DRIVE_D=$root
export EMU2_CWD='C:\'
export EMU2_DEFAULT_DRIVE=C
dos_acc() {
    "$emu" "$acc" -toolroot='D:\' "$@"
}
cd "$work"

"$emu" "$acc" -toolroot='D:\' -target=dos2 main.c -o hello.exe
test -s hello.exe
"$emu" "$work/hello.exe"
"$emu" "$acc" -toolroot='D:\' -target=dos2 main.c part1.c part2.c \
    part3.c part4.c part5.c part6.c part7.c part8.c -o many.exe
test -s many.exe
"$emu" "$work/many.exe"

dos_acc -target=dos2 -c main.c
printf 'main.o\r\n' > objects.inp
dos_acc -target=dos2 objects.inp -o fromlist.exe
test -s fromlist.exe
"$emu" "$work/fromlist.exe"

cp hello.exe preserved.exe
status=0
"$emu" "$acc" -toolroot='D:\' -target=dos2 bad.c -o hello.exe \
    >failure.log 2>&1 || status=$?
if [ "$status" -eq 0 ]; then
    printf 'Expected the DOS compiler failure to propagate\n' >&2
    exit 1
fi
cmp hello.exe preserved.exe
grep -q 'backend failed' failure.log

mkdir overlays
cp "$examples/ovrmain.c" "$examples/ovrfunc1.c" "$examples/ovrfunc2.c" overlays/
(
    cd overlays
    export EMU2_CWD='C:\overlays'
    dos_acc -target=cpm86 -c ovrmain.c
    dos_acc -target=cpm86 -c ovrfunc1.c
    dos_acc -target=cpm86 -c ovrfunc2.c
    dos_acc -target=cpm86 -overlay-root=1020,aOO ovrmain.o -o ovraza.cmd
    test -s ovraza.cmd
    test -s ovraza.rsm
    dos_acc -target=cpm86 -overlay=ovraza.rsm ovrfunc1.o -o ovrfunc1.ovr
    dos_acc -target=cpm86 -overlay=ovraza.rsm ovrfunc2.o -o ovrfunc2.ovr
    test -s ovrfunc1.ovr
    test -s ovrfunc2.ovr
    output=$("$emu" "$work/overlays/ovraza.cmd")
    printf '%s\n' "$output" | tr -d '\r' |
        grep -qx 'Hello from c (ansi cc with overlay1): fn1(10,20)=80'
    printf '%s\n' "$output" | tr -d '\r' |
        grep -qx 'Hello from c (ansi cc with overlay2): fn2(10,20)=130'
)

for path in A??????; do
    if [ -d "$path" ]; then
        printf 'DOS acc left a private workspace: %s\n' "$path" >&2
        exit 1
    fi
done
printf 'ACC DOS EXEC CHECK PASS\n'
