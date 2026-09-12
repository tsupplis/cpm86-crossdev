# CP/M-86 DR C Toolchain — Command Reference

Quick reference for the DR C tools available via `../../bin/`.
All examples use the exact binary names as they appear in the project Makefiles.

---

## 1. DR C Compiler — `drccpm_cc`

Two-pass C compiler. Reads one `.c` source file, emits Intel OMF-86 object code.

**Syntax**
```
drccpm_cc [options] <file>.c
```

One source file per invocation. If `-o` is not given, the object is named after
the source file: `foo.c` → `foo.obj` in the current directory.

**Options**

Options are split internally between the preprocessor pass (`drc860`) and the
code-generator pass (`drc861`); the table shows which pass each option affects.

| Option | Pass | Description |
|--------|------|-------------|
| `-b` | both | Big memory model (default: small) |
| `-c` | both | Compact memory model |
| `-M` | both | Medium memory model |
| `-h` | both | Suppress sign-on banner |
| `-v[n]` | both | Verbose level 1–5 (see below) |
| `-w[n]` | both | Warning display: `0`=all, `1`=suppress warnings, `2`=suppress all |
| `-r[dev]` | both | Generate C/asm interlisting; default device: `CON` |
| `-d<name>` | drc860 | Define `<name>` as `1` (like `#define name 1`) |
| `-i<drive:>` | drc860 | Search `<drive:>` for `#include` files |
| `-l[dev]` | drc860 | Generate source listing; default device: `CON` |
| `-p` | drc860 | Preprocessor only — stop after drc860, leave `ctemp.tok` |
| `-z<drive:>` | drc860 | Place temp files on `<drive:>` |
| `-o<file>` | drc861 | Object output filename (default: `<source>.obj`) |
| `-f` | drc861 | Use 8087 math coprocessor for floating-point |
| `-j` | drc861 | Disable jump optimizer (faster compile, larger output) |
| `-n` | drc861 | Disable code optimizer (faster compile, larger output) |
| `-q<n>` | drc861 | Code generator node count (default 500, min 100) |
| `-x` | drc861 | Save/restore registers via library call (smaller, slower) |
| `-a[files]` | drc861 | Auto-invoke linker after compilation |

> **Note:** Options `-0<drive:>`, `-1<drive:>`, `-2<drive:>` (module locations for
> the `DRC.EXE` supervisor) are silently dropped — the wrapper calls `drc860` and
> `drc861` directly and does not use the supervisor.

**`-v` levels**

| Level | Output |
|-------|--------|
| `-v1` | General information messages only |
| `-v2` | `#` character per function |
| `-v3` | Function name per function |
| `-v4` | Start/end messages for `#include` files |
| `-v5` | Filename and line number per source line |

Each level activates all lower levels except `-v2` and `-v3` are mutually exclusive;
`-v4`/`-v5` activate `-v3`, not `-v2`.

**Memory models**

| Option | Code seg | Data seg | Link library |
|--------|----------|----------|--------------|
| *(none)* | ≤64 K | ≤64 K | `clears.l86` (small) |
| `-M` | >64 K | ≤64 K | `clearl.l86` (large) |
| `-c` | ≤64 K | >64 K | `clearl.l86` (large) |
| `-b` | >64 K | >64 K | `clearl.l86` (large) |

**Output files**

| File | Contents |
|------|----------|
| `<file>.obj` | Intel OMF-86 relocatable object |
| `<file>.lst` | Source listing (requires `-l`) |
| `ctemp.tok` | Preprocessor token file (temporary; deleted on success) |

**Typical invocations**
```sh
# Basic compile — object named after source
drccpm_cc foo.c

# Explicit output name
drccpm_cc -ofoo.obj foo.c

# Compact model, define a macro
drccpm_cc -c -dDEBUG foo.c

# Search B: for #include files, suppress banner
drccpm_cc -h -iB: foo.c

# Preprocessor only (stop before code generation)
drccpm_cc -p foo.c
```

