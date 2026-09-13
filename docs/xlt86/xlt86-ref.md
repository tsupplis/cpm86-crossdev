# CP/M-86 XLT86 Translator — Command Reference

Quick reference for the Digital Research XLT86 translator available via `../../bin/cpm_xlt86`.
All examples use the exact binary names as they appear in the project Makefiles.

---

## 1. 8080 to 8086 Translator — `cpm_xlt86`

Source-to-source translator. Reads an Intel 8080 assembly source file (`<file>.asm`) and produces an equivalent Intel 8086 assembly source file (`<file>.a86`) suitable for assembly with `cpm86_asm86`, `cpm_asm86`, or `pcdev_rasm86`.

`cpm_xlt86` runs the CP/M-80 binary `xlt86.com` and its overlays (`xlt00.ovl`, `xlt01.ovl`) under `tnylpo`.

Unlike simple token-replacement translators, XLT86 performs **global data flow analysis** across basic blocks to analyze register and flag lifetimes, optimizing 8086 code generation and eliminating redundant register saves/moves.

**Syntax**
```
cpm_xlt86 <file>[.asm] ['['<parameters>']']
```

Pass the file basename or filename. The input is expected to be `<file>.asm`, and the output is generated as `<file>.a86`.

> **Note:** When specifying parameters in brackets on the command line, quote the parameters or brackets to protect them from shell expansion. In Makefiles, write `$$` for `$` and escape brackets if necessary.

**Parameters**

Parameters can be given on the command line in brackets `[...]` separated by spaces or embedded directly in the source program starting in column 1. Prefixing a parameter with `NO` negates its effect (e.g. `NOJ`, `NOS`).

| Parameter | Default | Meaning | Description |
|-----------|---------|---------|-------------|
| `A<d>` | default drive | Set A86 drive | Place the output `.a86` file on drive `<d>` (`A`–`P`). |
| `B` | `NOB` | Block trace | Output basic block register usage tables in the `.prn` listing. |
| `C` | `NOC` | Compact model | Generate non-overlapping code/data segments using `DSEG`/`CSEG` transitions. |
| `J` | `J` | Short jump optimization | Translate conditional jumps to short conditional jumps or inverted short jump pairs. |
| `L` | `NOL` | List to printer | Send `.prn` listing directly to the system printer. |
| `N` | `NON` | Line numbers | Display line and statement numbers on console during translation. |
| `P<d>` | default drive | Set PRN drive | Place the listing `.prn` file on drive `<d>` (`A`–`P`). |
| `R` | `NOR` | Return assumption | Assume no flags are active/returned at subroutine returns (reduces code size). |
| `S` | `NOS` | Segment directives | Assume source contains explicit `CSEG`/`DSEG`/`ASEG` directives (overrides `C`). |
| `T<d>` | default drive | Set temp drive | Place temporary file (`<file>.$$$`) on drive `<d>` (`A`–`P`). |
| `80` | `NO80` | 8080 listing | Include 8080 pseudo-assembly listing in the `.prn` file. |
| `86` | `NO86` | 8086 listing | Include 8086 translated line and statement listing in the `.prn` file. |

**Output files**

| File | Contents |
|------|----------|
| `<file>.a86` | Translated 8086 assembly language source file |
| `<file>.prn` | Listing file containing errors, translation trace, and optional listings |
| `<file>.$$$` | Temporary work file (automatically removed upon successful completion) |

**Key Translation Mappings**

*Registers*

| 8080 (8-bit) | 8086 Equivalent | 8080 (16-bit) | 8086 Equivalent |
|--------------|-----------------|---------------|-----------------|
| `A` | `AL` | `PSW` | `AX` (flags / accumulator) |
| `B` | `CH` | `B` (`BC`) | `CX` |
| `C` | `CL` | `D` (`DE`) | `DX` |
| `D` | `DH` | `H` (`HL`) | `BX` |
| `E` | `DL` | `SP` | `SP` |
| `H` | `BH` | | |
| `L` | `BL` | | |
| `M` | `Byte Ptr 0[BX]` (via `M EQU Byte Ptr 0[BX]`) | | |

*Special BDOS and BIOS Calls*

| 8080 Instruction | Translated 8086 Output | Notes |
|------------------|------------------------|-------|
| `CALL 5` | `INT 224` (`0E0H`) | CP/M-86 BDOS entry |
| `JMP 5` | `INT 224` | BDOS entry |
| `CALL 0` | `MOV CL,0` <br> `MOV DL,0` <br> `INT 224` | CP/M-86 Warm Boot / Exit |
| `JMP 0` | `MOV CL,0` <br> `MOV DL,0` <br> `INT 224` | CP/M-86 Warm Boot / Exit |

**Typical Invocations**

```sh
# Basic translation: foo.asm -> foo.a86
cpm_xlt86 foo

# Translation with 8080 and 8086 listing in PRN file
cpm_xlt86 foo '[80 86]'

# Translation assuming subroutines do not return flags
cpm_xlt86 foo '[R]'

# Translation with compact segment model
cpm_xlt86 foo '[C]'
```

**Typical Build Flow**

```sh
# 1. Translate 8080 source to 8086 source
cpm_xlt86 hello80

# 2. Assemble with ASM-86 (or RASM-86)
cpm86_asm86 hello80.a86

# 3. Generate CMD executable
cpm86_gencmd hello80.h86
```

**Makefile pattern (from `examples/Makefile`):**
```makefile
XLT86  = cpm_xlt86
ASM86  = cpm86_asm86
GENCMD = cpm86_gencmd

hello80.a86: hello80.asm
	$(XLT86) hello80

hello80.h86: hello80.a86
	$(ASM86) $<

hello80.cmd: hello80.h86
	$(GENCMD) $< 8080
```

> **Note:** "Number of Errors: 1" during ASM-86 / GENCMD assembly for `ORG 100H` is expected and harmless for translated 8080 programs.
