# acc

Aztec C 4.3 compiler driver. Compiles, assembles and links for CP/M-86
and DOS. Runs on Unix (Aztec tools under emu2) and on DOS 2.x.

```
acc [options] inputs
acc -help | -version
```

## Scenarios

Programs run directly under `emu2` on Unix: `emu2 hello.cmd`.

| Goal | Command |
| --- | --- |
| CP/M-86 program (`a.cmd`) | `acc hello.c` |
| DOS 2 program | `acc -o hello.exe hello.c` |
| DOS 1.1 program | `acc -target=dos1 -o hello.exe hello.c` |
| Several sources, one step | `acc -o app.cmd -Iinc -DVERSION=3 main.c util.c` |
| Compile, then link | `acc -c main.c util.c` then `acc -o app.cmd main.o util.o` |
| Your own library (`aztec43_lib util.lib util.o`) | `acc -o app.cmd main.c -Llib -lutil` |
| Math (`-lm`) | `acc -o calc.cmd calc.c -lm` |
| Large model, DOS only | `acc -mcmodel=large -o big.exe hello.c` |
| Architecture: `i8086` (default), `i80186`, `i80286`; defines `__i186__`, `__i286__` | `acc -march=i80286 -o hello.cmd hello.c` |
| Larger compiler symbol tables (`-L` locals, `-E` expressions, `-y` switch cases, `-z` strings; 1 to 32767) | `acc -Wc,-L300 -c big.c` |
| C with an assembler function | `acc -o mix.cmd mix.c addw.asm` |
| Debug information (`dbg.dbg` beside the program) | `acc -g -o dbg.exe hello.c` |
| Squeeze compiled objects with `sqz` | `acc -squeeze -o hello.cmd hello.c` |
| Overlays | see `examples/Makefile` (`ovraza.cmd`, `ovrazd.exe`) |
| Print the plan, run nothing | `acc -### -o hello.cmd hello.c` |
| Keep the temporary files | `acc -save-temps -o hello.cmd hello.c` |

```make
app.cmd: main.o util.o
	acc -o $@ main.o util.o

%.o: %.c
	acc -Iinc -c $<
```

## Modes

| Mode | Selected by | Inputs | Output |
| --- | --- | --- | --- |
| Compile | `-c` | `.c`, `.asm` (one or more) | `<base>.o` per source |
| Compile to assembly | `-S` | `.c` (one or more) | `<base>.asm` per source |
| Link | neither | `.c` `.asm` `.o` `.lib`, or one `.inp` | one `.cmd`, `.exe` or `.com` |

- `-o` names the output only with a single source; with several it is an error.
- Two sources with the same base name collide: error.
- Outputs are published only after every command succeeds; a failure leaves
  existing files untouched.
- In link mode sources are compiled first; their objects stay in the
  private workspace and are not published.

## Inputs

| Suffix | Meaning |
| --- | --- |
| `.c` | C source |
| `.asm`, `.s` | assembler source |
| `.o` | object |
| `.lib` | library file |
| `-lNAME` | `NAME.lib` from the library areas; `-lm` is the math library of the selected model and float backend |
| `.inp` | link list: files to link, see below |

Order is preserved. `-lm` before the runtime, always.

### `.inp` link list

| Rule | |
| --- | --- |
| Entries | one `.o` or `.lib` path per line, relative to the invocation directory |
| Line ends | LF or CRLF; blank lines ignored |
| Forbidden | options, comments, blanks inside a path, sources, nested lists |
| Use | link mode only; at most one; not combined with other file inputs; `-lNAME` and options are allowed |

Response files (`@file`) do not exist; options always stay on the command line.

## Options

