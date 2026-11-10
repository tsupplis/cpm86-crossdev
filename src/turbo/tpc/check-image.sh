#!/bin/sh
# Fail if a patch changes any byte outside its reviewed IDE replacement sites.
set -eu
variant=$1
original=$2
patched=$3
work=$4
mkdir -p "$work"

case "$variant" in
    dos200)
        size=36480
        bias=-256
        hooks='0x28e4 0x29fe 0x2d0e 0x356f'
        start=$((0x39a9))
        end=$((0x3e00))
        allowed=''
        ;;
    dos302)
        size=39731
        bias=-256
        hooks='0x2e29 0x2f64 0x3478 0x3a96'
        start=$((0x40cd))
        end=$((0x5000))
        allowed=''
        ;;
    dms301)
        size=38144
        bias=-256
        hooks='0x2b30 0x2c70 0x3163 0x3781'
        start=$((0x3db8))
        end=$((0x4ceb))
        allowed="$((0x2ae9 + bias)):$((0x2aef + bias))"
        ;;
    c86301)
        size=37760
        bias=128
        hooks='0x285b 0x299b 0x2cb4 0x3485'
        start=$((0x398a))
        end=$((0x4900))
        allowed="$((0x280e + bias)):$((0x2817 + bias))"
        ;;
    *) echo "Unknown image variant: $variant" >&2; exit 1 ;;
esac
first_hook=1
failure_target=''
for address in $hooks; do
    offset=$((address + bias))
    allowed="$allowed $offset:$((offset + 3))"
done
allowed="$allowed $((start + bias)):$((end + bias))"
test "$(wc -c < "$original")" -eq "$size"
test "$(wc -c < "$patched")" -eq "$size"
status=0
cmp -l "$original" "$patched" > "$work/diff" || status=$?
test "$status" -le 1
test -s "$work/diff"
awk -v allowed="$allowed" '
    BEGIN {
        count = split(allowed, ranges, " ")
        for (i = 1; i <= count; i++) {
            split(ranges[i], ends, ":")
            first[i] = ends[1]; last[i] = ends[2]
        }
    }
    {
        offset = $1 - 1
        valid = 0
        for (i = 1; i <= count; i++)
            if (offset >= first[i] && offset < last[i]) valid = 1
        if (!valid) {
            printf "Unexpected change at file offset %d\n", offset > "/dev/stderr"
            failed = 1
        }
    }
    END { exit failed }
' "$work/diff"

for address in $hooks; do
    offset=$((address + bias))
    bytes=$(od -An -tu1 -j "$offset" -N 3 "$patched")
    set -- $bytes
    test "$1" -eq 233
    displacement=$(($2 + 256 * $3))
    target=$(((address + 3 + displacement) % 65536))
    if test "$target" -lt "$start" || test "$target" -ge "$end"; then
        echo "Hook $address jumps outside the CLI replacement window" >&2
        exit 1
    fi
    if test "$first_hook" -eq 1; then
        test "$target" -eq "$start"
        first_hook=0
    elif test -z "$failure_target"; then
        failure_target=$target
    else
        test "$target" -eq "$failure_target"
    fi
done
echo "Turbo $variant image: unchanged size; all edits confined to reviewed sites"
