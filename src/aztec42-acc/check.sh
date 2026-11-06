#!/bin/sh
set -eu
root=$1
shift
unset INCLUDE CLIB ACCROOT AZTEC_STARTUP AZTEC_NOSTARTUP
work=$(mktemp -d "${TMPDIR:-/tmp}/acc-check.XXXXXX")
trap 'rm -rf "$work"' EXIT
cd "$work"
if [ "$#" -gt 1 ]; then
    EMU2_DRIVE_C=$work
    EMU2_CWD="C:\\"
    EMU2_DEFAULT_DRIVE=C
    export EMU2_DRIVE_C EMU2_CWD EMU2_DEFAULT_DRIVE
fi
printf 'int main(void) { return 0; }\n' > main.c
cp main.c other.c
printf '; assembly input\n' > entry.asm
printf 'object placeholder\n' > main.o
printf 'library placeholder\n' > custom.lib

output=$("$@" -help)
printf '%s\n' "$output" | tr -d '\r' | grep -q '^Usage: acc '

output=$("$@" -version)
printf '%s\n' "$output" | tr -d '\r' |
    grep -qx 'acc 0.3-dev'

output=$("$@" -help 2>&1)
printf '%s\n' "$output" | grep -F -- '-overlay-root=CODE,DATA' >/dev/null
printf '%s\n' "$output" | grep -F -- '-overlay=ROOT.rsm' >/dev/null

expect_failure() {
    status=0
    output=$("$@" 2>&1) || status=$?
    if [ "$status" -ne 1 ]; then
        printf 'Expected status 1, got %s\n%s\n' "$status" "$output" >&2
        exit 1
    fi
    printf '%s\n' "$output" | tr -d '\r' | grep -q '^acc: '
}

expect_error() {
    expected=$1
    shift
    expect_failure "$@"
    printf '%s\n' "$output" | grep -F "$expected" >/dev/null
}

expect_failure "$@"
expect_failure "$@" -unknown
expect_failure "$@" --help
expect_failure "$@" -help extra
expect_failure "$@" hello.c
expect_failure "$@" -### -toolroot="$root" -c -S main.c
expect_failure "$@" -### -toolroot="$root" -target=cpm86 -mcmodel=large main.c
expect_failure "$@" -### -toolroot="$root" -target=dos2 -mcmodel=garbage main.c
expect_failure "$@" -### -toolroot="$root" -c main.c other.c -o both.o
expect_failure "$@" -### -toolroot="$root" -S entry.asm
expect_failure "$@" -### -toolroot="$root" -c main.o
expect_failure "$@" -### -toolroot="$root" -target=cpm86 main.c -o wrong.exe
expect_failure "$@" -### -toolroot="$root" -target=cpm86 -D__MSDOS__ main.c
expect_failure "$@" -### -toolroot="$root" -DTEST=1 -DTEST=2 main.c
expect_failure "$@" -### -toolroot="$root" -D9BAD main.c
expect_failure "$@" -### -toolroot="$root" main.c -o
expect_failure "$@" -### -toolroot="$root" @args.rsp
expect_failure "$@" -### -toolroot="$root" -c -Wc,-A main.c
expect_failure "$@" -### -toolroot="$root" -mfloat=8087-fast main.c
expect_error 'cannot read input' "$@" -### -toolroot="$root" missing.c
expect_error 'select the target/model' "$@" -### -toolroot="$root" main.o -lc86
expect_error 'output aliases an input' "$@" -### -toolroot="$root" -c main.c -o main.c
expect_error 'duplicate input' "$@" -### -toolroot="$root" main.c ./main.c
expect_failure "$@" -### -toolroot="$root" custom.lib
expect_error 'output aliases an input' "$@" -### -toolroot="$root" -c main.c -o ./main.c
expect_error 'parent does not exist' "$@" -### -toolroot="$root" -c main.c -o missing/a.o
expect_error 'reserved DOS device' "$@" -### -toolroot="$root" -c main.c -o nul.o
expect_error 'DOS filename aliasing' "$@" -### -toolroot="$root" -c main.c -o .o
expect_error 'invalid option value' "$@" -### -toolroot="$root" -march=bad -march=i8086 main.c
mkdir sub
cp main.c sub/main.c
expect_error 'output collision' "$@" -### -toolroot="$root" -c main.c sub/main.c

