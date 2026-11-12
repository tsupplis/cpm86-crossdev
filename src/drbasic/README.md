# Personal BASIC 1.2N

A small appended-code patch for the supplied CP/M-86 Personal BASIC 1.2
interpreter, built from `basic12.cmd`.

```
BASIC
BASIC HELLO.BAS
BASIC HELLO.BAS [R
BASIC HELLO.BAS [R]
```

A filename loads the program through BASIC's `OLD` command. Either run
option selects `RUN "filename"`, which loads and runs the program. Options
are case-insensitive. BASIC supplies its usual load diagnostics and refuses
to run a program whose source fails to load. With `[R]`, normal completion through `END` or the last program line exits
BASIC through its existing SYSTEM cleanup routine, closing open files.
Load failures, an empty program, STOP and errors that interrupt execution
return to the interpreter. Subsequent manual runs stay interactive. Warnings
that allow execution to continue retain the normal completion behavior.
Errors do not set a batch-process exit status.

Use a single CP/M filename, optionally prefixed by a drive, with no spaces,
quotes or wildcard characters. BASIC supplies the default `.BAS` extension.
The argument parser bounds filenames to 14 characters and rejects extra
arguments. Detailed filename and file-format validation remains with BASIC.
No arguments opens the normal interactive interpreter. The startup banner is
hidden for `[R]` runs and shown for interactive and load-only startup.

## Build and check

Requires Python 3 and NASM. Tests additionally require `emu2`.

```
make
make check
```

The output is `basic.cmd` in this directory. `EMU` can select the emulator
used by the checks. The build checks the original binary's SHA-256 before
patching. Source files use 8.3-compatible names.

## Patch layout

The code group grows from 48,992 to 49,488 bytes, including 16-byte alignment.
Its image length and minimum allocation are updated together. Data and extra
group descriptors retain their original allocations.

Six code hooks use offsets relative to the code group:

- `00C2h`, three bytes, initializes the startup command from the runtime's
  preserved command tail and performs the displaced instruction.
- `024Fh`, five bytes, supplies the command through the blocking console
  reader. Status polling and break checks retain their original paths.

- `2F37h`, four bytes, selects cleanup after END in startup run mode.
- `2E37h`, three bytes, selects cleanup after the last program line.
- `2B40h`, four bytes, clears startup run mode when dispatching later
  interactive commands, including commands entered at the break prompt.

- `2AECh`, four bytes, skips the startup banner formatter for `[R]` runs.

The appended code begins at `BF60h`. It stores its private command and cursor
in the code group and disables injection before returning the final carriage
return. Program `INPUT` and subsequent interpreter commands then read the
normal console. The fallback executes the displaced comparison and rejoins
at `0254h`.

The banner changes to `1.2N` by consuming one preceding padding space,
keeping the string and data group lengths fixed. All original code bytes
outside the six hook sites remain identical.

Tests execute the built interpreter under `emu2` and cover load-only startup,
both run-option forms, missing and malformed files, invalid arguments,
default extensions, drive prefixes, normal interactive startup, and a program
that requests keyboard input. Completion tests run with no SYSTEM input and
verify END, fall-through, early END, and closure of a buffered output file.
STOP/error recovery tests also verify that subsequent manual runs remain
interactive. Structural checks verify group placement and
the exact scope of the patch. Real CP/M-86 kernel validation remains pending.
