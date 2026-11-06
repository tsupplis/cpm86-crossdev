#!/bin/sh
set -eu
root=$1
acc=$2
emu=$3
examples=$4
azdefs=$5
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
run_acc -c -squeeze main.c -o squeezed.o
test -s squeezed.o
test "$(wc -c < squeezed.o)" -lt "$(wc -c < main.o)"
run_acc -squeeze main.c -o squeezed.cmd
output=$("$emu" squeezed.cmd)
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

# Explicit assembly, verified by building (emu2 cannot run 8087 code).
# acc passes as -186 for -march=i80186 and i80286, as documented; as v4.10d
# turns out to accept 80186 instructions either way (probed), so the
# objects must be identical whatever -march says.
run_acc -S -march=i80186 math.c -o cpu186.asm
awk '/^[[:space:]]*leave/ && !done { print "\tpush 5"; print "\tpop ax";
     print "\tshl ax,3"; print "\timul ax,bx,7"; print "\tpusha"; print "\tpopa";
     done = 1 } { print }' cpu186.asm > more186.asm
grep -q 'pusha' more186.asm
run_acc -c -march=i80186 more186.asm -o cpu186a.o
run_acc -c -march=i80286 more186.asm -o cpu186b.o
run_acc -c more186.asm -o cpu186c.o
cmp cpu186a.o cpu186b.o
cmp cpu186a.o cpu186c.o
# 8087 instructions always assemble; with an 80286, as -2 drops the WAITs.
run_acc -S -mfloat=8087 math.c -o fpu.asm
run_acc -c -mfloat=8087 fpu.asm -o fpu86.o
run_acc -c -march=i80286 -mfloat=8087 fpu.asm -o fpu286.o
test "$(wc -c < fpu286.o)" -lt "$(wc -c < fpu86.o)"

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
printf '%s\n' "$output" | grep -F 'command line too long' >/dev/null
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

run_acc -c main.c
printf 'main.o\n' > objects.inp
run_acc objects.inp -o inp.cmd
output=$("$emu" inp.cmd)
printf '%s\n' "$output" | tr -d '\r' | grep -qx 'ACC EXEC PASS'
assert_clean

run_acc -save-temps -v main.o -lm -lm -lm -lm -lm -lm -lm -lm \
    -o long.cmd > saved.log 2>&1
saved=$(sed -n 's/^acc: workspace retained: //p' saved.log)
case $saved in "$work"/temps/acc.*) ;; *) cat saved.log; exit 1;; esac
# The response file is named by the plan; it must be in the workspace.
response=$(sed -n 's/^response: "E:\\\\\([^"]*\)".*/\1/p' saved.log)
test -n "$response"
test -s "$saved/$response"
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
    run_acc -target=cpm86 -overlay-root=1020,a00 ovrmain.o -o ovraza.cmd
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

# Executor failures: a tool that "succeeds" without its product, and a
# workspace parent that does not exist.  Previous outputs must survive.
printf '#!/bin/sh\nexit 0\n' > noop
chmod +x noop
cp main.c lost.c
output=$(run_acc -emu=./noop -c lost.c 2>&1) && exit 1
printf '%s\n' "$output" | grep -F 'backend did not create its output' >/dev/null
test ! -e lost.o
output=$(CCTEMP="$work/missing" run_acc -c main.c 2>&1) && exit 1
printf '%s\n' "$output" | grep -F 'cannot resolve workspace parent' >/dev/null
assert_clean

# P5: the same overlay program as a DOS 2 .exe root (target from the
# suffix), beside the CP/M-86 one: -target=dos2 selects ovrfnd1/ovrfnd2.
(
    cd overlays
    run_acc -target=dos2 -c ovrmain.c -o ovrmaind.o
    run_acc -target=dos2 -c ovrfunc1.c -o ovrfnd1.o
    run_acc -target=dos2 -c ovrfunc2.c -o ovrfnd2.o
    run_acc -overlay-root=1020,a00 ovrmaind.o -o ovrazd.exe
    test -s ovrazd.rsm
    run_acc -target=dos2 -overlay=ovrazd.rsm ovrfnd1.o -o ovrfnd1.ovr
    run_acc -target=dos2 -overlay=ovrazd.rsm ovrfnd2.o -o ovrfnd2.ovr
    output=$("$emu" ovrazd.exe)
    printf '%s\n' "$output" | tr -d '\r' |
        grep -qx 'Hello from c (ansi cc with overlay1): fn1(10,20)=80'
    printf '%s\n' "$output" | tr -d '\r' |
        grep -qx 'Hello from c (ansi cc with overlay2): fn2(10,20)=130'
)
assert_clean