| Option | Meaning | Values |
| --- | --- | --- |
| `-c` `-S` | stop at object / assembly | |
| `-o file` | output name | |
| `-I dir` | include area | ordered |
| `-D name[=value]` | macro; `name` alone is `name=1` | conflicting redefinition is an error |
| `-L dir` | library area | ordered |
| `-l name` | library | |
| `-target=` | OS macro when compiling; link target (else from `-o`) | `cpm86` `dos1` `dos2` |
| `-mcmodel=` | memory model | `small` `medium` `compact` `large` |
| `-march=` | CPU | `i8086` `i80186` `i80286` |
| `-mfloat=` | floating point | `soft` `auto` `8087` |
| `-Osize` `-Ospeed` | code preference | |
| `-w` | no compiler warnings | |
| `-g` | sdb information; writes `<output>.dbg` beside the program | link: `.cmd` and `.exe` |
| `-squeeze` | run `sqz` in place on every object compiled or assembled, before linking or publishing | not with `-S`; needs a compiled source |
| `-Wc,opt` `-Wa,opt` `-Wl,opt` | one allow-listed `cc` / `as` / `ln` option, see below | |
| `-nostartfiles` | omit startup object | |
| `-nostdlib` | omit startup and C runtime | |
| `-overlay-root=CODE,DATA` | link an overlay root (`.cmd` or `.exe`) and its `.rsm` | hex bytes, 1-4 digits each; needs `-o` |
| `-overlay=ROOT.rsm` | link an overlay `.ovr` | needs `-o` |
| `-toolroot=dir` | Aztec tool root | |
| `-emu=path` | emulator (Unix only) | default `emu2` from `PATH` |
| `-###` | print the plan, run nothing | |
| `-v` | print the plan, then run it | |
| `-save-temps` | keep the workspace | |

Options use one dash, are case-sensitive, and are not bundled (`-cv` is not
`-c -v`). `-o -I -D -L -l` take attached or separate operands; the others
use `=`. Repeated scalar options: last wins.

## Defaults

| Setting | Default |
| --- | --- |
| Target, compiling | none: OS-neutral, no OS macro |
| Target, linking | from the `-o` suffix, see below |
| Output | `a.cmd`; `a.exe` with `-target=dos1`/`dos2` |
| Model, CPU, float | `small`, `i8086`, `soft` |
| Language | ANSI C (cc native mode); `-ansi` (disables `far`) and K&R (`cc -3`) are not available |
| Optimisation | compiler native |
| Debug data | off |
| Error pause | never |

## Link target

| `-o` suffix | No `-target` | Allowed `-target` |
| --- | --- | --- |
| `.cmd` | `cpm86` | `cpm86` |
| `.exe` | `dos2` | `dos1`, `dos2` |
| `.com` | `dos2` | `dos1`, `dos2` |
| none (no `-o`) | `cpm86`, `a.cmd` | any; `a.exe` for dos |

Other suffixes are errors. There is no tiny model: a `.com` program is a
small-model, single-segment program.

## Floating point

| `-mfloat=` | `cc` | `-lm` links | Runs on |
| --- | --- | --- | --- |
| `soft` | none | `m` | software only |
| `auto` | none | `m87s` | startup probes for an 8087 and falls back to software |
| `8087` | `+e` | `m87` | inline 8087 code; the CP/M-86 startup exits with a message when none answers |

`-lm` is chosen by `-mfloat` and `-mcmodel`; without `-lm` no math library is linked.

A CP/M-86 `.cmd` also records the coprocessor in its header flag byte (7Fh).
`ln` leaves it clear, so `acc` sets it in the private workspace after a
successful link, before publishing:

| `-mfloat=` | Flag bits | Value | Meaning |
| --- | --- | --- | --- |
| `soft` | none | `00` | the program does not use the 8087 |
| `auto` | 5 | `20` | allocate the 8087 to the program only if present |
| `8087` | 5 and 6 | `60` | allocate the 8087 to the program even if absent |

An explicit `-lm87s` or `-lm87` sets the same bits as `auto` or `8087`, whatever
`-mfloat` says; the bits of several sources are combined.

Only link mode with a `.cmd` output changes the header; DOS programs have
none. `-###` shows it as `flags: "PRODUCT" 0xNN`. Check the result with `cmdinfo`.

## Predefined macros

