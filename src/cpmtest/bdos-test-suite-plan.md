# CP/M-86 BDOS Test Suite — Plan

Test suite for CP/M-86 BDOS functions implemented in emu2.
All tests run under `emu2` or on real CP/M-86 hardware.

## Directory

`src/cpmtest/`

## Naming Conventions

| Item | Pattern | Example |
|---|---|---|
| Test binary | `tst{HHH}{YY}.cmd` | `tst01401.cmd` = fn 014h test-set 01 |
| Source | `tst{HHH}{YY}.a86` | |
| Sub-test ID in output | `HHH.NN` | `014.01` |
| Artifact files | `tst{NNNNN}.out` | `tst14buf.out` |

## Output Format

Every sub-test emits exactly:
```
INF: test HHH.NN started (mnemonic)
INF: test HHH.NN successful
```
or on failure:
```
ERR: test HHH.NN failed
```

Sub-tests are independent — failure prints ERR and continues; no early exit.
Exit always via fn 000h (`xor cx,cx` / `int 0E0h`).

## Code Conventions

- Single segment `cseg` / `org 100h`
- Entry: `mov cx,cs` / `mov ds,cx` / `mov es,cx`
- fn 033h (F_DMASEG) called at startup to redirect DMA to CS
- Labels `lower_snake_case`, constants `UPPER_CASE`, mnemonics lower-case
- Only `BDOS equ 0E0h` + specific function equates used
- No `mov var, immediate` — use AL/AX as intermediary
- `n[si]` not `[si+n]`; always `mov bx, offset fcb` before FCB access
- No `dup` — use explicit `db` lines

## Test Status

| File | Fn | Symbol | Sub-tests | Artifacts | Status |
|---|---|---|---|---|---|
| tst00001 | 000h | P_TERMCPM | 1 | — | ✅ pass |
| tst00201 | 002h | C_WRITE | 2 | — | ✅ pass |
| tst00901 | 009h | C_WRITESTR | 2 | — | ✅ pass |
| tst00b01 | 00Bh | C_STAT | 1 | — | ✅ pass |
| tst00c01 | 00Ch | S_BDOSVER | 2 | — | ✅ pass |
| tst00d01 | 00Dh | DRV_ALLRESET | 1 | — | ✅ pass |
| tst00f01 | 00Fh | F_OPEN | 2 | — | ✅ pass |
| tst01001 | 010h | F_CLOSE | 1 | — | ✅ pass |
| tst01101 | 011h | F_SFIRST | 4 | — | ✅ pass |
| tst01201 | 012h | F_SNEXT | 2 | — | ✅ pass |
| tst01301 | 013h | F_DELETE | 4 | — | ✅ pass |
| tst01401 | 014h | F_READ | 3 | tst14buf.out | ✅ pass |
| tst01501 | 015h | F_WRITE | 8 | tst15seq.out | ✅ pass |
| tst01502 | 015h | F_WRITE text/^Z | 5 | tst52txt.out | ✅ pass |
| tst01601 | 016h | F_MAKE | 2 | — | ✅ pass |
| tst01701 | 017h | F_RENAME | 2 | — | ✅ pass |
| tst01801 | 018h | DRV_LOGINVEC | 1 | — | ✅ pass |
| tst01901 | 019h | DRV_GET | 1 | — | ✅ pass |
| tst01a01 | 01Ah | F_DMAOFF | 1 | tst1adma.out | ✅ pass |
| tst01b01 | 01Bh | DRV_ALLOCVEC | 1 | — | ✅ pass |
| tst01d01 | 01Dh | DRV_ROVEC | 1 | — | ✅ pass |
| tst01e01 | 01Eh | F_ATTRIB | 2 | — | ✅ pass |
| tst01f01 | 01Fh | DRV_DPB | 2 | — | ✅ pass |
| tst02f01 | 02Fh | P_CHAIN | 1 | — | ✅ pass |
| tst02001 | 020h | F_USERNUM | 2 | — | ✅ pass |
| tst02101 | 021h | F_READRAND | 4 | tst21rnd.out | ✅ pass |
| tst02201 | 022h | F_WRITERAND | 6 | tst22rnd.out | ✅ pass |
| tst02301 | 023h | F_SIZE | 1 | tst23siz.out | ✅ pass |
| tst02401 | 024h | F_RANDREC | 2 | tst24rec.out | ✅ pass |
| tst03201 | 032h | S_BIOS | 2 | — | ✅ pass |
| tst03301 | 033h | F_DMASEG | 1 | tst01seg.out | ✅ pass |
| tst03501 | 035h | MC_MAX | 2 | — | ✅ pass |
| tst03701 | 037h | MC_ALLOC | 2 | — | ✅ pass |
| tst03901 | 039h | MC_FREE | 2 | — | ✅ pass |
| tst03a01 | 03Ah | MC_ALLFREE | 1 | — | ✅ pass |
| tst09801 | 098h | F_PARSE | 4 | — | ✅ pass |

**Total: 36/36 passing**

> Helper: `sub02f02.cmd` — P_CHAIN target (built by `make all` via `HELPERS`, not in `TESTS`; `tst02f01.cmd` depends on it)

## Deferred / Not Implemented in emu2

| Fn | Symbol | Reason |
|---|---|---|
| 00Eh | DRV_SET | Drive select — deferred (needs known drive layout) |
| 020h set | F_USERNUM set | emu2 always returns 0; set is accepted but silently ignored |
| 036h | MC_ABSMAX | Returns hard error AX=0xFFFF, CX=3 — not supported |
| 038h | MC_ABSALLOC | Returns hard error AX=0xFFFF, CX=3 — not supported |
| 028h | F_WRITEZF | ❌ not implemented in emu2 |
| All MP/M-only | — | ❌ not implemented in emu2 |
| 02Fh | P_CHAIN | Interactive — cannot run non-interactively |
| 069h | T_GET | Clock-dependent — output not deterministic |
| 09Bh | T_SECONDS | Clock-dependent — output not deterministic |

## Fixture Files

| File | Contents | Used by |
|---|---|---|
| `tst014aa.in` | 128 bytes of 0x41 ('A') | tst00f01, tst01001, tst01401, tst01a01, tst03301 |
