# CP/M-86 Pascal/MT+ Toolchain — Command Reference

Quick reference for the Pascal/MT+ tools available via `../../bin/`.
All examples use the exact binary names as they appear in the project Makefiles.

---

## 1. Pascal/MT+ Compiler — `drpmt_pc`

Multi-phase Pascal compiler. Reads one `.pas` (or `.src`) source file, emits
a Microsoft-compatible relocatable `.r86` object file.

**Syntax**
```
drpmt_pc <filename> {$<switches>}
```

The source filename may be prefixed with a drive letter (`A:`–`P:`, `@` for
the logged-in disk).  Extension defaults to `.src` then `.pas` if not found.
Switches follow the filename, preceded by `$` or `#`; spaces between switches
are ignored (`$PXRB` is the same as `$PX RB`).

**Command-line switches** (after `$`)

| Switch | Meaning | Default |
|--------|---------|---------|
| `Rd`   | Write `.r86` output to drive `d` (`@`,`A`–`O`) | Same drive as source |
| `Od`   | Look for `mt-86` root and overlays on drive `d` | Default drive |
| `Pd`   | Write `.prn` listing file to drive/device `d` (`X`=console, `P`=printer, `@`,`A`–`O`) | No listing |
| `x`    | Extended `.r86`: include disassembler records (required with `DIS8080`) | Off |
| `D`    | Debugger mode: embed debug info, write `.psy` symbol file | Off |
| `Ed`   | Look for `mterrs.txt` error messages on drive `d` | Default drive |
| `Td`   | Write `pastemp.tok` temp file to drive `d` | Default drive |
| `Q`    | Quiet: suppress progress messages | Verbose |
| `C`    | Continue on error without prompting | Pause on each error |
| `A`    | Auto-chain to `linkmt.cmd` at end of compilation | No auto-link |
| `B`    | Use BCD real arithmetic instead of floating-point | Floating-point |
| `Z`    | Generate Z80-optimised code | 8080-only code |
| `V`    | Print each procedure/function name found (aids locating errors) | Off |
| `@`    | Treat `@` as synonym for `^` (pointer dereference) | Off |

**In-source toggles**

Toggles appear in Pascal comments: `(*$toggle*)` or `{$toggle}`.
One toggle per comment; leading spaces are skipped but embedded spaces after
the key letter produce a parse error.

| Toggle | Meaning | Default |
|--------|---------|---------|
| `$E+` / `$E-` | Export entry-point records (make symbols public) | `$E+` |
| `$S+` / `$S-` | Stack-frame (recursive) variable allocation | `$S-` |
| `$I <name>` | Include named file into source stream, e.g. `{$I D:USERFILE.LIB}` | — |
| `$R+` / `$R-` | Run-time array/subrange bounds checking | `$R-` |
| `$T+` / `$T-` | Strict ISO type-checking and portability warnings | `$T-` |
| `$W+` / `$W-` | Same as `$T` (tightly coupled) | `$W-` |
| `$X+` / `$X-` | Run-time exception checking (divide by zero, overflow, …) | `$X+` |
| `$P` | Insert form-feed in `.prn` listing | — |
| `$L+` / `$L-` | Enable/disable listing output | `$L+` |
| `$Kn` | Remove built-in symbol group `n` (0–15) from symbol table to save space | — |
| `$Z$nnnn` | Set initial hardware stack pointer to `nnnnH` (non-CP/M only) | From location 0006 |
| `$Cn` | Use `RST n` instead of `CALL` for real-number operations (`n`=0–7) | `CALL` |
| `$Qn` | Use `RST n` instead of `CALL` for loads/stores in recursive code | `CALL` |

**`$Kn` group table** — specify before `PROGRAM` or `MODULE`:

| `n` | Routines removed |
|-----|-----------------|
| 0 | `ROUND TRUNC EXP LN ARCTAN SQRT COS SIN` |
| 1 | `COPY INSERT POS DELETE LENGTH CONCAT` |
| 2 | `GNB WNB CLOSEDEL OPENX BLOCKREAD BLOCKWRITE` |
| 3 | `CLOSE OPEN PURGE CHAIN CREATE` |
| 4 | `WRD HI LO SWAP ADDR SIZEOF INLINE EXIT PACK UNPACK` |
| 5 | `IORESULT PAGE NEW DISPOSE` |
| 6 | `SUCC PRED EOF EOLN` |
| 7 | `TSTBIT CLRBIT SETBIT SHR SHL` |
| 8 | `RESET REWRITE GET PUT ASSIGN MOVELEFT MOVERIGHT FILLCHAR` |
| 9 | `READ READLN` |
| 10 | `WRITE WRITELN` |
| 12 | `MEMAVAIL MAXAVAIL` |
| 13 | `SEEKREAD SEEKWRITE` |
| 14 | `RIM85 SIM85 WAIT` |
| 15 | `READHEX WRITEHEX` |

**Output files**

