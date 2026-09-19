# CP/M-86 Debuggers — Command Reference

Quick reference and comparison for the two Digital Research debuggers shipped in
`share/cpm/`: **DDT-86** and **SID-86**. Both are CP/M-86 `.cmd` programs run
directly under `emu2`.

---

## 1. Invocation

**Via the `bin/` wrapper** (SID-86 only — the wrapper derives the CP/M program
name from its own basename, so a `cpm86_ddt86` copy would work identically):

```sh
cpm86_sid86 <prog>.cmd
```

**Directly under emu2** (required for DDT-86, and for any locally built emulator):

```sh
emu2 ../../share/cpm/ddt86.cmd <prog>.cmd
emu2 ../../share/cpm/sid86.cmd <prog>.cmd
```

Both accept an optional `.cmd` filename on the command line. Omit it and load
later with `E<filespec>`.

**Startup display**

| | DDT-86 | SID-86 |
|---|--------|--------|
| Banner | `DDT86 1.2` | 6-line boxed banner with serial number |
| Prompt | `-` | `#` |
| Segment table | `CS`/`DS`/`SS` (+ `ES`, `X1`–`X4` when present) | `CS`/`DS`/`SS` |
| Break indicator | `*ssss:oooo` | `*ssss:oooo` |

---

## 2. DDT-86 — `share/cpm/ddt86.cmd`

The original CP/M-86 debugger. 8086 disassembler, no symbolic support.

**Command set**

| Cmd | Syntax | Purpose |
|-----|--------|---------|
| `A` | `As` | Assemble instructions into memory at `s` |
| `D` | `D` · `Ds` · `Ds,f` | Display memory in hex + ASCII |
| `E` | `E<filespec>` | Load program for execution (resets segment registers) |
| `F` | `Fs,f,b` | Fill memory `s`–`f` with byte `b` |
| `G` | `G` · `Gs` · `G,b1,b2` · `Gs,b1,b2` | Go, with up to two temporary breakpoints |
| `H` | `Ha,b` | Hexadecimal sum and difference |
| `I` | `I<tail>` | Set command tail at `0080h` and FCBs at `005Ch`/`006Ch` |
| `L` | `L` · `Ls` · `Ls,f` | List (disassemble) memory |
| `M` | `Ms,f,d` | Move memory block |
| `R` | `R<filespec>` | Read disk file into memory |
| `S` | `Ss` | Set (patch) memory bytes interactively |
| `T` | `T` · `Tn` | Trace 1 (or `n`) instructions, displaying each |
| `U` | `U` · `Un` | Untraced monitor — run `n` instructions, display only the last |
| `V` | `V` | Show segment values of the last loaded file |
| `W` | `Ws,f` | Write memory block to disk file |
| `X` | `X` · `Xr` · `Xf` | Examine / modify CPU state |

> **Note:** DDT-86 splits the `X` register display across two lines
> (`AX…BP`, then `SI…IP`), unlike SID-86's single wide line.

---

## 3. SID-86 — `share/cpm/sid86.cmd` / `cpm86_sid86`

Symbolic Instruction Debugger, 1985. Superset of DDT-86: adds a 286
disassembler, symbol-file support, macros, pass points and search.

**Command set**

| Cmd | Syntax | Purpose |
|-----|--------|---------|
| `?` | `?` · `??` | Help · command formats |
| `:` | `:name` | Define a macro (prompts for body) |
| `=` | `=name` · `=` | Invoke macro · list all macros |
| `A` | `As` | Assemble into memory |
| `B` | `Bs,f,d` | Block compare |
| `D` | `D` · `Ds` · `Ds,f` · `-Dn` | Display memory · set default display length |
| `E` | `E<filespec>` | Load program **and** symbol file(s) |
| `F` | `Fs,f,b` | Fill memory |
| `G` | `G` · `Gs` · `G,b1,b2` · `Gs,b1,b2` | Go, with optional temporary breakpoints |
| `H` | `Ha,b` | Hexadecimal arithmetic |
| `I` | `I<tail>` | Set up program arguments |
| `L` | `L` · `Ls` · `Ls,f` | List memory (286 disassembler) |
| `M` | `Ms,f,d` | Move memory block |
| `P` | `Ps,n` · `P` · `-Ps` | Set pass point with count · list · remove |
| `Q` | `QIport` · `QOport,b` | Direct I/O request |
| `R` | `R<filespec>` | Read disk file |
| `S` | `Ss` · `SRs,f,pattern` | Set memory · search |
| `T` | `T` · `Tn` | Trace (single-step execution) |
| `U` | `U` · `Un` | Untraced single-step |
| `V` | `V` | Verify values of last file loaded |
| `W` | `Ws,f` | Write disk file |
| `X` | `X` · `Xr` | Examine / modify CPU state |

---

## 4. Side-by-side comparison

| Feature | DDT-86 | SID-86 |
|---------|--------|--------|
| Prompt | `-` | `#` |
| Disassembler | 8086 | 80286 |
| Symbolic debugging | No | Yes (`.SYM` files via `E`) |
| Temporary breakpoints | `G,b1,b2` | `G,b1,b2` |
| Permanent breakpoints | — | `P` pass points, with pass counts |
| Macros | — | `:name` / `=name` |
| Block compare | — | `B` |
| Memory search | — | `SR` |
| Direct port I/O | — | `Q` |
| Default display length | fixed | `-D` configurable |
| Online help | — | `?` / `??` |
| `X` display | two lines | one wide line |

