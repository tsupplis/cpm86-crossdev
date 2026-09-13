# CP/M-86 DR PL/I-86 Toolchain — Command Reference

Quick reference for the DR PL/I-86 tools available via `../../bin/`.
All examples use the exact binary names as they appear in the project Makefiles.

---

## 1. DR PL/I-86 Compiler — `drpli_pc`

Three-pass optimizing PL/I compiler (based on ANSI X3.74 PL/I General Purpose Subset G). Reads a `.pli` source file and emits an Intel OMF-86 relocatable object file (`.obj`).

`drpli_pc` executes the CP/M-86 compiler `pli.cmd` and its overlays (`pli0.ovr`, `pli1.ovr`, `pli2.ovr`) staged in `share/plicpm/` under `emu2`.

**Syntax**
```
drpli_pc <filespec>[.pli] ['$'$<options>]
```

You need not specify the `.pli` extension; the compiler assumes `.pli` by default.

> **Note:** In Makefiles and shell commands, the `$` prefix for options must be quoted or escaped: `drpli_pc foo '$$l'` or in Makefiles `drpli_pc foo '$$$$l'`.

**Compiler Options** (follow `$`, maximum 7 single-letter options)

| Option | Name | Action Enabled |
|--------|------|----------------|
| `A` | Abbreviated listing | Disables listing of parameters and `%INCLUDE` statements during pass 1 |
| `B` | Built-in subroutine trace | Shows Run-time Subroutine Library (`plilib.l86`) functions called by the program |
| `D` | Disk print | Sends program listing to disk as a `<file>.prn` file |
| `I` | Interlist | Decodes machine code produced by the compiler into pseudo-assembly in the listing (automatically sets `L`) |
| `K` | Abbreviated listing | Same as `A` |
| `L` | List source | Generates a line-numbered source listing with machine code locations |
| `N` | Nesting level display | Shows nesting depth of `DO`, `PROCEDURE`, and `BEGIN` blocks during pass 1 |
| `O` | Object code off | Disables generation of `.obj` object file (syntax/listing check only) |
| `P` | Page mode print | Inserts form-feed every 60 lines and sends listing to printer |
| `S` | Symbol table display | Shows variable names, assigned types, and defaulted attributes |

The default mode (no options specified) compiles the program, produces no listing file, outputs `<file>.obj`, and displays progress and error messages on the console.

**Output Files**

| File | Contents |
|------|----------|
| `<file>.obj` | Intel OMF-86 relocatable object file |
| `<file>.prn` | Listing file on disk (when `$D` or `$L` option enabled) |
| `<file>.sym` | Symbol table output (when `$S` option enabled) |

**Typical Invocations**

```sh
# Basic compilation: foo.pli -> foo.obj
drpli_pc foo

# Generate source listing and disk PRN file
drpli_pc foo '$ld'

# Generate interlisted assembly and display nesting levels
drpli_pc foo '$in'

# Syntax and nesting check only (no object output)
drpli_pc foo '$on'
```

Makefile pattern (from `examples/Makefile`):
```makefile
PLI = drpli_pc

hellopli.obj: hellopli.pli
	$(PLI) hellopli
```

---

## 2. DR PL/I-86 Linker — `drpli_link`

Linker. Combines PL/I-86 `.obj` modules with the PL/I runtime subroutine library (`plilib.l86`) into a CP/M-86 `.cmd` executable.

`drpli_link` is a wrapper around `linkcmd.exe` (LINK-86) with drive `D:` mapped to `share/plicpm/` so that the PL/I runtime library and DIO definitions are located automatically.
**The link syntax is identical to [`pcdev_linkcmd`](../asm/asm-ref.md#2-link-86----pcdev_linkcmd)** — refer to that section for the full option reference (`[map]`, `[search]`, `[$sz]`, etc.).

**Syntax**
```
drpli_link '<outfile>.cmd[=obj1[,obj2,...,lib.l86[search]]] [options]'
```

The entire argument **must be single-quoted** in the shell / Makefile so that `$` and brackets are passed through literally.

**D: drive contents**

The following runtime files are staged in `share/plicpm/` and visible as `D:` inside the linker:

*Runtime library*

| File | Description |
|------|-------------|
| `plilib.l86` | Standard PL/I-86 runtime subroutine library (I/O, math, string, memory management) |

*DIO (Direct I/O) declarations and support files*

| File | Contents |
|------|----------|
| `dio86.dcl` | PL/I `%INCLUDE` declarations for Direct I/O subsystem |
| `diomod.dcl` | PL/I declarations for DIO module interfaces |
| `fcb.dcl` | File Control Block structures for CP/M filesystem operations |
| `record.dcl` | Record-oriented I/O declaration definitions |
| `cpmdio.a86` | Assembly language support source for Direct I/O |

**Output Files**

| File | Contents |
|------|----------|
| `<out>.cmd` | CP/M-86 executable command file |
| `<out>.sym` | Symbol table file (optional) |
| `<out>.map` | Memory and segment map file (when `[map]` option is passed) |

**Typical Invocations**

```sh
# Basic link: links foo.obj with D:plilib.l86 automatically
drpli_link 'foo.cmd=foo'

# Multiple object modules
drpli_link 'app.cmd=main,sub1,sub2'

# Explicit runtime search specification
drpli_link 'foo.cmd=foo,D:plilib.l86[search]'

# Link with segment map generation
drpli_link 'foo.cmd=foo [map]'
```

In Makefiles, remember that `$` inside single quotes must be written `$$`:
```makefile
PLI     = drpli_pc
PLILINK = drpli_link

hellopli.obj: hellopli.pli
	$(PLI) hellopli

hellopli.cmd: hellopli.obj
	$(PLILINK) 'hellopli.cmd=hellopli'
```

> **Note:** Because `drpli_link` maps `share/plicpm/` to drive `D:` and passes `LIB=D:\`, LINK-86 searches `D:plilib.l86` automatically. Specifying `drpli_link 'foo.cmd=foo'` is sufficient in the standard build flow.