output=$("$@" -### -toolroot="$root" main.c -o hello.cmd 2>&1)
output=$(printf '%s\n' "$output" | tr -d '\r')
printf '%s\n' "$output" | grep -Fi '"D:\\bin\\cc.exe" "+0" "-b" "-n"' >/dev/null
if printf '%s\n' "$output" | grep -F '__CPM86__'; then exit 1; fi
printf '%s\n' "$output" | grep -F 'begin86.o' >/dev/null
printf '%s\n' "$output" | grep -F 'c86.lib' >/dev/null
test ! -e hello.cmd
test ! -e 'E:'
limit=$(printf '%072d' 0)
output=$("$@" -### -toolroot="$root" -c -DLONG="$limit" main.c 2>&1)
limit=$(printf '%073d' 0)
expect_error 'command line too long' "$@" -### -toolroot="$root" -c -DLONG="$limit" main.c

output=$("$@" -### -toolroot="$root" -S -ansi -DTEST= -I. main.c 2>&1)
output=$(printf '%s\n' "$output" | tr -d '\r')
printf '%s\n' "$output" | grep -F '"-A"' >/dev/null
printf '%s\n' "$output" | grep -F '"-ansi"' >/dev/null
printf '%s\n' "$output" | grep -F '"-DTEST="' >/dev/null
if printf '%s\n' "$output" | grep -Fi 'ln.exe'; then exit 1; fi
test ! -e main.asm

for model in small medium compact large; do
    case $model in
        small) startup=sbegin.o; runtime=c.lib; soft=m.lib; auto=m87s.lib; hw=m87.lib;;
        medium) startup=beginlc.o; runtime=clc.lib; soft=mlc.lib; auto=m87slc.lib; hw=m87lc.lib;;
        compact) startup=beginld.o; runtime=cld.lib; soft=mld.lib; auto=m87sld.lib; hw=m87ld.lib;;
        large) startup=lbegin.o; runtime=cl.lib; soft=ml.lib; auto=m87sl.lib; hw=m87l.lib;;
    esac
    for floating in soft auto 8087; do
        case $floating in soft) math=$soft;; auto) math=$auto;; 8087) math=$hw;; esac
        output=$("$@" -### -toolroot="$root" -target=dos2 -mcmodel="$model" \
            -mfloat="$floating" main.o -lm 2>&1)
        output=$(printf '%s\n' "$output" | tr -d '\r')
        printf '%s\n' "$output" | grep -F "$startup" >/dev/null
        printf '%s\n' "$output" | grep -F "$runtime" >/dev/null
        printf '%s\n' "$output" | grep -F "$math" >/dev/null
        printf '%s\n' "$output" |
            grep -E "${math%.lib}\\.lib.*${runtime%.lib}\\.lib" >/dev/null
    done
done