**Common to both:** `A D E F G H I L M R S T U V W X` — the DDT-86 command set is
a strict subset of SID-86's, with identical syntax. Anything you learn on DDT-86
transfers directly.

---

## 5. Address and operand syntax

Addresses are `segment:offset`, both hex, e.g. `10A0:000A`. A bare offset uses
the command's current default segment.

| Form | Meaning |
|------|---------|
| `10A0:000A` | Explicit segment and offset |
| `CS:0` | Segment register as the segment |
| `000A` | Offset in the current default segment |
| `s,f` | Start and finish (inclusive) for range commands |

> **Gotcha:** operands must follow the command letter with **no space**.
> `D DS:100` is parsed as a bare `D` and dumps from the default pointer;
> `DDS:100,12F` is what you want. Same for `L`, `S`, `F`, `M`.

> **Gotcha:** the default segment follows `CS`. If a trace leaves your program —
> for example into an interrupt handler — a subsequent `G,E` resolves against
> *that* segment, not your code. Always write `LCS:0,F` and `G` breakpoints with
> an explicit segment when in doubt.

---

## 6. Worked example

Debugging [examples/helloa.a86](../../examples/helloa.a86), a 16-byte program.
Hand-assembling gives the breakpoint addresses without needing a symbol file:

| CS offset | Bytes | Source |
|-----------|-------|--------|
| `0000` | `8C D9` | `mov cx,ds` |
| `0002` | `8E C1` | `mov es,cx` |
| `0004` | `B9 09 00` | `mov cx,09h` |
| `0007` | `BA 00 01` | `mov dx,offset msg` |
| `000A` | `CD E0` | `int 0E0h` — BDOS print string |
| `000C` | `33 C9` | `xor cx,cx` |
| `000E` | `CD E0` | `int 0E0h` — BDOS exit |

**Session** (identical under both debuggers; `-` prompt shown for DDT-86):

```
-LCS:0,F        ; confirm the disassembly
-G,A            ; run, break before the print call
-X              ; CX=0009, DX=0100
-DDS:100,12F    ; the '$'-terminated message
-T              ; step the INT E0 — message appears
-T              ; step XOR CX,CX
-G              ; run to exit
```

**The CP/M-86 base page** at `DS:0000` is worth knowing — `DDS:0` shows 6-byte
group descriptors that mirror the startup segment table:

| Offset | Contents |
|--------|----------|
| `00` / `03` | Code length / CS base |
| `06` / `09` | Data length / DS base |
| `0C` / `0F` | Extra group length / base |
| `12` / `15` | Stack length / SS base |
| `5C` / `6C` | FCB 1 / FCB 2 (blank-filled when no arguments) |
| `80` | Command tail (length byte + text) — populated by `I` |

---

## 7. emu2 notes

Both debuggers drive the same 8086 mechanisms, so they exercise the emulator
identically:

| Mechanism | Used by | emu2 requirement |
|-----------|---------|------------------|
| `INT 3` (opcode `CC`) | `G,addr`, `P` | Vector at `0000:000C`, writable low memory |
| `INT 1` (trap flag) | `T`, `U` | Vector at `0000:0004`, TF honoured on `POPF`/`IRET` |
| `INT E0` | program BDOS calls | Handled natively by the emulator |

> **Gotcha:** emu2 implements the BDOS in C. `INT E0` vectors to a synthetic trap
> byte in segment 0 (`0000:00E0` and neighbours — see `next_instruction()` in
> [archive/emu2/src/cpu.c](../../archive/emu2/src/cpu.c)). There is no real BDOS
> code to trace into: `L0000:00E0` disassembles garbage. On real CP/M-86 you could
> step through the BDOS; here you cannot.

> **Fixed:** `trap_1()` used to raise the single-step interrupt unconditionally
> after every traced instruction. A real 8086 clears TF while *taking* an
> interrupt, which suppresses that trap — so tracing an `INT` must step over the
> call, not halt at the handler's first byte. Without the guard, `T` on `INT E0`
> stopped at `0000:00E0`, SID's default segment followed `CS` to `0000`, and every
> subsequent address resolved against the interrupt vector table. It also caused a
> nested trap inside the debugger's own `INT 1` handler.

**Verified working under emu2** (both debuggers, using
[examples/helloa.a86](../../examples/helloa.a86)):

| Command | Status |
|---------|--------|
| `L` disassembly | Works |
| `G,addr` breakpoint | Works — reports `*ssss:oooo` |
| `X` register display | Works |
| `D` memory dump | Works |
| `T` single-step, including over `INT E0` | Works |
| `G` run to completion | Works |

Not yet exercised: `A`, `F`, `M`, `R`, `W`, `S`, `I`, `U`, `V`, and SID-86's
`B`, `P`, `Q`, `SR`, macros and `.SYM` loading. `Q` (direct port I/O) is unlikely
to do anything useful — emu2 has no port space for CP/M-86 programs.