Makefile pattern (from `examples/Makefile`):
```makefile
DRCCC = drccpm_cc

hellodrc.obj: hellodrc.c
	$(DRCCC) -o$@ $<
```

---

## 2. DR C Linker — `drccpm_link`

Linker. Combines DR C `.obj` files with the DR C runtime libraries into a
CP/M-86 `.cmd` executable.

`drccpm_link` is the standard LINK-86 linker with `D:` mapped to
`share/drc86cpm/` so the DR C runtime libraries are found automatically.
**The link syntax is identical to [`pcdev_linkcmd`](../asm/asm-ref.md#2-link-86----pcdev_linkcmd)**
— refer to that section for the full option reference (`[map]`, `[search]`,
`[$sz]`, I/O redirection, etc.).

**Syntax**
```
drccpm_link '<outfile>.cmd[=obj1[,obj2,...,lib.l86[search]]] [options]'
```

The entire argument **must be single-quoted** in the shell / Makefile so that
`$` and brackets are passed through literally.

**D: drive contents**

The following files are visible as `D:` inside the linker (and compiler):

*Runtime libraries*

| File | Description |
|------|-------------|
| `clears.l86` | Small-model C runtime (startup, stdio, string, math, …) |
| `clearl.l86` | Large-model C runtime (medium / compact / big models) |

*Standard headers* (for `#include` in C source)

| File | Contents |
|------|----------|
| `stdio.h` | `printf`, `scanf`, file I/O macros |
| `ctype.h` | Character classification (`isalpha`, `isdigit`, …) |
| `setjmp.h` | Non-local jump (`setjmp` / `longjmp`) |
| `errno.h` | Error codes for `perror` |
| `portab.h` | Portability macros (`WORD`, `LONG`, …) |
| `alloc.h` | Memory allocation declarations |
| `conio.h` | Console I/O (`getch`, `putch`, …) |
| `dos.h` | DOS / CP/M system call interface |
| `bios.h` | BIOS call interface |
| `float.h` | Floating-point limit macros |

*Reference source* (not linked — for inspection only)

| File | Contents |
|------|----------|
| `startup.a86` | C runtime startup routine source |
| `pcbios.a86` | PC BIOS interface source |

> **Note:** The compiler searches `D:` automatically for `#include` files
> (the wrapper sets `PATH=D:\`). No `-i` flag is needed for the standard headers.

**Output files**

| File | Contents |
|------|----------|
| `<out>.cmd` | CP/M-86 executable |
| `<out>.sym` | Symbol table (optional) |
| `<out>.map` | Segment map (requires `[map]`) |

**Typical invocations**
```sh
# Default: compiler auto-library record selects the right runtime automatically
drccpm_link 'foo.cmd=foo.obj'

# Multiple objects
drccpm_link 'app.cmd=main.obj,util.obj'

# Explicit runtime (useful when overriding the default, or linking without -a)
drccpm_link 'foo.cmd=foo.obj,D:clears.l86[search]'

# Large model (medium / compact / big) — explicit
drccpm_link 'big.cmd=big.obj,D:clearl.l86[search]'

# With segment map
drccpm_link 'foo.cmd=foo.obj [map]'
```

In Makefiles the `$` inside single-quoted strings must be `$$`:
```makefile
DRCCC   = drccpm_cc
DRCLINK = drccpm_link

hellodrc.obj: hellodrc.c
	$(DRCCC) -o$@ $<

hellodrc.cmd: hellodrc.obj
	$(DRCLINK) 'hellodrc.cmd=hellodrc.obj'
```

> **Note:** The compiler embeds an auto-library record in every `.obj` that tells
> LINK-86 which system library to search and link with `[search]` automatically.
> The default `drccpm_link 'foo.cmd=foo.obj'` is sufficient in the normal case.
> Specify `D:clears.l86[search]` (or `D:clearl.l86[search]`) explicitly only when
> mixing object files from different sources or overriding the default library selection.
