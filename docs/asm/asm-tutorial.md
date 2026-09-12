# CP/M-86 Assembly Guide

## 1. Toolchain Overview

Two toolchain paths produce a runnable `.cmd` binary.

```mermaid
flowchart LR
    subgraph RASM-86 path
        A86["source.a86"] -->|unix2dos| DOS["source.a86\n(CRLF)"]
        DOS -->|pcdev_rasm86| OBJ["source.obj\n(OMF-86)"]
        OBJ -->|pcdev_linkcmd| CMD1["source.cmd"]
        OBJ2["other.obj\n(optional)"] --> CMD1
        LIB["lib.l86\n(optional)"] --> CMD1
    end
    subgraph ASM86 path
        B86["source.a86"] -->|unix2dos| BDOS2["source.a86\n(CRLF)"]
        BDOS2 -->|cpm86_asm86| H86["source.h86"]
        H86 -->|cpm86_gencmd 8080| CMD2["source.cmd"]
    end
```

**RASM-86** produces OMF-86 `.obj` files that LINK-86 combines into a `.cmd`.
Use it for all multi-segment, multi-module, and library work.
**ASM86+GENCMD** is a simpler single-pass path: single `cseg org 100h` only,
no `CodeMacro`, no `RD` directive. See §5 and §8 for the comparison.

