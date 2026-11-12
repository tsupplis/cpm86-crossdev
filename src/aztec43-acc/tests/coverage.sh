#!/bin/sh
# Test inventory lint: checks that the test corpus mentions
#   - every option parsed by args.c (including -o -I -D -L -l),
#   - every value of -target, -mcmodel, -march and -mfloat,
#   - every allow-listed raw backend option of raw.c,
#   - every diagnostic passed to fail(), dos_error() or io_error(),
#   - and that build.bat compiles exactly the DOS build's modules,
# except diagnostics listed in tests/coverage-exempt.txt, which must state
# why no deterministic test can trigger them.
#
# The corpus is tests/plan/*.cases plus the shell suites. This checks text
# presence only. Behavioral coverage comes from the suites themselves.  Usage: coverage.sh SOURCE_DIR
set -eu
src=$1
corpus=$(mktemp "${TMPDIR:-/tmp}/acc-coverage.XXXXXX")
trap 'rm -f "$corpus"' EXIT
cat "$src"/tests/plan/*.cases "$src"/check.sh "$src"/exec-check.sh \
    "$src"/exec-dos-check.sh > "$corpus"
exempt=$src/tests/coverage-exempt.txt
missing=0

report() {
    printf 'UNCOVERED %s: %s\n' "$1" "$2" >&2
    missing=$((missing + 1))
}

# Options: literal spellings compared in args.c, plus the short options.
for option in $(grep -o 'strn\{0,1\}cmp(arg, "[^"]*"' "$src/args.c" |
                sed 's/.*"\(.*\)"/\1/' | sort -u) -o -I -D -L -l; do
    grep -qF -- "$option" "$corpus" || report option "$option"
done

# Option values, from the profile tables.
for table in target_names model_names cpu_names float_names; do
    # From the table's opening line up to its own "};" (one-line tables too).
    values=$(awk -v t="$table" '
        index($0, t "[") && index($0, "= {") { on = 1 }
        on { print; if (index($0, "};")) exit }' "$src/profile.c" |
             grep -o '"[^"]*"' | tr -d '"')
    case $table in
        target_names) option=-target= ;;
        model_names) option=-mcmodel= ;;
        cpu_names) option=-march= ;;
        float_names) option=-mfloat= ;;
    esac
    for value in $values; do
        grep -qF -- "$option$value" "$corpus" || report value "$option$value"
    done
done

# Raw backend options: every allow-list entry, with its -W prefix.
sed -n 's/^ *{ RAW_\([A-Z]*\), "\([^"]*\)".*/\1 \2/p' "$src/raw.c" |
while read -r tool spelling; do
    case $tool in
        COMPILER) prefix=-Wc, ;;
        ASSEMBLER) prefix=-Wa, ;;
        LINKER) prefix=-Wl, ;;
    esac
    grep -qF -- "$prefix$spelling" "$corpus" ||
        { printf 'UNCOVERED raw option: %s\n' "$prefix$spelling" >&2; echo x; }
done > "$corpus.raw"
missing=$((missing + $(wc -l < "$corpus.raw")))
rm -f "$corpus.raw"

# Substrings the shell suites assert (the quoted operand of expect_error or
# grep, at least 12 characters): a diagnostic containing one is covered.
grep -hoE "(expect_error|grep -[A-Za-z]+) '[^']{12,}'" "$src"/check.sh \
    "$src"/exec-check.sh "$src"/exec-dos-check.sh |
    sed "s/^[^']*'//; s/'\$//" | sort -u > "$corpus.asserted"

# Diagnostics: every literal message, unless exempt.
grep -ho '\(fail\|dos_error\|io_error\)("[^"]*"' "$src"/*.c |
    sed 's/^[a-z_]*("//; s/"$//' | sort -u > "$corpus.messages"
while IFS= read -r message; do
    grep -qF -- "$message" "$corpus" && continue
    covered=0
    while IFS= read -r asserted; do
        case $message in *"$asserted"*) covered=1; break ;; esac
    done < "$corpus.asserted"
    [ "$covered" = 1 ] && continue
    grep -qF -- "$message	" "$exempt" && continue
    printf 'UNCOVERED diagnostic: %s\n' "$message" >&2
    echo x
done < "$corpus.messages" > "$corpus.count"
missing=$((missing + $(wc -l < "$corpus.count")))
total=$(wc -l < "$corpus.messages")
rm -f "$corpus.messages" "$corpus.count" "$corpus.asserted"

# Exemptions must still name an existing diagnostic.
while IFS='	' read -r message reason; do
    case $message in '' | '#'*) continue ;; esac
    if ! grep -qF -- "\"$message\"" "$src"/*.c; then
        printf 'STALE exemption: %s\n' "$message" >&2
        missing=$((missing + 1))
    fi
    [ -n "$reason" ] || { printf 'EXEMPTION without reason: %s\n' "$message" >&2; missing=$((missing + 1)); }
done < "$exempt"

# build.bat (native DOS bootstrap, not run by make) must compile exactly
# the DOS build's modules: every .c file except the Unix executor.
expected=$(cd "$src" && ls *.c | grep -v '^execunix\.c$' | sort | tr '\n' ' ')
actual=$(sed -n 's/^cc .* \([a-z]*\.c\)$/\1/p' "$src/build.bat" | sort | tr '\n' ' ')
if [ "$expected" != "$actual" ]; then
    printf 'STALE build.bat modules: %s (expected %s)\n' "$actual" "$expected" >&2
    missing=$((missing + 1))
fi

if [ "$missing" -ne 0 ]; then
    printf 'ACC COVERAGE CHECK FAILED: %s gaps\n' "$missing" >&2
    exit 1
fi
printf 'ACC COVERAGE CHECK PASS: %s diagnostics\n' "$total"
