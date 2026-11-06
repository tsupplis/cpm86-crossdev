# acc

Aztec C 4.2 compiler driver. Compiles, assembles and links for CP/M-86
and DOS. Runs on Unix (Aztec tools under emu2) and on DOS 2.x.

```
acc [options] inputs
acc -help | -version
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
| `-ansi` | strict ANSI: also disables `near` `far` `huge` `pascal` `fortran` `cdecl` (breaks `dos.h`) | |
| `-Osize` `-Ospeed` | code preference | |
| `-w` | no compiler warnings | |
| `-g` | sdb information; writes `<output>.dbg` beside the program | link: `.cmd` and `.exe` |
| `-Wc,opt` `-Wa,opt` `-Wl,opt` | one allow-listed `cc` / `as` / `ln` option, see below | |
| `-nostartfiles` | omit startup object | |
| `-nostdlib` | omit startup and C runtime | |
| `-overlay-root=CODE,DATA` | link an overlay root; writes `.cmd` and `.rsm` | hex sizes; needs `-o` |
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
| Language | ANSI C (cc native mode); K&R (`cc -3`) is not available |
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

## Predefined macros

| Macro | Defined when |
| --- | --- |
| `__CPM86__=1` | `-target=cpm86` |
| `__MSDOS__=1` | `-target=dos1` or `dos2` |
| `__STDC__` `__FILE__` `__LINE__` `__DATE__` `__TIME__` `__FUNC__` `MPU8086` | always (compiler) |

With `-target`, a user `-D` of either OS macro must agree with it.

## Constraints

| Rule | Reason |
| --- | --- |
| Non-small model: `dos2` `.exe` only | only DOS 2 `.exe` programs can use large models |
| Explicit non-`dos2` `-target` with a non-small model | rejected at once, even with `-c` |
| Overlays are CP/M-86 only | not yet supported for DOS |
| `-mfloat=8087-fast` rejected | not runtime-qualified |
| Explicit `.asm` with `-march=i80286` or `-mfloat=8087` rejected | not qualified |
| `-D` for `.asm`: decimal constant values only | `as -Dsym=const` |

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

## Workspace

Every invocation has one private workspace, mapped as `E:` for the tools.
All intermediates are named `E:\uNNNN.ext` from one counter, so names are
unique within and across invocations. The workspace is removed on success,
failure and interruption unless `-save-temps` is given.

## Plan output (`-###`, `-v`)

On stderr, one record per line, words double-quoted:

| Line | Content |
| --- | --- |
| `acc: plan stage=.. target=.. model=.. cpu=..` | resolved settings |
| `acc: plan float=.. language=.. optimize=..` | resolved settings |
| `tool: "PROGRAM" "ARG" ...` | one per backend command |
| `response: "FILE" "LINE" ...` | linker response file |
| `publish: "PRODUCT" -> "OUTPUT"` | one per published output |

Guest drives: `C:` invocation directory, `D:` tool root, `E:` workspace.
