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
    dos-editor200)
        hook=$((0x28e4 - 0x100))
        protected_sites='0x391f 0x2a4c 0x2b1f 0x2dfd 0x02fc'
        bias=-256
        ;;
    c86-editor301)
        hook=$((0x285b + 0x80))
        protected_sites='0x38f2 0x29fa 0x2c89 0x2da0 0x9200'
        bias=128
        ;;
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
count=3
case "$variant" in
    *-editor*)
        cp "$patched" "$work/target.bin"
        printf '\000\000' | dd of="$work/target.bin" bs=1 seek=$((hook + 1)) conv=notrunc 2>/dev/null
        reject target
        count=$((count + 1))
        for address in $protected_sites; do
            offset=$((address + bias))
            byte=$(od -An -tu1 -j "$offset" -N 1 "$patched")
            # Always change the byte, including an original FF.
            if test "$byte" -eq 255; then value='\000'; else value='\377'; fi
            cp "$patched" "$work/boundary-$address.bin"
            printf "$value" | dd of="$work/boundary-$address.bin" bs=1 seek="$offset" conv=notrunc 2>/dev/null
            reject "boundary-$address"
            count=$((count + 1))
        done
        ;;
esac
echo "Turbo $variant image guard: $count corruption cases rejected"