| File | Contents |
|------|----------|
| `<name>.r86` | Relocatable object (Microsoft-compatible ERL format) |
| `<name>.prn` | Compiler listing (requires `$Pd`) |
| `<name>.psy` | Debugger symbol table (requires `$D`) |
| `pastemp.tok` | Compiler temp file (deleted on success) |

**Typical invocations**
```sh
# Compile to default drive, no listing
drpmt_pc foo

# ERL to B:, listing to console, auto-chain to linker
drpmt_pc foo '$RB PX A'

# Extended ERL for disassembler, range-checking on
drpmt_pc foo '$x'   # source must contain {$R+}
```

Makefile pattern (from `examples/Makefile`):
```makefile
PMT = drpmt_pc

hellopmt.r86: hellopmt.pas
	$(PMT) hellopmt
	ls -l $@
```

> **Note:** Switches are passed through `emu2` unchanged; the entire switch
> string should be quoted in the shell so `$` is not expanded.

---

## 2. LINK/MT+ Linker — `drpmt_link`

Links Pascal/MT+ `.r86` relocatable modules and runtime libraries into a CP/M
`.com` executable.

**Syntax**
```
drpmt_link {<outfile>=}<main>,<module(s)>,PASLIB{/switches}
```

All input files use the `.r86` extension (called `.erl` in the manual) but
the extension is **not** specified on the command line.  Output defaults to
the same drive as the main program.  A maximum of 32 input names.

**Switches** (precede with `/`, append after the last file or the file they
modify)

| Switch | Meaning |
|--------|---------|
| `/S`   | Search the preceding library; extract only referenced modules (position-sensitive: place directly after the library name) |
| `/L`   | List module code/data addresses as they are linked |
| `/M`   | Print memory map of all entry points to console |
| `/E`   | Like `/M` but also includes `$`, `?`, `@` runtime symbols |
| `/P:nnnn` | Relocate code segment to hex address `nnnn` (ROM placement) |
| `/D:nnnn` | Relocate data segment to hex address `nnnn` |
| `/H:nnnn` | Write Intel HEX output (starting at `nnnn`) instead of `.com` |
| `/W`   | Write SID-compatible `.sym` symbol file |
| `/F`   | Treat preceding filename as a `.cmd` linker-input command file |
| `/Vm:nnnn` | Overlay area starting address (see §3.3) |
| `/X:nnnn` | Extra static data space added to `SYSMEM` (overlays) |
| `/O:n` | Number this overlay; preceding filename = root symbol table |

**Runtime libraries** (in `share/drpmtcpm/`, always link `PASLIB` last)

| File | Contents |
|------|----------|
| `paslib.r86` | Core runtime: I/O, arithmetic, string, heap (always required) |
| `fpreals.r86` | Software floating-point `@XOP @RRL @WRL`; searchable; link before `paslib` |
| `bcdreals.r86` | BCD real arithmetic (alternative to `fpreals`; not searchable) |
| `trancend.r86` | Transcendentals: `SIN COS ARCTAN SQRT LN EXP SQR` |
| `randomio.r86` | `SEEKREAD` / `SEEKWRITE` random-access I/O |
| `fullheap.r86` | ISO heap management with garbage collection (link before `paslib`) |
| `debugger.r86` | Symbolic debugger (link as main when `$D` was used) |

**Output files**

| File | Contents |
|------|----------|
| `<out>.com` | CP/M executable |
| `<out>.sym` | SID symbol table (requires `/W`) |

**Typical invocations**
```sh
# Basic: main + transcendentals + floating-point + runtime
drpmt_link 'calc,trancend,fpreals,paslib/s'

# Output to B:, with memory map
drpmt_link 'b:calc=calc,trancend,fpreals,paslib/s /m'

# Using a CMD command file
# (CALC.CMD contains: A:calc,D:trancend,D:fpreals,D:paslib/s)
drpmt_link 'calc/f/l'

# ROM placement: code at 8000h, data at C000h
drpmt_link 'rom=main,paslib/s /p:8000 /d:c000'
```

Makefile pattern (from `examples/Makefile`):
```makefile
PMT     = drpmt_pc
PMTLINK = drpmt_link

hellopmt.r86: hellopmt.pas
	$(PMT) hellopmt

hellopmt.cmd: hellopmt.r86
	$(PMTLINK) hellopmt.cmd=hellopmt,p:paslib/s
	ls -l $@
	$(CMDINFO) $@
```

> **Note:** `/S` is position-sensitive — it must immediately follow the
> library filename.  `PASLIB` and `FPREALS` are searchable; user modules and
> `BCDREALS` are not searchable unless pre-processed by `LIB/MT+`.

> **Gotcha:** `FULLHEAP` must be linked **before** `PASLIB`; `FPREALS` must be
> linked **before** `PASLIB`; `TRANCEND` must precede `FPREALS` when both are
> used.  Typical full order: `main, trancend, fpreals, paslib/s`.
