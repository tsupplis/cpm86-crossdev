# CP/M-86 DR FORTRAN-77 Toolchain — Command Reference

Quick reference for the DR FORTRAN-77 tools available via `../../bin/`.
All examples use the exact binary names as they appear in the project Makefiles.

For the linker option syntax, see also [asm-ref.md — LINK-86](../asm/asm-ref.md#2-link-86----pcdev_linkcmd);
`drfcpm_link` is the same linker with the DR FORTRAN-77 runtime libraries on `D:`.

---

## 1. DR FORTRAN-77 Compiler — `drfcpm_f77`

Two-pass FORTRAN-77 compiler (DR FORTRAN-77 4.0 for CP/M-86).
Pass 1 (`f77.cmd`) is the front-end; pass 2 (`codegen.cmd`) is the code generator.
Produces an Intel OMF-86 `.obj` file ready for `drfcpm_link`.

**Syntax**
```
drfcpm_f77 [options] <file>.f77
```

One source file per invocation. The object file is named after the source file:
`foo.f77` → `foo.obj` in the current directory.

Options can also be embedded in the source with a `%` in column 1:
```fortran
%-S
%-W
      PROGRAM FOO
      ...
```

**Options**

| Option | Description | Default |
|--------|-------------|---------|
| `-A` | Abbreviate listing by omitting `%INCLUDE` files. Implies `-L`. | Include files listed |
| `-C` | Source is in card-image (column-based) format | Card-image format |
| `-D` | DO loops execute at least once (1966 ANSI). | Zero-trip DO (1977 ANSI) |
| `-F` | Source is in free-field format | Card-image format |
| `-I` | Generate source/machine-code interlisting file (`<file>.lis`) | No interlisting |
| `-L` | Send source listing to console | No listing |
| `-M2` | 2-byte (16-bit) integers | 2-byte integers |
| `-M4` | 4-byte (32-bit) integers | 2-byte integers |
| `-S` | Compile for small memory model | Large model |
| `-U` | Convert source to upper case (except string literals) | No conversion |
| `-V` | Verify array subscript bounds at runtime | No checking |
| `-W` | Generate compiler warning messages | No warnings |

> **Note:** The `-X` option described in the original Reference Manual is not
> implemented in version 4.0.

**Memory models**

| Option | Link library |
|--------|--------------|
| *(none)* | `clearl.l86` (large model — default) |
| `-S` | `clears.l86` (small model) |

`clears.l86` and `clearl.l86` are mutually exclusive — do not mix object files
compiled with different models.

**Output files**

| File | Contents |
|------|----------|
| `<file>.obj` | Intel OMF-86 relocatable object |
| `<file>.lis` | Source/machine-code interlisting (requires `-I`) |
| `<file>.cil` | Intermediate code file (temporary; deleted on success) |
| `<file>.cym` | Symbol intermediate file (temporary; deleted on success) |

> **Note:** Always end the source file with a carriage return. Without it the
> compiler cannot detect end-of-file and reports an error.

**Typical invocations**
```sh
# Basic compile
drfcpm_f77 foo.f77

# Small model, enable warnings
drfcpm_f77 -S -W foo.f77

# Free-field format source, 4-byte integers
drfcpm_f77 -F -M4 foo.f77

# Generate interlisting
drfcpm_f77 -I foo.f77
```

Makefile explicit rule (from `examples/Makefile`):
```makefile
F77    = drfcpm_f77
F77LNK = drfcpm_link

hellof77.cmd: hellof77.f77
	$(F77) hellof77.f77
	$(F77LNK) 'hellof77.cmd=hellof77'
```

---

## 2. DR FORTRAN-77 Linker — `drfcpm_link`

Linker. Combines DR FORTRAN-77 `.obj` files with the DR F77 runtime libraries
into a CP/M-86 `.cmd` executable.

`drfcpm_link` is the standard LINK-86 linker with `D:` mapped to
`share/f77cpm/` so the DR FORTRAN-77 runtime libraries are found automatically.
**The link syntax is identical to [`pcdev_linkcmd`](../asm/asm-ref.md#2-link-86----pcdev_linkcmd)**
— refer to that section for the full option reference (`[map]`, `[search]`,
`[$sz]`, I/O redirection, etc.).

**Syntax**
```
drfcpm_link '<outfile>.cmd[=obj1[,obj2,...]] [options]'
```

The entire argument **must be single-quoted** in the shell / Makefile so that
`$` and brackets are passed through literally.

**D: drive contents**

The following files are visible as `D:` inside the linker:

*Runtime libraries*

| File | Description |
|------|-------------|
| `clears.l86` | Small-model FORTRAN-77 runtime |
| `clearl.l86` | Large-model FORTRAN-77 runtime (default) |
| `f32s.obj` | Single-precision (32-bit) intrinsics — small model |
| `f32l.obj` | Single-precision (32-bit) intrinsics — large model |

*Utility programs (not linked — on `D:` for reference)*

| File | Description |
|------|-------------|
| `link86.cmd` | Native CP/M-86 LINK-86 (used internally) |
| `lib86.cmd` | Native CP/M-86 LIB-86 |
| `clear87.cmd` | 8087 simulator reset utility |
| `8087.sim` | 8087 coprocessor emulation routines |
| `f77.ers` | Compiler error message texts |
| `f77io.err` | Runtime error message texts |

> **Note:** The compiler embeds an auto-library record in every `.obj` that tells
> LINK-86 which runtime library to use. The default
> `drfcpm_link 'foo.cmd=foo.obj'` is sufficient in the normal case — the linker
> selects `clearl.l86` or `clears.l86` automatically based on the model flag
> embedded at compile time.

**F32 single-precision libraries**

To use 32-bit single-precision intrinsics instead of the default 64-bit routines,
add `f32s` (small model) or `f32l` (large model) before the runtime library and
include the `[sim8087]` option:

```sh
drfcpm_link 'foo.cmd=foo.obj,D:f32s,D:clears.l86[search] [sim8087]'
```

> **Note:** Do not use `COMPLEX` or `DOUBLE PRECISION` variables with the F32
> libraries — results will be unpredictable and LINK-86 may report
> `MULTIPLE DEFINITION`.

**Output files**

| File | Contents |
|------|----------|
| `<out>.cmd` | CP/M-86 executable |
| `<out>.sym` | Symbol table (optional) |
| `<out>.map` | Segment map (requires `[map]`) |

**Typical invocations**
```sh
# Default: auto-library record selects the right runtime automatically
drfcpm_link 'foo.cmd=foo.obj'

# Multiple objects
drfcpm_link 'app.cmd=main.obj,util.obj'

# Explicit small-model runtime
drfcpm_link 'foo.cmd=foo.obj,D:clears.l86[search]'

# With 32-bit single-precision math (small model)
drfcpm_link 'foo.cmd=foo.obj,D:f32s,D:clears.l86[search] [sim8087]'

# With segment map
drfcpm_link 'foo.cmd=foo.obj [map]'
```

In Makefiles the `$` inside single-quoted strings must be `$$`:
```makefile
F77    = drfcpm_f77
F77LNK = drfcpm_link

hellof77.obj: hellof77.f77
	$(F77) $<

hellof77.cmd: hellof77.obj
	$(F77LNK) 'hellof77.cmd=hellof77'
```
