#!/bin/sh
set -eu
root=$1
acc=$2
emu=$3
examples=$4
work=$(mktemp -d "${TMPDIR:-/tmp}/acc-exec.XXXXXX")
trap 'rm -rf "$work"' EXIT
unset INCLUDE CLIB ACCROOT CCTEMP TEMP AZTEC_STARTUP AZTEC_NOSTARTUP
cd "$work"
work=$(pwd -P)
mkdir temps
TMPDIR=$work/temps
export TMPDIR

run_acc() {
    "$acc" -toolroot="$root" -emu="$emu" "$@"
}

expect_error() {
    expected=$1
    shift
    status=0
    output=$(run_acc "$@" 2>&1) || status=$?
    if [ "$status" -ne "$expected" ]; then
        printf 'Expected status %s, got %s\n%s\n' "$expected" "$status" "$output" >&2
        exit 1
    fi
}

assert_clean() {
    test -z "$(find temps -mindepth 1 -print)"
}

cat > main.c <<'C'
#include <stdio.h>
int main(void) { puts("ACC EXEC PASS"); return 0; }
C
run_acc -c main.c
test -s main.o
run_acc -S main.c
test -s main.asm
run_acc -c main.asm -o asm.o
test -s asm.o
run_acc asm.o -o asm.cmd
output=$("$emu" asm.cmd)
printf '%s\n' "$output" | tr -d '\r' | grep -qx 'ACC EXEC PASS'
run_acc main.o -o main.cmd
output=$("$emu" main.cmd)
printf '%s\n' "$output" | tr -d '\r' | grep -qx 'ACC EXEC PASS'

for target in dos1 dos2; do
    run_acc -target="$target" main.c -o main.exe
    output=$("$emu" main.exe)
    printf '%s\n' "$output" | tr -d '\r' | grep -qx 'ACC EXEC PASS'
done
for model in small medium compact large; do
    run_acc -target=dos2 -mcmodel="$model" main.c -o main.exe
    output=$("$emu" main.exe)
    printf '%s\n' "$output" | tr -d '\r' | grep -qx 'ACC EXEC PASS'
done
printf 'int main(void) { return 0; }\n' > empty.c
run_acc -target=dos2 empty.c -o empty.exe
"$emu" empty.exe
assert_clean

cat > math.c <<'C'
#include <math.h>
#include <stdio.h>
int main(void)
{
    volatile double value = 2.0;
    double result;
    result = sqrt(value * value);
    if (fabs(result - 2.0) > 0.00001) {
        printf("unexpected sqrt: %g\n", result);
        return 2;
    }
    puts("ACC MATH PASS");
    return 0;
}
C
run_acc math.c -lm -o math.cmd
output=$("$emu" math.cmd)
printf '%s\n' "$output" | tr -d '\r' | grep -qx 'ACC MATH PASS'
for model in small medium compact large; do
    run_acc -target=dos2 -mcmodel="$model" math.c -lm -o math.exe
    output=$("$emu" math.exe)
    printf '%s\n' "$output" | tr -d '\r' | grep -qx 'ACC MATH PASS'
    # emu2 cannot execute coprocessor code; verify these builds without running them.
    for floating in auto 8087; do
        run_acc -target=dos2 -mcmodel="$model" -march=i80286 \
            -mfloat="$floating" math.c -lm -o hard.exe
        test -s hard.exe
    done
    run_acc -S -target=dos2 -march=i80286 -mfloat=8087 math.c -o hard.asm
    grep -Ei '^[[:space:]]*f(ld|st|mul|sub|add|div)' hard.asm >/dev/null
done

