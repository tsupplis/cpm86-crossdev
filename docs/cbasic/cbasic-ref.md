# CP/M-86 CBASIC Compiler (CB86) Toolchain — Command Reference

Quick reference for the CB86 tools available via `../../bin/`.
All examples use the exact binary names as they appear in the project Makefiles.

For the linker option syntax, see also [asm-ref.md — LINK-86](../asm/asm-ref.md#2-link-86----pcdev_linkcmd);
`drcbcpm_link` is the same linker with the CB86 runtime library on `D:`.

---

## 1. CBASIC Compiler — `drcbcpm_bc`

CBASIC source compiler. Reads `.bas` (or any) source file, emits Intel OMF-86
object code.

**Syntax**
```
drcbcpm_bc <file>[.bas] [toggles]
```

Source files default to `.bas` filetype. One source file per invocation.
The object file is named after the source file: `foo.bas` → `foo.obj`.

Toggles are placed in brackets after the filename:
```
drcbcpm_bc TEST [B,P,S]
```

Multiple bracket groups are allowed; toggles are evaluated left to right (last
toggle wins on conflict):
```
drcbcpm_bc TEST [B] [P]
```

**Toggles** (Table 2-1 from the CB86 Programmer's Guide)

| Toggle | Description |
|--------|-------------|
| `B` | Suppress source listing on console (errors and statistics still show) |
| `C(d)` | Set default drive for `%INCLUDE` files to drive `d` |
| `F` | Send source listing to disk file (`<file>.lst`) on same drive as source |
| `I` | Interlist compiler-generated 8086 code with source statements |
| `L(n)` | Set page length to `n` lines for printer listing |
| `N` | Generate line-number tracking code (enables `ERRL` at runtime) |
| `O` | Suppress `.obj` output file |
| `P` | Send source listing to printer |
| `R(d)` | Write `.obj` file to drive `d` |
| `S` | Include symbol names in `.obj` (enables `.sym` file from linker) |
| `T` | List symbol table after source listing |
| `U` | Generate error for undeclared variable names |
| `V` | Place source line numbers in `.sym` file |
| `W(n)` | Set page width to `n` columns for printer listing (default 80) |
| `X(d)` | Place temporary work files on drive `d` |

Toggles `I`, `N`, and `V` can also be controlled from within source code using
the `%DEBUG` directive:
```
%DEBUG I
%DEBUG -I
%DEBUG INV
```

**Source code directives** (begin with `%` — no blanks between `%` and keyword)

| Directive | Description |
|-----------|-------------|
| `%NOLIST` | Suppress listing from this point |
| `%LIST` | Resume listing |
| `%EJECT` | Page break in printer listing |
| `%PAGE n` | Set page length to `n` lines |
| `%INCLUDE <file>` | Include another source file (nestable 6 deep) |
| `%DEBUG [±I][±N][±V]` | Enable/disable I, N, V toggles inline |

**Output files**

| File | Contents |
|------|----------|
| `<file>.obj` | Intel OMF-86 relocatable object |
| `<file>.lst` | Source listing (requires `F` or `P` toggle) |
| `<file>.$PA`, `<file>.$QC`, `<file>.$DA` | Temporary work files (deleted on success) |

**Typical invocations**
```sh
# Basic compile — object named after source
drcbcpm_bc foo.bas

# Explicit filetype override
drcbcpm_bc foo.cb

# Suppress listing, include symbols for debugger
drcbcpm_bc foo.bas [B,S]

# Listing to disk file, undeclared variable warnings
drcbcpm_bc foo.bas [F,U]

# Interlist generated code with source
drcbcpm_bc foo.bas [I]
```

Makefile explicit rule (from `examples/Makefile`):
```makefile
BC     = drcbcpm_bc
CBLINK = drcbcpm_link

hellor.cmd: hellor.cb
	$(BC) hellor.cb
	$(CBLINK) hellor
```

> **Note:** All four compiler files (`cb86.exe`, `cb86.or1`, `cb86.or2`,
> `cb86.or3`) must be on the same drive (`D:` — set automatically by the wrapper).
> The source file can be on any drive.

---

## 2. CBASIC Linker — `drcbcpm_link`

Linker. Combines CB86 `.obj` files with the CB86 runtime library into a
CP/M-86 `.cmd` executable.

`drcbcpm_link` is the standard LINK-86 linker with `D:` mapped to
`share/cb86cpm/` so the CB86 runtime library is found automatically.
**The link syntax is identical to [`pcdev_linkcmd`](../asm/asm-ref.md#2-link-86----pcdev_linkcmd)**
— refer to that section for the full option reference (`[map]`, `[search]`,
`[$sz]`, I/O redirection, etc.).

**Syntax**
```
drcbcpm_link '<outfile>[=obj1[,obj2,...]] [options]'
```

The entire argument **must be single-quoted** in the shell / Makefile so that
`$` and brackets are passed through literally.
If `outfile` is omitted the name is taken from the first input file.

**D: drive contents**

| File | Description |
|------|-------------|
| `cb86.l86` | CB86 runtime library (I/O, memory, string, math routines) |
| `cb86.exe` | CB86 compiler executable |
| `cb86.or1`, `cb86.or2`, `cb86.or3` | CB86 compiler overlays |
| `lib86.exe` | LIB-86 library manager |
| `rasm86.exe` | RASM-86 assembler |

> **Note:** LINK-86 searches `cb86.l86` automatically when it finds the
> auto-library record in a CB86-compiled `.obj`. The default
> `drcbcpm_link 'foo'` is sufficient — no explicit library specification needed.

**Output files**

| File | Contents |
|------|----------|
| `<out>.cmd` | CP/M-86 executable |
| `<out>.sym` | Symbol table (optional) |
| `<out>.map` | Segment map (requires `[map]`) |

**Typical invocations**
```sh
# Default: auto-library record selects CB86 runtime automatically
drcbcpm_link 'foo'

# Explicit output name
drcbcpm_link 'foo.cmd=foo'

# Multiple objects
drcbcpm_link 'app=main,util'

# With segment map
drcbcpm_link 'foo [map]'

# Explicit library (when not relying on auto-library record)
drcbcpm_link 'foo=foo,D:cb86.l86[search]'
```

In Makefiles the `$` inside single-quoted strings must be `$$`:
```makefile
BC     = drcbcpm_bc
CBLINK = drcbcpm_link

hellor.obj: hellor.cb
	$(BC) $<

hellor.cmd: hellor.obj
	$(CBLINK) hellor
```
