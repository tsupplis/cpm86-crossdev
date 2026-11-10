# Turbo Pascal compiler and editor patches

The originals are unchanged. Compiler patches provide compile-and-exit
invocation; separate editor patches retain the native editor.

| Platform | Original | Patch source | Compiler |
|---|---|---|---|
| DOS, Turbo Pascal 2.00B | `../cdos-2.0/turbo.com` | `dospc200.asm` | `dospc200.com` |
| DOS, Turbo Pascal 3.02A | `../dos-3.0/turbo.com` | `dospc302.asm` | `dospc302.com` |
| DOS terminal variant, Turbo Pascal 3.01A | `../dosms-3.0/turbo.com` | `dmspc301.asm` | `dmspc301.com` |
| CP/M-86, Turbo Pascal 3.01A | `../cpm86-3.0/turbo.cmd` | `c86pc301.asm` | `c86pc301.cmd` |

```sh
make
make check
../../../bin/emu2 dospc200.com -cc example.pas
../../../bin/emu2 dospc302.com -cc example.pas
../../../bin/emu2 dospc302.com -ch -sc 0100 -sd 0100 -mi 0400 -ma A000 example.pas
../../../bin/emu2 dmspc301.com -cc example.pas
../../../bin/emu2 c86pc301.cmd -cc example.pas
../../../bin/emu2 c86pc301.cmd -ch -sc 0100 -sd 0100 -mi 0400 -ma A000 example.pas
```

Installed wrappers `pcdev_tp2`, `pcdev_tp3` and `cpm86_tp3` open the
original IDE without arguments and invoke `tpc.com`/`tpc.cmd` in their
respective share directories otherwise. `make check-wrappers` checks dispatch,
argument/status forwarding and compilation using the installed compilers.
Run `make install` first when those compilers are not installed.

## Standalone editors

The editor variants are built from the **original IDE images**, never the
TPC compiler outputs: TPC reuses editor code space and cannot supply an editor.
Editor patches preserve the original editor and installer configuration.
They accept one filename, enter editing directly, and exit to the OS instead
of returning to the IDE menu.

| Platform | Original | Patch source | Editor |
|---|---|---|---|
| DOS 2.00B | `../cdos-2.0/turbo.com` | `dospe200.asm` | `dospe200.com` |
| CP/M-86 3.01A | `../cpm86-3.0/turbo.cmd` | `c86pe301.asm` | `c86pe301.cmd` |

```sh
make editors
../../../bin/emu2 dospe200.com example.pas
../../../bin/emu2 c86pe301.cmd example.pas
```

`DOS_EDITOR200_INPUT`/`DOS_EDITOR200_OUTPUT` and
`C86_EDITOR301_INPUT`/`C86_EDITOR301_OUTPUT` override their paths; exact-input
hash guards remain in effect. Existing compiler wrappers are unchanged:
they still select the original IDE with no arguments, or TPC with arguments.
The editor variants are separate executables, not additional compiler modes.

| Editor behavior | Contract |
|---|---|
| Argument | Exactly one literal 8.3 filename; no default extension |
| Drive | Optional `A:`-`O:` on DOS, `A:`-`P:` on CP/M-86 |
| Paths/wildcards | Not accepted |
| Missing file | Named empty buffer; no disk file until saved |
| Exit | Original `Ctrl-K`, then `D`; modified buffers prompt Save (Y/N) |
| Save/backup | Original routines and `.BAK` behavior |
| Failed save | Original error/ESC recovery, then return to the dirty buffer |
| File format | Original text/EOF conventions; no newline normalization added |

`make check-editors` runs 21 real-console cases per editor and the image
guards. Tests require Python 3 (standard-library PTY support) and an unprivileged
POSIX host for the write-protection case. Coverage includes 60 KB editing,
oversized-file rejection, backups, discard and failed-save buffer retention.
Each image guard rejects nine deliberate corruptions, including editor,
helper-boundary, configuration/data, hook and length changes.

Native DOS/CP/M execution, actual disk-full media, terminal behavior and a
TINST installation round-trip require manual validation. Configuration bytes
are preserved, but that alone does not prove installer compatibility.
Build from the supplied pristine image before applying TINST; customized
inputs are intentionally rejected by the build hash guard.

## Compiler command line

| Option | Meaning |
|---|---|
| `-cc` | Generate COM on DOS or CMD on CP/M-86 (default); do not execute |
| `-ch` | Generate CHN |
| `-sc HEX` | Minimum code segment paragraphs (version-specific limits below) |
| `-sd HEX` | Minimum data segment paragraphs (version-specific limits below) |
| `-mi HEX` | Minimum free memory paragraphs; default `0400` |
| `-ma HEX` | Maximum free memory paragraphs; default `A000` |
| `-h` | Help |

Hexadecimal values have 1-4 digits. Code/data sizes exclude the runtime, as
in the IDE; `0000` selects the size required by the compiled program.
Minimum free memory must not exceed maximum free memory.

| Variant | Maximum `-sc` | Maximum `-sd` |
|---|---|---|
| `dos200` (2.00B) | `0D7D` | `0FE7` |
| `dos302` | `0D2B` | `0FDB` |
| `dms301` | `0D5A` | `0FDE` |
| `c86301` | `0D88` | `0FD3` |

DOS accepts DOS paths. CP/M accepts an optional `A:`-`P:` drive and an 8.3
filename, not directories or wildcards. `.PAS` is added when omitted.
CP/M switches are case-insensitive.