mkdir inc1 inc2 libs1 libs2
printf '#define VALUE 11\n' > inc1/pick.h
printf '#define VALUE 22\n' > inc2/pick.h
printf '#define LOCAL 3\n' > local.h
cat > include.c <<'C'
#include <stdio.h>
#include <pick.h>
#include "local.h"
int main(void) { printf("%d\n", VALUE + LOCAL); return 0; }
C
(INCLUDE=inc2 run_acc -Iinc1 include.c -o inc.cmd)
output=$("$emu" inc.cmd)
printf '%s\n' "$output" | tr -d '\r' | grep -qx 14
(INCLUDE=':inc2:' run_acc include.c -o inc.cmd)
output=$("$emu" inc.cmd)
printf '%s\n' "$output" | tr -d '\r' | grep -qx 25
printf 'int pick(void) { return 11; }\n' > pick.c
printf 'int pick(void); int main(void) { return pick(); }\n' > use.c
run_acc -target=dos2 use.c pick.c -o both.exe
status=0
"$emu" both.exe || status=$?
test "$status" = 11
run_acc -target=dos2 -c pick.c
EMU2_DRIVE_C=$work EMU2_CWD="C:\\" "$emu" "$root/bin/lb.exe" libs1/pick.lib pick.o
cp libs1/pick.lib libs2/pick.lib
(CLIB=libs2 run_acc -target=dos2 -Llibs1 use.c -lpick -o use.exe)
status=0
"$emu" use.exe || status=$?
test "$status" = 11
PATH=$(dirname "$emu"):$PATH "$acc" -toolroot="$root" -c main.c -o path.o
test -s path.o

printf 'not C source !!!\n' > bad.c
printf 'old object\n' > bad.o
expect_error 4 -c bad.c
grep -qx 'old object' bad.o
printf 'int absent(void); int main(void) { return absent(); }\n' > undef.c
printf 'old executable\n' > old.cmd
expect_error 1 undef.c -o old.cmd
grep -qx 'old executable' old.cmd
assert_clean

cat > fake <<'SH'
#!/bin/sh
printf 'called\n' > called
printf '%s\n' "$EMU2_DRIVE_C" "$EMU2_DRIVE_D" "$EMU2_DRIVE_E" \
    "$EMU2_CWD" "$EMU2_DEFAULT_DRIVE" "$EMU2_PROGNAME" "${EMU2_CPU:-}" > env.log
exit "${FAKE_STATUS:-0}"
SH
chmod +x fake
FAKE_STATUS=7 expect_error 7 -emu=./fake -c main.c
FAKE_STATUS=0 expect_error 1 -emu=./fake -c main.c
printf '%s\n' "$output" | grep -F 'did not create its output' >/dev/null
assert_clean
rm called
long=$(printf '%0140d' 0)
expect_error 1 -emu=./fake -c -DLONG="$long" main.c
printf '%s\n' "$output" | grep -F 'command-tail limit' >/dev/null
test ! -e called
expect_error 1 -emu=./fake -c '-DSTR="two words"' main.c
test ! -e called
expect_error 1 -emu=./missing -c main.c
assert_clean

(EMU2_DRIVE_C=/bad EMU2_DRIVE_D=/bad EMU2_DRIVE_E=/bad \
    EMU2_CWD='Z:\BAD' EMU2_DEFAULT_DRIVE=Z EMU2_PROGNAME='Z:\BAD.EXE' \
    EMU2_APPEND='Z:\BAD' EMU2_CPU=8086 \
    run_acc -c main.c -o env.o)
test -s env.o
(EMU2_CPU=8088 FAKE_STATUS=9 expect_error 9 -emu=./fake -c main.c)
grep -qx '8088' env.log
grep -Fx "$root" env.log >/dev/null
grep -Fx "$work" env.log >/dev/null

cat > signal <<'SH'
#!/bin/sh
kill -TERM "$$"
SH
chmod +x signal
expect_error 143 -emu=./signal -c main.c
assert_clean
cat > hold <<'SH'
#!/bin/sh
trap 'exit 0' TERM
printf 'ready\n' > ready
while :; do :; done
SH
chmod +x hold
"$acc" -toolroot="$root" -emu=./hold -c main.c > interrupt.log 2>&1 &
driver=$!
attempt=0
while [ ! -e ready ]; do
    attempt=$((attempt + 1))
    if [ "$attempt" -ge 100 ]; then
        kill -TERM "$driver"
        wait "$driver" || :
        cat interrupt.log
        exit 1
    fi
    sleep 0.05
done
kill -TERM "$driver"
status=0
wait "$driver" || status=$?
test "$status" = 143
grep -F 'acc: interrupted by signal' interrupt.log >/dev/null
assert_clean

