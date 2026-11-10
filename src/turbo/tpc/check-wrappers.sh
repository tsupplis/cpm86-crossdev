#!/bin/sh
# Check wrapper dispatch without starting the IDE, then compile through emu2.
set -eu
root=$1
work=$2
mkdir -p "$work/bin"
cat > "$work/bin/emu2" <<'EOF'
#!/bin/sh
set -eu
test "$1" = "$EMU2_DRIVE_D/$EXPECTED_PROGRAM"
test "$EMU2_PROGNAME" = "D:$EXPECTED_PROGRAM"
shift
if test "$EXPECTED_PROGRAM" = turbo.com || test "$EXPECTED_PROGRAM" = turbo.cmd; then
    test "$#" -eq 2
else
    test "$#" -eq 5
    test "$1" = -cc
    test "$2" = 'source name.pas'
    test "$3" = ''
    shift 3
fi
test "$1" = --
test "$2" = 'PATH=D:\'
test "${EMU2_APPEND:-${EMU2_CPM_APPEND:-}}" = D:
exit "$EXPECTED_STATUS"
EOF
chmod +x "$work/bin/emu2"
for wrapper in pcdev_tp2 pcdev_tp3 cpm86_tp3; do
    case "$wrapper" in
        cpm86_tp3) extension=cmd; directory=tp3cpm86 ;;
        pcdev_tp2) extension=com; directory=tp2dos ;;
        pcdev_tp3) extension=com; directory=tp3dos ;;
    esac
    cp "$root/bin/$wrapper" "$work/bin/$wrapper"
    export EXPECTED_PROGRAM EXPECTED_STATUS
    EXPECTED_PROGRAM=turbo.$extension
    EXPECTED_STATUS=0
    sh "$work/bin/$wrapper" > "$work/$wrapper-ide.log" 2>&1
    EXPECTED_PROGRAM=tpc.$extension
    EXPECTED_STATUS=7
    status=0
    sh "$work/bin/$wrapper" -cc 'source name.pas' '' > "$work/$wrapper-dispatch.log" 2>&1 || status=$?
    test "$status" -eq 7
    grep -q "$directory" "$root/bin/$wrapper"
    mkdir -p "$work/$wrapper"
    (
        cd "$work/$wrapper"
        "$root/bin/$wrapper" -h > help.log 2>&1
        grep -q '^Turbo Pascal ' help.log
        printf "program hello;\r\nbegin end.\r\n" > hello.pas
        rm -f "hello.$extension"
        "$root/bin/$wrapper" -cc hello.pas > compile.log 2>&1
        if grep -Eq '^TPC: (compilation failed|invalid arguments)' compile.log; then
            cat compile.log
            exit 1
        fi
        test -s "hello.$extension"
    )
done
echo "Turbo wrappers: IDE/CLI dispatch, arguments, status and installed compilation passed"
echo "Outputs: $work"
