#!/bin/sh
# Demonstrate that the image guard rejects runtime, hook and size corruption.
set -eu
guard=$1
variant=$2
original=$3
patched=$4
work=$5
mkdir -p "$work"
case "$variant" in
    dos200) hook=$((0x28e4 - 0x100)) ;;
    dos302) hook=$((0x2e29 - 0x100)) ;;
    dms301) hook=$((0x2b30 - 0x100)) ;;
    c86301) hook=$((0x285b + 0x80)) ;;
    *) echo "Unknown image variant: $variant" >&2; exit 1 ;;
esac
reject()
{
    if sh "$guard" "$variant" "$original" "$work/$1.bin" "$work/$1" > "$work/$1.log" 2>&1; then
        echo "Image guard accepted $variant $1 corruption" >&2
        exit 1
    fi
}
cp "$patched" "$work/protected.bin"
printf '\377' | dd of="$work/protected.bin" bs=1 seek=0 conv=notrunc 2>/dev/null
reject protected
cp "$patched" "$work/hook.bin"
printf '\000' | dd of="$work/hook.bin" bs=1 seek="$hook" conv=notrunc 2>/dev/null
reject hook
cp "$patched" "$work/size.bin"
printf '\000' >> "$work/size.bin"
reject size
echo "Turbo $variant image guard: 3 corruption cases rejected"
