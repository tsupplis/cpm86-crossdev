#!/bin/sh
# Golden plan tests.
#
# tests/plan/*.cases files hold the cases.  Each case is:
#
#   @case NAME
#   ARGUMENT            one acc argument per line; blank lines and lines
#   ARGUMENT            starting with '#' are ignored; '@ROOT@' expands to
#   @out                the tool root
#   EXPECTED OUTPUT     stdout+stderr with CR removed, then "[exit N]"
#   @dos                optional: expected output when the DOS build
#   EXPECTED OUTPUT     legitimately differs from the Unix build
#   @end
#
# Lines before the first @case are kept as the file header.
#
# Usage:
#   run-plan.sh ROOT ACC              Unix build
#   run-plan.sh ROOT EMULATOR ACC.EXE DOS build under emu2 (ROOT is 'D:\')
#
# Set ACC_BLESS=1 to rewrite the expected output from the current build
# (the Unix run owns @out; the DOS run only adds or removes @dos).
set -eu
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cases=$here/plan
root=$1
shift
if [ "$#" -eq 1 ]; then
    host=unix
    acc=$1
    run_acc() { "$acc" "$@"; }
else
    host=dos
    emu=$1
    acc=$2
    run_acc() { "$emu" "$acc" "$@"; }
fi
bless=${ACC_BLESS:-0}

unset INCLUDE CLIB ACCROOT AZTEC_STARTUP AZTEC_NOSTARTUP
work=$(mktemp -d "${TMPDIR:-/tmp}/acc-plan.XXXXXX")
trap 'rm -rf "$work"' EXIT
cd "$work"
work=$(pwd -P)
if [ "$host" = dos ]; then
    EMU2_DRIVE_C=$work
    EMU2_CWD='C:\'
    EMU2_DEFAULT_DRIVE=C
    export EMU2_DRIVE_C EMU2_CWD EMU2_DEFAULT_DRIVE
    hostroot=
else
    hostroot=$(cd "$root" && pwd -P)
fi

# Fixtures: planning only checks existence, so placeholders are enough.
printf 'int main(void) { return 0; }\n' > main.c
cp main.c other.c
printf '; assembly input\n' > entry.asm
printf 'object placeholder\n' > main.o
printf 'library placeholder\n' > custom.lib
: > ovraza.rsm
printf 'main.o\ncustom.lib\n' > list.inp
printf 'main.o\r\n\r\ncustom.lib\r\n' > crlf.inp
printf 'main.c\n' > source.inp
printf 'main.o extra\n' > blank.inp
printf '\n\n' > empty.inp
printf 'other.inp\n' > nested.inp
printf 'main.o\n-lm\n' > option.inp
mkdir sub inc1 libs1 fake fake/bin fake/include fake/lib nolib nolib/bin \
    nolib/include nolib/lib
cp main.c sub/main.c
cp custom.lib libs1/pick.lib
for tool in cc.exe cgen.exe as.exe ln.exe; do
    printf 'tool placeholder\n' > "nolib/bin/$tool"
done
mkdir "$work/.run"

substitute() {
    line=$1
    case $line in
        *@ROOT@*) line=${line%%@ROOT@*}$root${line#*@ROOT@} ;;
    esac
    printf '%s' "$line"
}

unsubstitute() {
    line=$1
    case $line in
        *"$root"*) line=${line%%"$root"*}@ROOT@${line#*"$root"} ;;
    esac
    printf '%s\n' "$line"
}

normalize() {
    tr -d '\r' | {
        if [ -n "$hostroot" ]; then
            sed -e "s|$hostroot|<ROOT>|g" -e "s|$work|<CWD>|g"
        else
            cat
        fi
    }
}

count=0
failed=0

# Run the case held in "$@" and .run/unix(.dos); compare, or bless.
finish_case() {
    name=$1
    shift
    expect=$work/.run/unix
    if [ "$host" = dos ] && [ -f "$work/.run/dos" ]; then
        expect=$work/.run/dos
    fi
    status=0
    run_acc "$@" > actual.raw 2>&1 || status=$?
    { normalize < actual.raw; printf '[exit %s]\n' "$status"; } > actual
    count=$((count + 1))
    if [ "$bless" = 1 ]; then
        if [ "$host" = unix ]; then
            cp actual "$work/.run/unix"
        elif cmp -s actual "$work/.run/unix"; then
            rm -f "$work/.run/dos"
        else
            cp actual "$work/.run/dos"
        fi
    elif ! cmp -s "$expect" actual; then
        printf 'FAIL %s (%s)\n' "$name" "$host" >&2
        diff -u "$expect" actual >&2 || true
        failed=$((failed + 1))
    fi
    # Planning must never create products or workspace artifacts.
    for product in hello.cmd hello.com a.cmd a.exe main.asm ovraza.cmd \
        function.ovr E:; do
        if [ -e "$product" ]; then
            printf 'FAIL %s: -### created %s\n' "$name" "$product" >&2
            rm -rf "$product"
            failed=$((failed + 1))
        fi
    done
}

for file in "$cases"/*.cases; do
    state=header
    seen=0
    : > "$work/.run/new"
    while IFS= read -r line || [ -n "$line" ]; do
        case $state:$line in
            header:@case\ *)
                name=${line#@case }
                seen=1
                set --
                : > "$work/.run/unix"
                rm -f "$work/.run/dos"
                state=args
                ;;
            header:'') [ "$seen" = 1 ] || printf '\n' >> "$work/.run/new" ;;
            header:*) printf '%s\n' "$line" >> "$work/.run/new" ;;
            args:@out) state=out ;;
            args:'' | args:'#'*) ;;
            args:*) set -- "$@" "$(substitute "$line")" ;;
            out:@dos) : > "$work/.run/dos"; state=dos ;;
            out:@end | dos:@end)
                {
                    printf '\n@case %s\n' "$name"
                    if [ "$bless" = 1 ]; then
                        for a in "$@"; do unsubstitute "$a"; done
                    fi
                } > "$work/.run/block"
                finish_case "$name" "$@"
                {
                    cat "$work/.run/block"
                    printf '@out\n'
                    cat "$work/.run/unix"
                    if [ -f "$work/.run/dos" ]; then
                        printf '@dos\n'
                        cat "$work/.run/dos"
                    fi
                    printf '@end\n'
                } >> "$work/.run/new"
                state=header
                ;;
            out:*) printf '%s\n' "$line" >> "$work/.run/unix" ;;
            dos:*) printf '%s\n' "$line" >> "$work/.run/dos" ;;
        esac
    done < "$file"
    if [ "$bless" = 1 ]; then
        cp "$work/.run/new" "$file"
    fi
done
if [ "$failed" -ne 0 ]; then
    printf 'ACC PLAN CHECK FAILED: %s of %s cases (%s)\n' "$failed" "$count" "$host" >&2
    exit 1
fi
printf 'ACC PLAN CHECK PASS: %s cases (%s)\n' "$count" "$host"
