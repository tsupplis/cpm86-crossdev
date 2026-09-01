# Microsoft BASIC 5.22 for CP/M-86 — Patch Notes

The file `mbasic86.cmd` is a patched version of the original `mbasic86.org`
(sourced from the CP/M-86 kernel distribution). Two bytes are changed.

## Patch 1 — BDOS error check bug fix (offset 0x1B51)

| | Value | Instruction |
|---|---|---|
| Original | `80 FA FF` | `CMP DL, 0xFF` |
| Patched  | `83 FA FF` | `CMP DX, -1`   |

The original code checks only the low byte (DL) of the BDOS return value
against `0xFF`. Under CP/M-86, BDOS file operations return error codes as a
full 16-bit value in DX — `0xFFFF` signals failure. If DH is non-zero (which
can happen depending on register state), the byte compare misses the error
entirely, causing BASIC to proceed as if the operation succeeded.

Changing opcode `0x80` to `0x83` switches from `CMP r/m8, imm8` to
`CMP r/m16, imm8` (sign-extended), so the full word `0xFFFF` is correctly
tested. This fixes crashes or incorrect behaviour when opening, closing or
reading files from BASIC on CP/M-86.

## Patch 2 — Version string (offset 0x7121)

| | String |
|---|---|
| Original | `[CPM/86 Version]` |
| Patched  | `[CPM-86 Patched]` |

Cosmetic change to identify the patched binary and normalise the separator
from `/` to `-` (the canonical CP/M-86 spelling).