# P6: azdefs.h values, and the CPU tier macros defined by acc.
mkdir defs
cp "$azdefs" defs/azdefs.h
(
    cd defs
    cat > ident.c <<'C'
#include <stdio.h>
#include "azdefs.h"
int version_ok[(__AZTEC_C_VERSION__ == 40300L) ? 1 : -1];
#if __AZTEC_C_VERSION__ != 40300
int version_preprocessor_wrong[-1];
#endif
int order_ok[(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) ? 1 : -1];
int bits_ok[(__CHAR_BIT__ == 8) ? 1 : -1];
#ifndef __i86__
int family_missing[-1];
#endif
int main(void) { puts(__VERSION__); return 0; }
C
    run_acc ident.c -o ident.cmd
    output=$("$emu" ident.cmd)
    printf '%s\n' "$output" | tr -d '\r' |
        grep -qx 'Aztec C 4.3.0 (https://github.com/tsupplis/cpm86-crossdev)'
    # acc defines __AZTEC__ (as 1) without azdefs.h; azdefs.h needs it.
    cat > family.c <<'C'
#include <stdio.h>
#ifndef __AZTEC__
int family_missing[-1];
#endif
int family_value[(__AZTEC__ == 1) ? 1 : -1];
#ifdef __AZTEC_C__
int identity_leaked[-1];
#endif
int main(void) { puts("family ok"); return 0; }
C
    run_acc family.c -o family.cmd
    "$emu" family.cmd | tr -d '\r' | grep -qx 'family ok'
    cat > guard.c <<'C'
#undef __AZTEC__
#include "azdefs.h"
#ifdef __AZTEC_C__
int guard_failed[-1];
#endif
int main(void) { return 0; }
C
    run_acc -c guard.c
    cat > cpu.c <<'C'
#ifdef EXPECT286
#if !defined(__i186__) || !defined(__i286__)
int tier_missing[-1];
#endif
#else
#if defined(__i186__) || defined(__i286__)
int tier_unexpected[-1];
#endif
#endif
int cpu_checked;
C
    run_acc -c cpu.c
    run_acc -c -march=i80286 -DEXPECT286 cpu.c -o cpu286.o
    test -s cpu286.o
)
assert_clean

# P4 probes: every allow-listed backend option is accepted by the real tool
# and, where observable, has its documented effect.
mkdir raw
cp main.c raw/
(
    cd raw
    pass() {
        output=$("$emu" "$1")
        printf '%s\n' "$output" | tr -d '\r' | grep -qx 'ACC EXEC PASS'
    }
    # The backend must reject the build; its exact status is not asserted.
    fails() {
        status=0
        run_acc "$@" > fails.log 2>&1 || status=$?
        test "$status" -ne 0
    }
    # cc -L: a function with 120 locals overflows the default table.
    {
        printf 'int f(void){int '
        i=0
        while [ $i -lt 119 ]; do printf 'v%d,' $i; i=$((i + 1)); done
        printf 'v119; v0 = 1; return v0;}\n'
    } > locals.c
    fails -c locals.c
    run_acc -c -Wc,-L300 locals.c
    test -s locals.o
    # cc table sizes and code options: compile, link and run.
    run_acc -Wc,-E120 -Wc,-y100 -Wc,-z1000 -Wc,+U -Wc,+R -Wc,+A -Wc,+M \
        main.c -o opts.cmd
    pass opts.cmd
    run_acc -target=dos2 -mcmodel=large -Wc,+s main.c -o strings.exe
    pass strings.exe
    # cc -k: far is no longer a keyword.
    printf 'char far *p;\nint main(void) { return 0; }\n' > far.c
    run_acc -c far.c
    fails -c -Wc,-k far.c
    # cc -T: C source statements appear as comments in the assembly.
    run_acc -S -Wc,-T main.c -o comment.asm
    grep -q '^;.*puts' comment.asm
    # as -S, -C, and -D with a constant.
    run_acc -S main.c
    run_acc -c -Wa,-S3 -Wa,-C main.asm -o squeezed.o > listing.log
    run_acc squeezed.o -o squeezed.cmd
    pass squeezed.cmd
    awk '/^[[:space:]]*end([[:space:]]|$)/ && !done { print "\tdw ACCVAL"; done = 1 }
         { print }' main.asm > valued.asm
    grep -q 'ACCVAL' valued.asm
    run_acc -c -DACCVAL=5 valued.asm
    run_acc valued.o -o valued.cmd
    pass valued.cmd
    rm -f valued.o
    status=0
    { run_acc -c valued.asm && run_acc valued.o -o unvalued.cmd; } \
        > undefined.log 2>&1 || status=$?
    test "$status" -ne 0
    # ln -M -V; DOS memory hints -S and -X.
    run_acc -c main.c
    run_acc -Wl,-M -Wl,-V main.o -o quiet.cmd > verbose.log
    pass quiet.cmd
    run_acc -Wl,-S400 -Wl,-X1000 main.o -o memory.exe
    pass memory.exe
    # -g: sdb information, published as .dbg beside the program.
    run_acc -g main.c -o debug.exe
    test -s debug.dbg
    pass debug.exe
    run_acc -g main.c -o debug.cmd
    test -s debug.dbg
    pass debug.cmd
)
assert_clean
printf 'ACC EXEC CHECK PASS\n'