| Macro | Defined when |
| --- | --- |
| `__AZTEC__=1` | always, on every `cc` and `as` command (a user `-D__AZTEC__` must be 1) |
| `__CPM86__=1` | `-target=cpm86` |
| `__MSDOS__=1` | `-target=dos1` |
| `__MSDOS__=2` | `-target=dos2` |
| `__i186__=1` | `-march=i80186` or `i80286` |
| `__i286__=1` | `-march=i80286` |
| `__STDC__` `__FILE__` `__LINE__` `__DATE__` `__TIME__` `__FUNC__` `MPU8086` | always (compiler) |

A user `-D` of an OS macro (with `-target`) or a CPU macro must agree.

### `azdefs.h`

Opt-in: `#include <azdefs.h>` (installed in the tool root's `include/`). Its content is guarded by `__AZTEC__`, so it is empty when the file is compiled without `acc`.

| Macro | Value |
| --- | --- |
| `__AZTEC_C__` `__AZTEC_C_MINOR__` `__AZTEC_C_PATCHLEVEL__` | `4` `3` `0` |
| `__AZTEC_C_VERSION__` | `40300L` (a `long`: 40300 exceeds the 16-bit `int`) |
| `__VERSION__` | `"Aztec C 4.3.0 (https://github.com/tsupplis/cpm86-crossdev)"` |
| `__i86__` | `1` (from the compiler's `MPU8086`) |
| `__CHAR_BIT__` | `8` (`char` is signed) |
| `__STDC_NO_ATOMICS__` `__STDC_NO_THREADS__` | `1` |
| `__BYTE_ORDER__` | `__ORDER_LITTLE_ENDIAN__` (`1234`; also `__ORDER_BIG_ENDIAN__` `4321`, `__ORDER_PDP_ENDIAN__` `3412`) |

## Constraints

| Rule | Reason |
| --- | --- |
| Non-small model: `dos2` `.exe` only | only DOS 2 `.exe` programs can use large models |
| Explicit non-`dos2` `-target` with a non-small model | rejected at once, even with `-c` |
| Overlays: small model; root `.cmd` (cpm86) or `.exe` (dos); not `.com` | Tech Info 2.1 |
| No `8087-fast` (unsafe `cc +ej`) | documented in the 4.10a notes, rejected by cc v4.10d (`unknown option: j`) |
| `-D` for `.asm`: decimal constant values only | `as -Dsym=const` |
| Explicit `.asm`: `-march=i80186`/`i80286` pass `as -186`; `i80286` with `-mfloat=8087` adds `as -2` (80287, no WAIT) | 8087 instructions always assemble; as v4.10d also accepts 80186 instructions without `-186` (probed) |

## Backend options (`-Wc`, `-Wa`, `-Wl`)

One option per `-W`, spelled as the tool spells it; commas do not split.
Anything not listed is rejected, as is a `-W` for a tool that does not run.

| Tool | Option | Effect | Requires |
| --- | --- | --- | --- |
| cc | `-E<n>` `-L<n>` `-y<n>` `-z<n>` | table size: expressions, locals, switch cases, strings (1..32767) | |
| cc | `-T` | C source as comments in the `.asm` | `-S` |
| cc | `-k` | no `near` `far` `huge` `pascal` `fortran` `cdecl` | |
| cc | `+U` `+R` `+A` `+M` | uninitialised globals extern; no register tracking; no-alias assertion; MUL for constants | |
| cc | `+s` | strings in the code segment | `compact` or `large` |
| as | `-S<n>` | squeeze passes | |
| as | `-C` `-CA` `-CS` | listing to the console | |
| ln | `-M` `-V` | no multiple-definition warnings; verbose | |
| ln | `-S<hex>` | minimum stack+heap bytes | DOS link |
| ln | `-X<hex>` | maximum stack+heap paragraphs | `dos2` `.exe` |

Managed by acc, never raw: `-A -B -D -I -O -S -n -ansi -3 +F +C +0..2 +L* +e* -c` (cc);
`-O -I -D -186 -2 -ZAP -L* -X` (as); `-O -L -F -T -N -B -C -D -U -R +C +D -g -q` (ln).
Foreign ABI modes (`+N +D +DF -ze`) and `+b` are not available.

## Tool root

First found: `-toolroot`, `ACCROOT`, then (Unix) the directory above the
`bin/` that contains this `acc`, provided `bin/cc.exe` exists. DOS 2 cannot
discover it: set `-toolroot` or `ACCROOT`.

## Environment

| Variable | Use |
| --- | --- |
| `ACCROOT` | tool root |
| `INCLUDE`, `CLIB` | extra include / library areas, after `-I` / `-L`, before the root's |
| `CCTEMP`, `TEMP`, `TMPDIR` | parent of the private workspace, in that order, then `/tmp` (Unix) or the working directory (DOS) |
| `PATH` | emulator lookup (Unix) |

## Exit status

| Status | Meaning |
| --- | --- |
| 0 | success |
| 1 | driver error: diagnostic on stderr |
| other | status of the first failing tool; `128+N` when killed by signal N (Unix) |

## Limits

| Limit | Value |
| --- | --- |
| Entries per list (inputs, `-I`, `-D`, `-L`) | 128 |
| Include areas | 7, plus the current directory |
| DOS path | 63 characters, 8.3 names, no aliasing |
| `cc`, `as` command | 126 bytes of arguments; longer is an error |
| `ln` command | unlimited: always passed through a response file |
| `acc.exe` command line (DOS) | 125 characters; a full 126-character DOS tail is refused as possibly truncated: use `.inp` |

## Workspace

Every invocation has one private workspace, mapped as `E:` for the tools.
All intermediates are named `E:\uNNNN.ext` from one counter, so names are
unique within and across invocations. The workspace is removed on success,
failure and interruption unless `-save-temps` is given.

### Temporary files

| Item | Behaviour |
| --- | --- |
| Parent directory | the first of `CCTEMP`, `TEMP`, `TMPDIR` (Unix only) that is set, then `/tmp` on Unix or the working directory on DOS |
| Workspace | a new directory below the parent: `acc.XXXXXX` on Unix, `A?XXXXXX` on DOS (`A` plus a letter and six characters) |
| What the tools see | `E:` is the workspace; the tools get `CCTEMP=E:\` (Unix) or `CCTEMP=<workspace>\` (DOS), so `cc`, `cgen` and `as` write their own temporaries there too |
| Cleanup | the whole workspace is deleted after every run, including files the tools leave behind; products are copied out first |
| `-save-temps` | keeps it and prints `acc: workspace retained: DIR`; for a link it holds `u0001.o`, `u0002.cmd`, `u0003.rsp` |
| `-v` | prints `acc: workspace: DIR` |
| Limit (DOS) | the parent path must leave room within 63 characters |

`CCTEMP=/tmp/sc/mytmp acc -save-temps -o st.cmd hello.c` retains the
workspace as `/tmp/sc/mytmp/acc.4OJZfB`.

### Terminal output (Unix)

The emulated linker fails (status 10) on a link of about twenty objects when
its standard output is a terminal, and succeeds when it is redirected. When
`acc`'s own standard output is a terminal, it therefore gives each backend a
file, `E:\..\stdout.txt` in the workspace, instead, and copies it to its
standard output when the backend ends. The backend's messages appear when it
finishes, not while it runs. With a pipe or a redirect nothing changes.

## Plan output (`-###`, `-v`)

On stderr, one record per line, words double-quoted:

| Line | Content |
| --- | --- |
| `acc: plan stage=.. target=.. model=.. cpu=..` | resolved settings |
| `acc: plan float=.. optimize=.. debug=..` | resolved settings |
| `tool: "PROGRAM" "ARG" ...` | one per backend command |
| `response: "FILE" "LINE" ...` | linker response file |
| `flags: "PRODUCT" 0xNN` | CP/M-86 header flag bits set in the linked product before it is published |
| `publish: "PRODUCT" -> "OUTPUT"` | one per published output |

Guest drives: `C:` invocation directory, `D:` tool root, `E:` workspace.
