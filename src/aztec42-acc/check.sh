#!/bin/sh
set -eu

output=$("$@" -help)
printf '%s\n' "$output" | tr -d '\r' | grep -qx 'Usage: acc -help | -version'
printf '%s\n' "$output" | tr -d '\r' |
    grep -qx 'Compilation and linking are not implemented yet.'

output=$("$@" -version)
printf '%s\n' "$output" | tr -d '\r' |
    grep -qx 'acc 0.1-dev (driver foundation)'

expect_failure() {
    status=0
    output=$("$@" 2>&1) || status=$?
    if [ "$status" -ne 1 ]; then
        printf 'Expected status 1, got %s\n%s\n' "$status" "$output" >&2
        exit 1
    fi
    printf '%s\n' "$output" | tr -d '\r' | grep -q '^acc: '
}

expect_failure "$@"
expect_failure "$@" -unknown
expect_failure "$@" --help
expect_failure "$@" -help extra
expect_failure "$@" hello.c

printf 'ACC CHECK PASS: %s\n' "$*"
