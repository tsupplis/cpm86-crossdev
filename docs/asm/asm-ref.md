# CP/M-86 Assembler Toolchain — Command Reference

Quick reference for the five CP/M-86 assembly tools available via `../../bin/`.
All examples use the exact binary names as they appear in the project Makefiles.

---

## 1. RASM-86 — `pcdev_rasm86`

Relocating assembler. Reads `.a86` source, emits Intel OMF-86 object code.

**Syntax**
```
pcdev_rasm86 <file>[.a86] [$ <params>]
```

**Parameters** (after `$`, any order, no space between param and device)

| Param | Meaning | Valid devices |
|-------|---------|---------------|
| `A`   | Source file device | `A`–`P` |
| `L`   | Include local symbols in `.obj` | `O` (only valid argument) |
| `O`   | Object file device | `A`–`P`, `Z` (suppress) |
| `P`   | List file device | `A`–`P`, `X` (console), `Y` (printer), `Z` (suppress) |
| `S`   | Symbol file device | `A`–`P`, `X`, `Y`, `Z` (suppress) |

Device names: `A`–`P` = drives; `X` = console; `Y` = printer; `Z` = suppress output.

**Output files**

| File | Contents |
|------|----------|
| `<file>.obj` | Intel OMF-86 relocatable object |
| `<file>.lst` | Assembly listing with error messages |
| `<file>.sym` | Symbol table |

**Typical invocations**
```sh
# Suppress list and symbol files (normal build)
pcdev_rasm86 foo.a86 $$ pz sz

# Listing to console, suppress symbol file
pcdev_rasm86 foo.a86 $$ px sz

# Include local symbols in .obj (for debugger / XREF-86)
pcdev_rasm86 foo.a86 $$ lo
```

> **Note:** In Make rules, `$` must be written `$$` to prevent shell expansion.
> In a shell one-liner, quote the parameter string: `pcdev_rasm86 foo.a86 '$ pz sz'`.

---

## 2. LINK-86 — `pcdev_linkcmd`

Linker. Combines `.obj` / `.l86` modules into a CP/M-86 `.cmd` executable.

**Syntax**
```
pcdev_linkcmd '<outfile>[=obj1,obj2,...] [options]'
```

If `outfile` is omitted the name is taken from the first input file.
The entire argument **must be single-quoted** in the shell / Makefile so that
`$` and brackets are passed through literally.

**Options** (enclosed in `[…]` immediately after the file they affect)

| Option | Abbr | Meaning |
|--------|------|---------|
| `[$sz]` | — | Suppress "NO STACK SEGMENT" warning (use for pure-asm programs) |
| `[map]` | `[m]` | Produce `<out>.map` segment-layout file |
| `[noprefix]` | `[nop]` | Omit high-level-language startup prefix (pure asm only) |
| `[data[add[N]]]` | `[d[ad[N]]]` | Add `N` paragraphs of runtime data space to DATA section header |
| `[data[ori[N]]]` | `[d[ori[N]]]` | Set DATA section origin to `N` paragraphs |
| `[stack[N]]` | — | Reserve `N` paragraphs of stack |
| `[search]` | `[s]` | Applied to the preceding filename; search that library, link only referenced modules |
| `[locals]` | `[lo]` | Include local symbols in `.sym` output (default: on) |
| `[nolocals]` | `[nolo]` | Exclude local symbols from `.sym` output |
| `[libsyms]` | `[li]` | Include library symbols in `.sym` output |
| `[nolibsyms]` | `[noli]` | Exclude library symbols from `.sym` (default) |
| `[input]` | `[i]` | Read remainder of command from disk file (filetype `.inp`) |

**I/O redirection** (`$td` where `t`=type, `d`=drive/device)

| Option | Meaning |
|--------|---------|
| `$Cd`  | Command (`.cmd`) output drive; `$Cz` suppresses |
| `$Ld`  | Drive to search for auto-linked runtime libraries |
| `$Md`  | Map file drive; `$Mx` → console |
| `$Od`  | Object/library input file drive |
| `$Sd`  | Symbol file drive; `$Sz` suppresses |

Drive: `A`–`P` = disk; `X` = console; `Y` = printer; `Z` = suppress.

**Output files**

| File | Contents |
|------|----------|
| `<out>.cmd` | CP/M-86 executable |
| `<out>.sym` | Symbol table (optional) |
| `<out>.map` | Segment map (requires `[map]`) |