> Both tools require **CRLF line endings** — run `unix2dos` before assembling (§10 gotcha #2).

---

## 2. Program Entry State

CP/M-86 sets registers before jumping to the program entry point.

### `org 100h` (single-segment, via RASM-86 or ASM86)

| Register | Value at entry | Notes |
|----------|---------------|-------|
| CS | code segment | your `cseg` |
| DS | base-page segment | **not** CS — redirect before using data |
| ES | base-page segment | same as DS |
| SS | stack segment | CP/M default stack |
| SP | top of CP/M stack | usable |
| DS:80h | command tail length | byte count |
| DS:81h | command tail text | space + args |

> Read the command tail **before** redirecting DS (§7.7, §10 gotcha #9).

### Dual/triple/stack-segment (RASM-86 via LINK-86, or ASM86 via GENCMD no keyword)

| Register | Value at entry | Notes |
|----------|---------------|-------|
| CS | `cseg` base | — |
| DS | `dseg` / dgroup base | set by loader from DATA header |
| ES | `eseg` base (if present) | else = DS |
| SS | `sseg` base (if `sseg` linked) | else CP/M default |
| SP | `sseg` top (if `sseg` linked) | else CP/M default |

---

## 3. Segment Patterns

### 3.1 Single Segment — `org 100h`

Source: [`ex1sing.a86`](ex1sing.a86)

Single `cseg` with `org 100h`. No separate data segment — DS ≠ CS at entry;
must redirect DS and ES to CS before accessing any data labels.

Key points:
- `jmp main` at top so execution skips data
- All data defined **before** code (avoids forward-reference ERROR 8)
- `key_char rb 1` — saves BDOS return value before the next BDOS call clobbers AX/BX
- `end main` names the entry point

### 3.2 Dual Segment — CODE + DATA

Source: [`ex2dual.a86`](ex2dual.a86)

Separate `cseg` (code) and `dseg` (data). CP/M-86 loader sets DS to the data
segment base at entry — no redirect needed for DS, but ES must be copied from DS
before using string instructions.

Key points:
- `mov cx,ds` / `mov es,cx` at entry — DS is already correct
- `offset line1` resolves into the dseg; DS points there at entry
- `end` with **no label** — LINK-86 uses the first code byte as entry point

### 3.3 Triple Segment — CODE + DATA + EXTRA

Source: [`ex3trip.a86`](ex3trip.a86)

Adds an `eseg` (extra segment). The loader sets ES to the extra segment base.
Demonstrates `rep movsb` copying from DS:SI (dseg) to ES:DI (eseg), then
printing the copy.

Key points:
- `mov ax,seg dst_buf` / `mov es,ax` — reload ES explicitly from the extra segment
- `rep movsb` copies SRC_LEN bytes including the `'$'` terminator
- After copy, `mov ax,es` / `mov ds,ax` redirects DS so fn 9 can print from eseg
- `end` with no label

### 3.4 Explicit Stack Segment

Source: [`ex4stak.a86`](ex4stak.a86)

Adds an `sseg` and groups DATA + STACK into `dgroup`. LINK-86 produces no
"NO STACK SEGMENT" warning.

Key points:
- `dgroup group DATA,STACK` — one segment register covers both
- `rw 32` reserves 64 bytes; `stk_top rw 0` places a label at the top (SP initial value)
- `mov ax,seg stk_top` returns the dgroup base; sets SS, DS, ES, and SP in one block
- `end main` (explicit entry label needed because sseg comes first in source)


---

## 4. Multi-Module and Libraries

### 4.1 Public / Extrn — Two-File Link

Sources: [`ex5util.a86`](ex5util.a86) · [`ex5main.a86`](ex5main.a86)

Demonstrates splitting code across two object files linked together.

Key points:
- `ex5util.a86` declares `public str_upper` — no `org`, no entry point, `end` with no label
- `ex5main.a86` declares `extrn str_upper:near` before the `cseg`
- Link command joins both objects: `pcdev_linkcmd 'ex5main=ex5main,ex5util [$sz]'`
- `str_upper` walks a null-terminated string in-place; `and al,0DFh` clears bit 5 → uppercase
- After calling `str_upper`, the null at offset 20 is already in place; the `'$'` at offset 21 lets fn 9 print the result directly

### 4.2 Library Creation and Use (LIB-86)

Sources: [`add16.a86`](add16.a86) · [`mul16.a86`](mul16.a86) · [`ex6lib.a86`](ex6lib.a86)

Demonstrates building a `.l86` library and linking against it.

Key points:
- `add16.a86`: `public add_u16` — `add ax,bx; ret`; no entry point
- `mul16.a86`: `public mul_u16` — `mul bx; ret`; DX clobbered (high 16 bits of product)
- Library built with: `pcdev_lib86 'mathlib=add16,mul16'`
- `ex6lib.a86` links against it: `pcdev_linkcmd 'ex6lib=ex6lib,mathlib.l86[search] [$sz]'`
- `[search]` tells LINK-86 to pull only modules that satisfy unresolved extrns
- `word_to_hex` is a local routine in `ex6lib.a86`; all inter-call state lives in named memory vars to survive BDOS clobbering AX/BX


---

## 5. ASM86 / GENCMD Path

ASM86 produces a `.h86` hex file (Digital Research format by default); GENCMD
packages it into a `.cmd`. Two segment models are supported.

### 5.1 Single segment — `8080` model

Source: [`ex7genc.a86`](ex7genc.a86)

Single `cseg org 100h`. Code and data share one segment. GENCMD `8080` keyword
produces a single CODE header; loader sets DS = CS.

Key points:
- Assemble: `cpm86_asm86 ex7genc.a86` → `ex7genc.h86`
- Package: `cpm86_gencmd ex7genc.h86 8080` → single-header `.cmd`
- `jmp main` at top; all data before code (forward-reference rule, same as RASM-86)
- `end` with no label — ASM86 does not accept `end <label>` (§10 gotcha #22)
- DS ≠ CS at entry — redirect: `mov cx,cs` / `mov ds,cx` / `mov es,cx`

### 5.2 Dual segment — `cseg` + `dseg`

Source: [`ex7gend.a86`](ex7gend.a86)

Separate code and data segments. ASM86 default DR hex embeds the segment type
in each record; GENCMD with **no keyword** reads the layout and produces a
two-header CMD (CODE + DATA).

Key points:
- Assemble: `cpm86_asm86 ex7gend.a86` → `ex7gend.h86`
- Package: `cpm86_gencmd ex7gend.h86` *(no keyword)* → two-header `.cmd`
- **`dseg org 100h` is required** — the loader reserves the first 256 bytes of DS
  for the base page; data placed at DS:0 overlaps it (§10 gotcha #23)
- DS is set by the loader at entry — no redirect needed; set ES = DS explicitly
- `end` with no label (same rule as single-segment ASM86 programs)

Makefile rules (explicit, to avoid conflict with `%.cmd: %.obj` pattern):

```makefile
ex7gend.h86: ex7gend.a86
	unix2dos $<
	$(ASM86) $<

ex7gend.cmd: ex7gend.h86
	$(GENCMD) $<
```

CMD headers produced:
```
INF: HDR(0)TYP(01,CODE)BAS(0000h)MN(0.1k=96) LEN(96)
INF: HDR(1)TYP(02,DATA)BAS(0000h)MN(0.4k=368)LEN(368)
```

---

## 6. Code-Macros

Source: [`ex8macr.a86`](ex8macr.a86)

RASM-86 `CodeMacro` / `EndM` defines inline byte-encoding macros. Three macros are demonstrated.

Key points:
- `BDOS_CALL fn:Db` — emits `B9h <fn word>` (MOV CX,imm16) + `CDh E0h` (INT 0E0h); parameter type `Db` covers fn 0–255
- `PUSHI imm:Db` — emits `68h <word>` (80186-style PUSH imm16 not in 8086 base set); `PUSHI 041h` then `pop ax` gives AX=41h
- `ESC opcode:Db(0,63), src:Eb` — 8087 escape encoding via `SEGFIX` + `DBIT 5(1Bh),3(opcode(3))` + `MODRM opcode,src`
- `Db` vs `Dw` matters: wrong parameter type causes ERROR 7; `Db` fits -256..255, `Dw` fits any 16-bit value (see §10 gotcha #14)
- `CodeMacro` definitions must appear **before** the `cseg` / `org` directive


---

## 7. Helper Library

All helper modules are archived into `helplib.l86`. The demo program
[`exhelp.a86`](exhelp.a86) is self-contained (inline copies of every routine)
and links standalone; the library modules exist for reuse in other programs.

### 7.1 Primitives (`hlp_num`)

Source: [`hlp_num.a86`](hlp_num.a86)

| Routine | IN | OUT | Notes |
|---|---|---|---|
| `nibble_to_hex` | AL = 0..Fh | AL = ASCII | `'0'..'9'` / `'A'..'F'` |
| `byte_to_hex` | AL = byte | AH = hi ASCII, AL = lo ASCII | does not print |
| `word_to_hex` | AX = word | prints 4 hex chars | via fn 2; state in `w2h_val`/`w2h_cnt` |
| `div10` | AX = dividend | AX = quotient, DX = remainder | unsigned 16÷10; saves/restores BX |
| `word_to_dec` | AX = value | prints decimal | no leading zeros; state in `w2d_buf`/`w2d_len`/`w2d_idx` |
| `hex_to_byte` | SI → 2 hex chars | AL = byte; SI += 2 | `"FF"` → 0FFh |
| `str_to_int` | SI → ASCII decimal | AX = value; SI advanced | stops at first non-digit |

Key points:
- All state that must survive a BDOS call lives in named memory variables — never in AX or BX between `int 0E0h` calls
- `word_to_hex` emulates `ROL AX,4` as 4× `shl ax,1` + carry-feed because RASM-86 8086 mode has no immediate-count rotate
- `str_to_int` uses `mul bx` (BX=10) which clobbers DX — acceptable since no BDOS calls are made inside

### 7.2 String (`hlp_str`)

Source: [`hlp_str.a86`](hlp_str.a86)

| Routine | IN | OUT | Notes |
|---|---|---|---|
| `strlen` | SI → zstring | CX = length | SI preserved |
| `strcpy` | SI → src, DI → dst | SI, DI advanced | copies null terminator |
| `strcmp` | SI → s1, DI → s2 | AL = 0 / 1 / FFh | eq / gt / lt |
| `ltrim` | SI → string | SI → first non-space | modifies SI |
| `rtrim` | SI → string | null placed after last non-space | in-place |
| `str_contains` | SI → string, AL = char | AL = 1 / 0 | found / not found |
| `pad_right` | SI → string, CX = width | spaces appended, null-terminated | in-place; caller ensures buffer fits |
| `toupper_buf` | SI → string | in-place a-z → A-Z | `and al,0DFh` |
| `tolower_buf` | SI → string | in-place A-Z → a-z | `or al,20h` |

Key points:
- No BDOS calls — no clobber risk on AX/BX within these routines
- String convention: byte value 0 = null terminator throughout

### 7.3 I/O (`hlp_io`)

Source: [`hlp_io.a86`](hlp_io.a86)

| Routine | IN | OUT | Notes |
|---|---|---|---|
| `print_char` | AL = char | — | fn 2 |
| `print_crlf` | — | — | two fn 2 calls |
| `cls` | — | — | `ESC[2J` via fn 9 |
| `print_zstr` | SI → zstring | — | fn 2 loop; SI saved in `pzs_si` across each BDOS call |
| `read_line` | DI → buf `[maxlen][0][chars...]` | CX = actual length, text null-terminated | fn 10 wrapper |

Key points:
- `print_zstr` saves SI to memory before every `int 0E0h` and restores it after — BDOS clobbers BX which would corrupt an index register
- `read_line` buffer layout: byte 0 = max length (set by caller), byte 1 filled by BDOS with actual length, bytes 2+ = text

### 7.4 Memory (`hlp_mem`)

Source: [`hlp_mem.a86`](hlp_mem.a86)

| Routine | IN | OUT | Notes |
|---|---|---|---|
| `memset` | SI → buf, CX = count, AL = fill | — | SI, CX clobbered |
| `memcpy` | SI → src, DI → dst, CX = count | SI, DI advanced | same segment |
| `memcmp` | SI → b1, DI → b2, CX = count | AL = 0 / 1 / FFh | eq / gt / lt |

Key points:
- No BDOS calls; straightforward byte loops
- `memcmp` returns 0 (equal), 1 (buf1 > buf2), FFh (buf1 < buf2) — same convention as `strcmp`

### 7.5 File I/O — FCB Layout and Routines (`hlp_file`)

Source: [`hlp_file.a86`](hlp_file.a86)

**FCB layout (36 bytes, must be zero-initialized before use):**

| Offset | Size | Field | Notes |
|---|---|---|---|
| +0 | 1 | drive | 0 = default, 1 = A, 2 = B… |
| +1 | 8 | name | uppercase, space-padded, no null |
| +9 | 3 | ext | uppercase, space-padded, no dot |
| +12 | 4 | extent / s1 / s2 / rc | zeroed by BDOS |
| +16 | 16 | allocation map | zeroed |
| +32 | 1 | cr (current record) | set to 0 before open |
| +33 | 3 | r0 / r1 / r2 (random record) | used by random I/O and `file_open_append` |

| Routine | BDOS fn | IN | OUT | Notes |
|---|---|---|---|---|
| `fcb_init` | — | SI → FCB(36B), BX → name(8B), CX → ext(3B) | FCB zeroed, name/ext copied | pure memory; no BDOS |
| `file_open` | 0Fh | SI → FCB | AL = 0 ok / FFh err | open existing |
| `file_create` | 16h | SI → FCB | AL = 0 ok / FFh err | create or truncate |
| `file_open_append` | 0Fh + 23h | SI → FCB | AL = 0 ok / FFh err | open → compute size → set cr = R0 |
| `file_close` | 10h | SI → FCB | AL = 0 ok / FFh err | flush + close |
| `file_read` | 1Ah + 14h | SI → FCB, DI → 128B buf | AL = 0 ok / non-0 EOF/err | sets DMA offset before read |
| `file_write` | 1Ah + 15h | SI → FCB, DI → 128B buf | AL = 0 ok / non-0 err | sets DMA offset before write |
| `file_delete` | 13h | SI → FCB | AL = 0 or FFh | wildcards allowed in name/ext |

Key points:
- Call `fcb_init` before every open/create — clears cr and random record fields
- Caller must issue fn 33h (Set DMA Segment) **once at startup** pointing to the segment containing the DMA buffer (see §10 gotcha #10)
- `file_open_append`: open (fn 0Fh) → compute file size (fn 23h) → copy FCB+33 (R0) into FCB+32 (cr) — positions sequential write at EOF
- `file_read` / `file_write` set the DMA **offset** (fn 1Ah) each call; the segment is set once by the caller

### 7.6 Calling Convention — BP Stack Frame

Worked example from [`exhelp.a86`](exhelp.a86) — `bp_demo`:

```
Caller:                    Stack after call + push bp:
  mov ax, 42              SP+0 → saved BP
  push ax                 SP+2 → return address   ← [bp+0] / [bp+2]
  call bp_demo            SP+4 → arg (42)         ← [bp+4]
  add sp, 2
```

```asm
bp_demo:
        push    bp
        mov     bp, sp
        mov     ax, 4[bp]       ; fetch argument (indexed: n[reg] syntax)
        call    word_to_dec
        pop     bp
        ret
```

Key points:
- Args pushed right-to-left before `call`; callee does `push bp` / `mov bp,sp`
- First arg is at `4[bp]` (2 bytes ret addr + 2 bytes saved BP)
- Callee cleans its frame with `pop bp; ret`; caller cleans args with `add sp,N`
- Indexed memory syntax in RASM-86: `4[bp]` not `[bp+4]` (see §10 gotcha #5)

### 7.7 Command-Line Parsing (`hlp_cmd`)

Source: [`hlp_cmd.a86`](hlp_cmd.a86)

**Base-page layout (DS:80h at `org 100h` entry):**

| Address | Content |
|---|---|
| DS:80h | Length byte — total chars in tail including leading space |
| DS:81h | First char of tail (usually a space) |
| DS:82h… | Remainder of tail |

| Symbol | Type | Notes |
|---|---|---|
| `parse_cmdline` | routine | call **before** `mov ds,cs` |
| `argv1_buf` | 9-byte buffer | first token, null-terminated (max 8 chars) |
| `argv2_buf` | 9-byte buffer | second token, null-terminated (max 8 chars) |
| `argc_val` | 1-byte var | 0, 1, or 2 |

Key points:
- Must be called **before** redirecting DS to CS — it reads DS:80h/81h (base-page segment)
- Writes results via ES (= CS set by caller before the call) so buffers land in the program's own segment
- Tokens are space-delimited; each capped at 8 chars; extras ignored
- After DS redirect, read `argc_val`, `argv1_buf`, `argv2_buf` normally

### 7.8 Demo Program

Source: [`exhelp.a86`](exhelp.a86)

Self-contained single-segment demo that exercises every helper routine inline.
Links standalone — no external library needed.

Key points:
- `parse_cmdline` called first (DS still = base page); `mov cx,cs` / `mov es,cx` done before that call so `stosb` writes to the correct segment
- `mov cx,FN_SET_SEG` / `mov dx,cs` / `int BDOS` — sets DMA segment to CS once at startup before any file I/O
- File I/O sequence: `fcb_init` → `file_create` → fill DMA buffer → `file_write` → `file_close` → `fcb_init` again → `file_open` → zero DMA buffer → `file_read` → `file_close` → `file_delete`
- BP-frame demo: `mov ax,42` / `push ax` / `call bp_demo` / `add sp,2`


---

## 8. ASM86 vs RASM-86 Syntax Comparison

| Feature | RASM-86 | ASM86 | Notes |
|---|---|---|---|
| Output format | `.obj` (OMF-86) | `.h86` (Intel hex) | RASM-86 → LINK-86; ASM86 → GENCMD |
| Next tool | `pcdev_linkcmd` | `cpm86_gencmd` | — |
| Segment directives | `cseg` / `dseg` / `eseg` / `sseg` | `cseg` only | no `segment...ends` Intel syntax in either |
| Multi-segment | yes | no (single `cseg org 100h` only) | — |
| `CodeMacro` / `EndM` | yes | no | RASM-86 only |
| `RD` directive | yes | no | RASM-86 repeat-data |
| `INCLUDE` directive | no | yes | ASM86 only |
| Local symbols param | `$ sz` | `$ S+` | different parameter letter |
| Hex constant format | `0FFh` (trailing `h`) | `0FFH` or `#FF` | case-insensitive in ASM86 |
| Line endings | CRLF required | CRLF required | `unix2dos` before assembling |
| Filename limit | ≤ 8 chars (silently fails on longer) | ≤ 8 chars | see §10 gotcha #1 |
| `JMPS` / `JMPF` | `JMPS` (short), `JMPF` (far) | `JMP SHORT`, `JMP FAR` | — |
| `CALLF` / `RETF` | `CALLF`, `RETF` | `CALL FAR`, `RET FAR` | — |


---

## 9. BDOS Quick Reference

`INT 0E0h` — CL = function number — returns AL = BL — **clobbers AX and BX**.

| fn (hex) | Name | CL | DX in | Returns |
|---|---|---|---|---|
| 00h | Exit (warm boot) | 00h | — | — |
| 01h | Console input | 01h | — | AL = char |
| 02h | Console output | 02h | DL = char | — |
| 09h | Print string | 09h | DX → `'$'`-terminated string | — |
| 0Ah | Read console buffer | 0Ah | DX → `[maxlen][0][chars...]` | buf filled; `[1]` = actual len |
| 0Fh | Open file | 0Fh | DX → FCB | AL = 0 ok / FFh err |
| 10h | Close file | 10h | DX → FCB | AL = 0 ok / FFh err |
| 11h | Search first | 11h | DX → FCB (wildcards ok) | AL = 0 found / FFh not found |
| 13h | Delete file | 13h | DX → FCB (wildcards ok) | AL = 0 ok / FFh err |
| 14h | Read sequential | 14h | DX → FCB | AL = 0 ok / non-0 EOF/err |
| 15h | Write sequential | 15h | DX → FCB | AL = 0 ok / non-0 err |
| 16h | Make (create) file | 16h | DX → FCB | AL = 0 ok / FFh err |
| 1Ah | Set DMA offset | 1Ah | DX = offset of DMA buffer | — |
| 1Bh | Get allocation vector | 1Bh | — | BX → alloc vector |
| 21h | Random read | 21h | DX → FCB (r0/r1/r2 set) | AL = 0 ok / non-0 err |
| 22h | Random write | 22h | DX → FCB (r0/r1/r2 set) | AL = 0 ok / non-0 err |
| 23h | Compute file size | 23h | DX → FCB | FCB+33 = record count |
| 24h | Set random record | 24h | DX → FCB | FCB r0/r1/r2 set from cr |
| 33h | Set DMA segment | 33h | DX = segment value | — |

> **Note:** fn 33h (Set DMA Segment) must be called once at startup for `org 100h` programs before any file I/O — the default DMA segment after relocation is not the program's segment (§10 gotcha #10).


---

## 10. Gotchas

**1. Filename > 8 chars** → RASM-86 silently truncates or fails to find the file.
Keep source filenames ≤ 8 chars (e.g. `ex1sing.a86`, not `example1_single.a86`).

**2. LF-only line endings** → assembler sees garbage / empty lines.
Fix: `unix2dos sourcefile.a86` before every assemble. The Makefile pattern rule does this automatically.

**3. Forward reference to a data label** → ERROR 8 ("undefined symbol" on pass 1).
Fix: put all `db` / `rb` / `rw` data **before** the code that references it; use `jmp main` at the top of `cseg` to skip over it.

**4. `mov [label], immediate`** → ERROR 8 (ambiguous operand size).
Fix: use a register intermediary: `mov al, 42` / `mov [label], al`.

**5. Indexed addressing syntax** → ERROR or wrong encoding.
RASM-86 requires `n[reg]`, not `[reg+n]`. Write `4[bp]`, `2[si]`, not `[bp+4]`, `[si+2]`.

**6. Conditional jump out of ±128 byte range** → ERROR 22.
Fix: invert the condition and jump over an unconditional `jmp`:
```asm
    je  target          ; ERROR 22 if target is far
    ; becomes:
    jne skip
    jmp target
skip:
```

**7. Makefile `[$sz]` quoting** → `make` expands `$s` as a shell variable.
Fix: single quotes only — `'ex1sing [$sz]'`. Double quotes let the shell eat `$sz`.

**8. `org 100h`: DS ≠ CS at entry** → data access via DS reads the wrong segment.
Fix: redirect immediately — `mov cx,cs` / `mov ds,cx` / `mov es,cx` — before touching any data label.

**9. `org 100h`: command tail in base-page DS** → tail is lost after DS redirect.
Fix: call `parse_cmdline` (or read DS:80h/81h manually) **before** the DS redirect.

**10. DMA segment not set for `org 100h` programs** → file reads/writes land in wrong memory.
Fix: issue fn 33h once at startup — `mov cx,33h` / `mov dx,cs` / `int 0E0h` — before any `file_read` or `file_write` call.

**11. BDOS clobbers AX and BX** → loop counters or return values in those registers are destroyed by every `int 0E0h`.
Fix: save any value that must survive a BDOS call to a named memory variable before the call; restore after.

**12. FCB not zero-initialized / name not space-padded** → open/create fails or finds wrong file.
Fix: call `fcb_init` before every open or create; it zeros all 36 bytes and copies the space-padded name and extension.

**13. Intel `segment...ends` syntax** → not supported by RASM-86.
Use RASM-86 native directives: `cseg`, `dseg`, `eseg`, `sseg`. No `segment` / `ends` keywords.

**14. `CodeMacro` parameter type mismatch** → ERROR 7.
`Db` accepts values in -256..255 (byte-sized); `Dw` accepts any 16-bit value. Using `Db` for a value outside that range produces ERROR 7. Use `Dw` for addresses or larger immediates.

**15. `rw 0` behavior** → reserves 0 bytes but places a label at the current offset.
Useful for marking the top of a stack area: `stk_top rw 0` gives a label at the high end without allocating extra space.

**16. `mov cx,dgroup` is not valid** → RASM-86 does not allow a group name as an immediate.
Fix: use `mov ax,seg <symbol_in_group>` to load the group base into a register, then `mov ds,ax` etc.

**17. Hex constant starting with A–F** → parsed as an identifier, not a number.
Fix: always prefix with a leading `0` — `0FFh`, `0E0h`, `0ABCDh`. `FFh` alone is a label reference.

**18. fn 9 string not printed or prints garbage** → missing `'$'` terminator.
Every string printed with BDOS fn 9 must end with the byte `24h` (`'$'`). A missing terminator causes fn 9 to scan past the end of your string until it finds a `$` somewhere in memory.

**19. `MORE THAN ONE MAIN PROGRAM` from LINK-86** → two modules both have `end <label>`.
Only one module's `end` directive should name the entry point. All other modules use bare `end` (no label).

**20. `UNDEFINED SYMBOLS` from LINK-86** → a module that exports a `public` is missing from the link command, or the spelling of the `extrn` and `public` names don't match.
Fix: list all required `.obj` files (and `.l86[search]` libraries) in the `pcdev_linkcmd` command.

**21. Ambiguous memory operand size** → ERROR 21 (`MISSING TYPE INFO`).
`mov [bx], 10` — RASM-86 can't infer byte vs word. Fix: load into a sized register first: `mov al, 10` / `mov [bx], al`.

**22. ASM86 rejects `end <label>`** → `GARBAGE AT END OF LINE` error.
ASM86 does not accept a start label on the `END` directive. Use bare `end` — entry is always the first code byte. This differs from RASM-86 where `end main` names the entry point.

**23. ASM86 dual-segment: `dseg` without `org 100h`** → garbled output, wrong data.
The CP/M-86 loader reserves DS:0–FFh for the base page (FCBs, command tail, etc.).
Without `org 100h` in the `dseg`, data starts at DS:0 and overlaps the base page.
Fix: add `org 100h` immediately after `dseg`, exactly as `cseg org 100h` does for the single-segment model.