DOS 3.02 exit codes: 0 success, 1 compiler/I/O error, 2 usage error.
DOS 2.00B and the MS-DOS terminal variant use `INT 20h` termination, without an exit code;
failures print `TPC: compilation failed` or `TPC: invalid arguments`.
This is not a claim that DOS 3.02 supports DOS 1.0; its original runtime
uses DOS 2.x services. DOS 1.0 execution itself has not been tested.
CP/M-86 1.x has no command return-code service; failures print
`TPC: compilation failed` or `TPC: invalid arguments`. Under emu2 these
commands return host status 0 even on compiler errors. Partial CP/M output
is deleted on compilation failure.

Every original image is checked by SHA-256 before patching. Generated
runtime bytes, installer configuration and image lengths are retained.
Only the compiler's console callback changes, not the generated runtime.
The CP/M patch retains the complete CMD header and initialized data group:

| Group | Stored length | Minimum allocation | Maximum allocation |
|---|---:|---:|---:|
| CODE, relocatable | 37264 | 37264 | unchanged |
| DATA, relocatable | 256 | 10112 | 786432 |

No extraction, group resizing or relocation changes are required.

| Target | Action |
|---|---|
| `make` | Build all four compilers and both editors |
| `make editors` | Build DOS 2.00B and CP/M-86 3.01A editors |
| `make dos-editor200` / `make c86-editor301` | Build one editor |
| `make check-editors` | Run both editor console and image-protection suites |
| `make check-editor-images` | Check both editor images and corruption rejection |
| `make dos` / `make dms` / `make c86` | Build all versions for one platform variant |
| `make dos200` / `make dos302` / `make dms301` / `make c86301` | Build one version |
| `make check` | Run all version-specific test suites |
| `make check-dos` / `make check-dms` / `make check-c86` | Run all suites for one platform variant |
| `make check-dos200` / `make check-dos302` / `make check-dms301` / `make check-c86301` | Run one version's full suite |
| `make check-cmd` | Run the CP/M-86 3.01 CLI suite only |
| `make check-images` | Check protected bytes, hook targets and image-guard rejection cases |
| `make check-com` | Compile and execute COM/CMD runtime fixtures for every version |
| `make check-chain` | Execute COM/CMD launchers chaining to CHN; verify shared data |
| `make clean` | Remove patched compilers and editors, not originals |

Inputs and outputs are independently overridable with `DOS200_INPUT`,
`DOS200_OUTPUT`, `DOS302_INPUT`, `DOS302_OUTPUT`, `DMS301_INPUT`,
`DMS301_OUTPUT`, `C86301_INPUT` and
`C86301_OUTPUT`. The exact-image SHA guards still apply to overridden inputs.
Individual test groups also have version-qualified targets, for example
`make check-dos200-images`, `make check-dos302-com` and
`make check-c86301-chain`.

Checks cover argument errors, missing/invalid source, output generation,
and execution of generated programs through file I/O. CP/M checks also
compare preserved image regions byte-for-byte, verify emitted CMD memory
descriptors for all four numeric switches, and execute a CHN via `Chain`.
The CP/M CLI suite is `check-cmd.sh`. Test filenames are lowercase.
The dedicated runtime fixtures exercise includes, procedures, heap allocation,
integer/real arithmetic and file I/O. CHN tests verify shared variable
preservation and that `Chain` does not return to its caller.
The separate `redirect.pas` fixture tests `{$G512,P512,D-}` and redirected
standard input/output only on DOS 3.02 and the MS-DOS terminal variant.
Shared fixtures omit these MS-DOS handle directives for DOS 2.00B/CP/M-86
compatibility.

### Retained test output

Every check retains generated files and logs below `tests/dos/200/`,
`tests/dos/302/`, `tests/dms/301/` or `tests/c86/301/`:

| Directory (under each OS/version directory) | Contents |
|---|---|
| `cli` | CLI sources, binaries, numbered compiler logs and runtime logs |
| `com` (DOS), `cmd` (CP/M-86) | Standalone runtime fixture, executable, result and logs |
| `chain` | Launcher, CHN, shared include, results and separate build/run logs |
| `image` | Byte-difference listing for the pristine and patched images |
| `image-negative` | Deliberately corrupted guard-test images and rejection logs |
| `editor` | Console logs, keyboard input, edited files and backups |
| `editor-image`, `editor-image-negative` | Editor differences and guard-rejection cases |

`check-chain.sh` is shared because both platforms exercise the same Pascal
`Chain` contract. The Makefile runs it separately with each OS/version compiler and
executable extension; their outputs never share a directory.

`check-image.sh` does not execute programs: it verifies unchanged image size,
unchanged bytes outside the allowed patch windows, and valid hook targets.
It protects the retained runtime, configuration and CMD header/data group.
The images under `image-negative/` are intentionally invalid; do not run them.

Checks overwrite their own named outputs on each run and remove stale runtime
markers before execution. They do not delete results afterward. `make clean`
removes the patched compilers and editors only; test results remain available.

Pascal sources and includes must use CRLF line endings; the check scripts
normalize LF or CRLF shared fixtures to CRLF when staging them. The standalone runtime fixture
also needs `input.txt` containing `17` and writes `COM=11457;REAL=10` to
`comtest.txt`; it does not print that result on the console.

See [PORTING.md](PORTING.md) for the exact patch-site map, verification limits
and procedure for another release or an 8087 variant.

TINST compatibility is intended through the preserved configuration layout;
an actual installer round-trip has not yet been verified. Use the matching
platform/version installer, with the patched compiler named `TURBO.COM` or
`TURBO.CMD` in a separate directory, not over the original input image.
The provided `tinst.com` is the DOS installer, not a CP/M executable.
