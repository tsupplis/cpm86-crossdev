#!/bin/sh
# Self-hosting fixed point: the DOS acc.exe, run under emu2, must plan
# every DOS module exactly like the Unix acc, and the acc.exe it builds
# must be byte-identical to the acc.exe the Unix acc built.
#
# Usage: self-host.sh SOURCE_DIR TOOL_ROOT EMULATOR "DOS_CFLAGS"
set -eu
src=$1
root=$2
emu=$3
flags=$4
cd "$src"
self=obj/self
rm -rf "$self"
mkdir -p "$self"
trap 'rm -rf "$src/$self"' EXIT

EMU2_DRIVE_C=$src
EMU2_DRIVE_D=$root
EMU2_CWD='C:\'
EMU2_DEFAULT_DRIVE=C
export EMU2_DRIVE_C EMU2_DRIVE_D EMU2_CWD EMU2_DEFAULT_DRIVE

dos_acc() {
    "$emu" "$src/acc.exe" -toolroot='D:\' "$@"
}

# The modules of the DOS build, in link order (as in the Makefile).
modules=$(sed -n 's/^cc .* \([a-z]*\)\.c$/\1/p' build.bat)

# 1. Same plan from both drivers (guest paths make them comparable).
for module in $modules; do
    # shellcheck disable=SC2086
    ./acc -toolroot="$root" -### -target=dos2 $flags -c "$module.c" \
        -o "$self/$module.o" > "$self/unix.plan" 2>&1
    # shellcheck disable=SC2086
    dos_acc -### -target=dos2 $flags -c "$module.c" -o "$self/$module.o" 2>&1 |
        tr -d '\r' > "$self/dos.plan"
    # Equal failures would also compare equal: require a real plan.
    grep -q '^tool: ' "$self/unix.plan" ||
        { cat "$self/unix.plan" >&2; exit 1; }
    if ! cmp -s "$self/unix.plan" "$self/dos.plan"; then
        printf 'SELF-HOST plan differs for %s.c\n' "$module" >&2
        diff -u "$self/unix.plan" "$self/dos.plan" >&2 || true
        exit 1
    fi
done

# 2. The DOS driver rebuilds itself; the link list keeps its command short.
: > "$self/acc.inp"
for module in $modules; do
    # shellcheck disable=SC2086
    dos_acc -target=dos2 $flags -c "$module.c" -o "$self/$module.o" > "$self/build.log" 2>&1 ||
        { cat "$self/build.log" >&2; exit 1; }
    printf '%s\r\n' "$self/$module.o" >> "$self/acc.inp"
done
dos_acc -target=dos2 "$self/acc.inp" -o "$self/acc.exe" > "$self/build.log" 2>&1 ||
    { cat "$self/build.log" >&2; exit 1; }

# 3. Fixed point.
if ! cmp -s acc.exe "$self/acc.exe"; then
    printf 'SELF-HOST acc.exe rebuilt by acc.exe differs from acc.exe\n' >&2
    exit 1
fi
printf 'ACC SELF-HOST CHECK PASS: %s modules, identical acc.exe\n' \
    "$(printf '%s\n' $modules | wc -l | tr -d ' ')"