output=$("$@" -### -toolroot="$root" -target=dos1 main.o 2>&1)
printf '%s\n' "$output" | grep -F 'begin11.o' >/dev/null
printf '%s\n' "$output" | grep -F 'd11.lib' >/dev/null
output=$("$@" -### -toolroot="$root" -L. main.o -lcustom -lm -lcustom 2>&1)
printf '%s\n' "$output" | grep -E 'custom\.lib.*m\.lib.*custom\.lib.*c86\.lib' >/dev/null
output=$("$@" -### -toolroot="$root" main.o custom.lib custom.lib 2>&1)
printf '%s\n' "$output" | grep -E 'custom\.lib.*custom\.lib.*c86\.lib' >/dev/null
output=$("$@" -### -toolroot="$root" -nostartfiles main.o 2>&1)
printf '%s\n' "$output" | grep -F 'c86.lib' >/dev/null
if printf '%s\n' "$output" | grep -F 'begin86.o'; then exit 1; fi

output=$("$@" -### -toolroot="$root" -overlay-root=1020,a00 \
    main.o -o ovraza.cmd 2>&1)
output=$(printf '%s\n' "$output" | tr -d '\r')
printf '%s\n' "$output" | grep -F '"+c" "1020" "+d" "a00" "-r"' >/dev/null
printf '%s\n' "$output" | grep -F 'ovld.o' >/dev/null
printf '%s\n' "$output" | grep -F 'c86.lib' >/dev/null
printf '%s\n' "$output" | grep -E 'E:\\\\u[0-9]{4}\.rsm' >/dev/null
printf '%s\n' "$output" | grep -F 'ovraza.rsm' >/dev/null
test ! -e ovraza.cmd
test ! -e ovraza.rsm
: > ovraza.rsm
output=$("$@" -### -toolroot="$root" -overlay=ovraza.rsm \
    main.o -o function.ovr 2>&1)
output=$(printf '%s\n' "$output" | tr -d '\r')
printf '%s\n' "$output" | grep -F 'ovraza.rsm' >/dev/null
printf '%s\n' "$output" | grep -F 'ovbgn.o' >/dev/null
if printf '%s\n' "$output" | grep -F 'c86.lib'; then exit 1; fi
rm ovraza.rsm

output=$("$@" -### -toolroot="$root" -nostdlib entry.asm 2>&1)
output=$(printf '%s\n' "$output" | tr -d '\r')
printf '%s\n' "$output" | grep -Fi 'as.exe' >/dev/null
if printf '%s\n' "$output" | grep -E 'begin.*\.o|c86\.lib'; then exit 1; fi
output=$("$@" -### -toolroot="$root" -c main.c other.c 2>&1)
output=$(printf '%s\n' "$output" | tr -d '\r')
test "$(printf '%s\n' "$output" | grep -ic 'cc.exe')" = 2
if printf '%s\n' "$output" | grep -Fi 'ln.exe'; then exit 1; fi

output=$("$@" -### -toolroot="$root" -c -march=i80186 -march=i80286 main.c 2>&1)
printf '%s\n' "$output" | grep -F '"+2"' >/dev/null
output=$("$@" -### -toolroot="$root" -c -march=i80186 entry.asm 2>&1)
printf '%s\n' "$output" | grep -F '"-186"' >/dev/null
expect_error 'not qualified yet' "$@" -### -toolroot="$root" -march=i80286 entry.asm
output=$("$@" -### -toolroot="$root" -c -mfloat=8087 -Ospeed -w main.c 2>&1)
printf '%s\n' "$output" | grep -F '"+e" "+f" "-s"' >/dev/null

mkdir inc1 inc2 libs1 libs2
cp custom.lib libs1/pick.lib
cp custom.lib libs2/pick.lib
if [ "$#" -eq 1 ]; then
    hostroot=$root
    output=$(INCLUDE='inc2:' CLIB='libs2:' "$@" -### -toolroot="$root" \
        -Iinc1 -Llibs1 main.c -lpick 2>&1)
else
    hostroot=${EMU2_DRIVE_D:?DOS checks require EMU2_DRIVE_D}
    output=$("$@" -### -toolroot="$root" -Iinc1 -Llibs1 main.c -lpick -- \
        INCLUDE='inc2;' CLIB='libs2;' 2>&1)
fi
output=$(printf '%s\n' "$output" | tr -d '\r')
printf '%s\n' "$output" | grep -F \
    '"-IC:\\inc1" "-IC:\\inc2" "-IC:\\" "-ID:\\include"' >/dev/null
printf '%s\n' "$output" | grep -Fi 'C:\\libs1\\pick.lib' >/dev/null
if printf '%s\n' "$output" | grep -Fi 'libs2\\pick.lib'; then exit 1; fi

ln -s "$hostroot" sdk
output=$("$@" -### -toolroot=sdk -c main.c 2>&1)
if [ "$#" -gt 1 ]; then
    printf '%s\n' "$output" | grep -Fi '"C:\\sdk\\bin\\cc.exe"' >/dev/null
    # acc.exe receives at most 125 characters of arguments intact: emu2, like
    # DOS, joins them with single spaces and cuts the tail at 126.  Tail
    # length here is 33 + the number of zeros.
    zeros=$(printf '%092d' 0)
    expect_error 'compiler command line too long' "$@" -### -toolroot="$root" \
        -c "-DX=$zeros" main.c
    expect_error 'command line fills the DOS limit' "$@" -### -toolroot="$root" \
        -c "-DX=${zeros}0" main.c
    # The workspace parent must leave room for 8.3 names within 63 characters.
    expect_error 'DOS workspace parent exceeds' "$@" -toolroot="$root" -c main.c -- \
        CCTEMP='C:\aaaaaaaa\bbbbbbbb\cccccccc\dddddddd\eeeeeeee\ffffffff\gggggggg'
    output=$("$@" -### -c main.c -- ACCROOT='C:\sdk' 2>&1)
    printf '%s\n' "$output" | grep -Fi '"C:\\sdk\\bin\\cc.exe"' >/dev/null
else
    output=$(ACCROOT=sdk "$@" -### -c main.c 2>&1)
    printf '%s\n' "$output" | grep -Fi '"D:\\bin\\cc.exe"' >/dev/null
    ln main.c alias.o
    expect_error 'output aliases an input' "$@" -### -toolroot="$root" -c main.c -o alias.o
    ln -s /etc/hosts away.c
    expect_error 'path lies outside' "$@" -### -toolroot="$root" -c away.c
    cp main.c 'bad\name.c'
    expect_error 'cannot be represented safely in DOS' "$@" -### -toolroot="$root" -c 'bad\name.c'

    expect_error 'empty argument' "$@" -### -toolroot="$root" '' main.c
    long=$(awk 'BEGIN { s = ""; for (i = 0; i < 1100; ++i) s = s "x"; print s }')
    output=$(INCLUDE="$long" "$@" -### -toolroot="$root" -c main.c 2>&1) || true
    printf '%s\n' "$output" | grep -F 'environment path is too long' >/dev/null
    output=$(AZTEC_STARTUP=x "$@" -### -toolroot="$root" -c main.c 2>&1) || true
    printf '%s\n' "$output" | grep -F 'legacy AZTEC_STARTUP/AZTEC_NOSTARTUP' >/dev/null
    ln -s nowhere dangling.o
    expect_error 'cannot resolve existing output' "$@" -### -toolroot="$root" -c main.c -o dangling.o

    # Tool root discovery: acc finds <root> when it runs as <root>/bin/acc
    # and <root>/bin/cc.exe exists; -toolroot and ACCROOT are not needed.
    mkdir -p found/bin found/include found/lib lone/bin
    cp "$1" found/bin/acc
    cp "$1" lone/bin/acc
    : > found/bin/cc.exe
    expect_error 'required backend tool is missing' found/bin/acc -### -c main.c
    expect_error 'cgen.exe' found/bin/acc -### -c main.c
    expect_error 'required backend tool is missing' env PATH="$work/found/bin:$PATH" acc -### -c main.c
    expect_error 'no Aztec tool root' lone/bin/acc -### -c main.c
    expect_error 'invalid Aztec installation root' found/bin/acc -### -toolroot=nowhere -c main.c
fi
mkdir -p fake/bin fake/include fake/lib
expect_error 'required backend tool is missing' "$@" -### -toolroot=fake -c main.c
printf 'linker placeholder\n' > fake/bin/ln.exe
expect_error 'missing installation library or startup' "$@" -### -toolroot=fake main.o

printf 'ACC CHECK PASS: %s\n' "$*"