run_acc -c main.c -o left.o &
left=$!
run_acc -c main.c -o right.o &
right=$!
wait "$left"
wait "$right"
cmp left.o right.o
assert_clean

run_acc -save-temps -v main.o -lm -lm -lm -lm -lm -lm -lm -lm \
    -o long.cmd > saved.log 2>&1
saved=$(sed -n 's/^acc: workspace retained: //p' saved.log)
case $saved in "$work"/temps/acc.*) ;; *) cat saved.log; exit 1;; esac
test -s "$saved/link.rsp"
output=$("$emu" long.cmd)
printf '%s\n' "$output" | tr -d '\r' | grep -qx 'ACC EXEC PASS'
rm -rf "$saved"
assert_clean

mkdir high middle
(CCTEMP=$work/high TEMP=$work/middle run_acc -save-temps -c main.c) > saved.log 2>&1
saved=$(sed -n 's/^acc: workspace retained: //p' saved.log)
case $saved in "$work"/high/acc.*) ;; *) cat saved.log; exit 1;; esac
rm -rf "$saved"
(CCTEMP=$work/missing expect_error 1 -c main.c)
assert_clean

mkdir migrated-examples
cp "$examples/helloazc.c" "$examples/helloaza.c" "$examples/helloaz7.c" \
    "$examples/mathazc.c" migrated-examples/
(
    cd migrated-examples
    run_acc -target=cpm86 helloazc.c -o helloazc.cmd
    output=$("$emu" helloazc.cmd)
    printf '%s\n' "$output" | tr -d '\r' | grep -q '^Hello from c (aztec cc)'
    run_acc -target=cpm86 helloaza.c -o helloaza.cmd
    output=$("$emu" helloaza.cmd)
    printf '%s\n' "$output" | tr -d '\r' |
        grep -q '^Hello from c (ansi cc): add(10,20)=30'
    run_acc -target=cpm86 helloaz7.c -lm -o helloaz7.cmd
    output=$("$emu" helloaz7.cmd)
    printf '%s\n' "$output" | tr -d '\r' |
        grep -q '^Hello from c (ansi cc): fdiv(22,7)=3.1429'
    run_acc -target=cpm86 mathazc.c -lm -o mathazc.cmd
    output=$("$emu" mathazc.cmd)
    printf '%s\n' "$output" | tr -d '\r' |
        grep -qx 'Test double   math: 5635678.0 / 1234.0 = 4567.00 (4567)'
    run_acc -target=dos1 helloaza.c -o helloazd.exe
    output=$("$emu" helloazd.exe)
    printf '%s\n' "$output" | tr -d '\r' |
        grep -q '^Hello from c (ansi cc): add(10,20)=30'
    run_acc -target=dos1 mathazc.c -lm -o mathazd.exe
    output=$("$emu" mathazd.exe)
    printf '%s\n' "$output" | tr -d '\r' |
        grep -qx 'Test double   math: 5635678.0 / 1234.0 = 4567.00 (4567)'
)

mkdir overlays
cp "$examples/ovrmain.c" "$examples/ovrfunc1.c" "$examples/ovrfunc2.c" overlays/
(
    cd overlays
    run_acc -target=cpm86 -c ovrmain.c
    run_acc -target=cpm86 -c ovrfunc1.c
    run_acc -target=cpm86 -c ovrfunc2.c
    run_acc -target=cpm86 -overlay-root=1020,aOO ovrmain.o -o ovraza.cmd
    test -s ovraza.cmd
    test -s ovraza.rsm
    run_acc -target=cpm86 -overlay=ovraza.rsm ovrfunc1.o -o ovrfunc1.ovr
    run_acc -target=cpm86 -overlay=ovraza.rsm ovrfunc2.o -o ovrfunc2.ovr
    test -s ovrfunc1.ovr
    test -s ovrfunc2.ovr
    output=$("$emu" ovraza.cmd)
    printf '%s\n' "$output" | tr -d '\r' |
        grep -qx 'Hello from c (ansi cc with overlay1): fn1(10,20)=80'
    printf '%s\n' "$output" | tr -d '\r' |
        grep -qx 'Hello from c (ansi cc with overlay2): fn2(10,20)=130'
)
assert_clean
printf 'ACC EXEC CHECK PASS\n'
