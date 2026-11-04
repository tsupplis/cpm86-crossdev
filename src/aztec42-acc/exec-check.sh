#!/bin/sh
set -eu
root=$1
acc=$2
emu=$3
work=$(mktemp -d "${TMPDIR:-/tmp}/acc-exec.XXXXXX")
trap 'rm -rf "$work"' EXIT
unset INCLUDE CLIB ACCROOT CCTEMP TEMP AZTEC_STARTUP AZTEC_NOSTARTUP
cd "$work"
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
INCLUDE=inc2 run_acc -Iinc1 include.c -o inc.cmd
output=$("$emu" inc.cmd)
printf '%s\n' "$output" | tr -d '\r' | grep -qx 14
INCLUDE=':inc2:' run_acc include.c -o inc.cmd
output=$("$emu" inc.cmd)
printf '%s\n' "$output" | tr -d '\r' | grep -qx 25
printf 'int pick(void) { return 11; }\n' > pick.c
printf 'int pick(void); int main(void) { return pick(); }\n' > use.c
run_acc -target=dos2 -c pick.c
EMU2_DRIVE_C=$work EMU2_CWD="C:\\" "$emu" "$root/bin/lb.exe" libs1/pick.lib pick.o
cp libs1/pick.lib libs2/pick.lib
CLIB=libs2 run_acc -target=dos2 -Llibs1 use.c -lpick -o use.exe
status=0
"$emu" use.exe || status=$?
test "$status" = 11

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
expect_error 1 -emu=./fake -c main.c
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

EMU2_DRIVE_C=/bad EMU2_DRIVE_D=/bad EMU2_DRIVE_E=/bad \
    EMU2_CWD='Z:\BAD' EMU2_DEFAULT_DRIVE=Z EMU2_PROGNAME='Z:\BAD.EXE' \
    EMU2_APPEND='Z:\BAD' EMU2_CPU=8086 \
    run_acc -c main.c -o env.o
test -s env.o
EMU2_CPU=8088 FAKE_STATUS=9 expect_error 9 -emu=./fake -c main.c
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
CCTEMP=$work/high TEMP=$work/middle run_acc -save-temps -c main.c > saved.log 2>&1
saved=$(sed -n 's/^acc: workspace retained: //p' saved.log)
case $saved in "$work"/high/acc.*) ;; *) cat saved.log; exit 1;; esac
rm -rf "$saved"
CCTEMP=$work/missing expect_error 1 -c main.c
assert_clean
printf 'ACC EXEC CHECK PASS\n'