**Typical invocations**
```sh
# Single object → .cmd, suppress stack warning
pcdev_linkcmd 'foo [$sz]'

# Multi-object link
pcdev_linkcmd 'main=main,util,helper [$sz]'

# With library (search mode — only pulls referenced modules)
# [$sz] must go on the output name; it cannot follow [search] as a second bracket
pcdev_linkcmd 'app[$sz]=app,mathlib.l86[search]'

# With map file for segment inspection
pcdev_linkcmd 'foo [map,$sz]'
```

In Makefiles the `$` inside single-quoted strings must be `$$`:
```makefile
$(LINK86) $* '[$$sz]'
$(LINK86) 'ex5main=ex5main,ex5util [$$sz]'
$(LINK86) 'ex6lib[$$sz]=ex6lib,mathlib.l86[search]'
```

---

## 3. LIB-86 — `pcdev_lib86`

Library manager. Creates and maintains `.l86` archives of OMF-86 object modules.

**Syntax**
```
pcdev_lib86 '<destlib>[=src1,src2,...] [options]'
```

Files without an extension default to `.obj`; the output library defaults to `.l86`.

**Options**

| Option | Abbr | Purpose |
|--------|------|---------|
| `[map]` | `[ma]` | Create module map file `<lib>.map` |
| `[xref]` | `[x]` | Create cross-reference file `<lib>.xrf` |
| `[modules]` | `[mo]` | List module names only |
| `[publics]` | `[p]` | List public symbols |
| `[externals]` | `[e]` | List external references |
| `[segments]` | `[seg]` | List segment names per module |
| `[replace[<mod>-<file>,...]]` | `[r[...]]` | Replace named module(s) in-place |
| `[delete[<mod>,...]]` | `[d[...]]` | Delete named module(s) |
| `[select[<mod>,...]]` | `[sel[...]]` | Extract only named module(s) to new lib |
| `[noalpha]` | `[n]` | List modules in order of occurrence (not alphabetically) |
| `[input]` | `[i]` | Read command from disk file (filetype `.inp`) |

`DELETE`, `REPLACE`, and `SELECT` are mutually exclusive.

**I/O redirection**

| Option | Meaning |
|--------|---------|
| `$Md`  | MAP file destination (`X`=console, `Y`=printer) |
| `$Od`  | OBJ/L86 source drive |
| `$Xd`  | XRF file destination |

**Typical invocations**
```sh
# Create new library from object files
pcdev_lib86 'mathlib=add16,mul16'

# Add modules to existing library
pcdev_lib86 'mathlib=mathlib.l86,div16,mod16'

# Replace one module
pcdev_lib86 'mathlib=mathlib.l86 [replace[add16-newadd]]'

# Delete a module
pcdev_lib86 'mathlib=mathlib.l86 [delete[mul16]]'

# Inspect: list all public symbols
pcdev_lib86 'mathlib.l86 [publics]'

# Full module map to console
pcdev_lib86 'mathlib.l86 [map,$mx]'

# Extract selected modules into new library
pcdev_lib86 'arith=mathlib.l86 [select[add16,sub16]]'
```

---

## 4. ASM86 — `cpm86_asm86`

CP/M-86 system assembler. Like RASM-86 but produces Intel/DR hex output (`.h86`)
instead of OMF-86 object code. Use when targeting the GENCMD path.

**Syntax**
```
cpm86_asm86 <file>[.a86] [$ <params>]
```

**Parameters** (after `$`)

| Param | Meaning | Valid arguments |
|-------|---------|-----------------|
| `A`   | Source file device | `A`–`P` |
| `H`   | Hex output file device | `A`–`P`, `X`, `Y`, `Z` |
| `P`   | List file device | `A`–`P`, `X`, `Y`, `Z` |
| `S`   | Symbol file device | `A`–`P`, `X`, `Y`, `Z` |
| `F`   | Hex output format | `I` = Intel hex; `D` = Digital Research hex (default) |

> **Note:** ASM86 uses `H` (hex output) instead of RASM-86's `O` (object output).
> There is no `L` parameter; local symbols are always included.

**Output files**

| File | Contents |
|------|----------|
| `<file>.h86` | Hex-format object (Intel or DR hex) |
| `<file>.lst` | Assembly listing |
| `<file>.sym` | Symbol table |

**Key differences from RASM-86**

| Feature | RASM-86 | ASM86 |
|---------|---------|-------|
| Output format | `.obj` (OMF-86) | `.h86` (hex) |
| Next stage | `pcdev_linkcmd` | `cpm86_gencmd` |
| `RD` directive | Supported | Not supported |
| `CodeMacro` | Supported | Supported |
| `INCLUDE` directive | Supported | Supported |
| Local-symbol param | `L` → object | None (always included) |
| Hex format param | — | `F I`/`F D` |
| `end <label>` | Names entry point | Error — use bare `end` |
| Multi-segment | `cseg`+`dseg` → LINK-86 | `cseg`+`dseg` → GENCMD (no keyword) |
| `dseg` data origin | 0 (loader handles offset) | Must use `org 100h` — base page at DS:0–FFh |

**Typical invocations**
```sh
# Assemble, suppress list and symbol files
cpm86_asm86 foo.a86 $$ hz sz

# Assemble with listing to console
cpm86_asm86 foo.a86 $$ px

# Assemble with Intel hex output
cpm86_asm86 foo.a86 $$ fi
```

Makefile pattern rule (from `examples/Makefile`):
```makefile
%.h86: %.a86
	$(ASM86) $<
```

---

## 5. GENCMD — `cpm86_gencmd`

Converts an ASM86-produced `.h86` hex file into a CP/M-86 `.cmd` executable.

**Syntax**
```
cpm86_gencmd <filespec> [8080 | CODE[An,Bn,Mn,Xn] DATA[An,Bn,Mn,Xn] STACK[...] EXTRA[...] X1[...]]
```

**Model keyword**

| Keyword | Meaning |
|---------|---------|
| `8080`  | 8080 Memory Model: code and data groups overlap; code begins at `100h`. Use for `org 100h` single-segment programs. |
| *(omit)* | GENCMD reads segment layout from the `.h86` file header (DR hex format embeds it). Works for multi-segment programs assembled by ASM86 in DR format (default). |

**Per-group placement values** (appended in `[...]` after each group keyword)

| Value | Meaning |
|-------|---------|
| `An`  | Load group at absolute paragraph address `n` |
| `Bn`  | Begin group at offset `n` in the hex file (needed only when `.h86` is Intel format — `ASM86 $ FI`) |
| `Mn`  | Group requires minimum `n × 16` bytes (use when group has uninitialized data at end) |
| `Xn`  | Group can address up to `n × 16` bytes if available |

`n` is a 1–4 digit hex constant; each paragraph = 16 bytes.

**Group keywords:** `CODE`, `DATA`, `STACK`, `EXTRA`, `X1`, `X2`, `X3`, `X4`

**Typical invocations**
```sh
# Single-segment org 100h → 1 CMD header
cpm86_gencmd foo.h86 8080

# Dual-segment cseg+dseg → 2 CMD headers (DR hex layout, no keyword)
cpm86_gencmd foo.h86

# Override placement: data minimum and max paragraphs
cpm86_gencmd myfile CODE[A40] DATA[M30,XFFF]
```

> **Note:** `Bn` is only needed when the `.h86` was produced with `ASM86 $ FI`
> (Intel hex format). The default DR hex format already encodes segment information,
> so `Bn` is not required for normal ASM86 output.

> **Gotcha:** for dual-segment programs, put `org 100h` in the `dseg` — the loader
> reserves DS:0–FFh for the base page. Data at DS:0 overlaps it and produces
> garbled output (see tutorial §10 gotcha #23).

**Output file:** `<file>.cmd`

Makefile explicit rules (avoids conflict with `%.cmd: %.obj` pattern):
```makefile
# Single-segment (8080 model)
foo.cmd: foo.h86
	$(GENCMD) $< 8080

# Dual-segment (DR hex — no keyword)
bar.cmd: bar.h86
	$(GENCMD) $<
```

---

## 6. XREF-86 — `pcdev_xref86`

Cross-reference utility. Annotates a RASM-86 listing with line-number references
for every symbol.

**Syntax**
```
pcdev_xref86 <filename> [$p]
```

Reads `<filename>.lst` and `<filename>.sym` (both produced by RASM-86).
Writes `<filename>.xrf` (annotated listing + cross-reference table).
`$p` redirects output to the printer instead of to the `.xrf` file.

**Output file:** `<filename>.xrf`

**Typical invocation**
```sh
# Assemble with locals, then cross-reference
pcdev_rasm86 foo.a86 '$ lo px'
pcdev_xref86 foo
```
