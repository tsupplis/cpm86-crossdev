# Microsoft BASIC Patch Notes

This repository includes two Microsoft BASIC binaries, both patched:

| File | Version | Target | Patch 1 offset | Patch 2 offset |
|---|---|---|---|---|
| `mbasic86.cmd` | 5.22 | CP/M-86 | `0x1B51` | `0x7121` |
| `mbasic86.com` | 5.28 | DOS     | `0x1C47` | `0x7B87` |

The unpatched originals are kept alongside for reference:

| File | Description |
|---|---|
| `mbasic86.org` | Original 5.22 CP/M-86 binary |
| `mbasorig.com` | Original 5.28 DOS binary |

---

## Patch 1 — `USR()` / `DEF USR` bug fix

`DEF USR` stores a user-supplied machine-language routine address in an
internal table.  Each slot is initialised to `0xFFFF` to mark it as
"undefined".  When `USR(n)` is called, BASIC checks whether the slot still
holds the sentinel before jumping to the routine.

The original check in both binaries reads:

```
CMP DX, -1     ; opcode 83 FA FF — word compare, sign-extended imm8
JNE <call>     ; jump to routine if slot is defined
```

Both patched versions correct the encoding to an explicit `imm16` word compare:

```
CMP DX, 0xFFFF ; opcode 81 FA FF FF — word compare, explicit imm16
JNE <call>
```

While both `83 FA FF` and `81 FA FF FF` are logically equivalent for the value
`0xFFFF`, the explicit form is unambiguous and matches the sentinel exactly as
stored.

| Binary | Original | Patched | Instruction |
|---|---|---|---|
| `mbasic86.cmd` (5.22 CP/M-86) | `80 FA FF` | `83 FA FF` | `CMP DL/DX, 0xFF/-1` |
| `mbasic86.com` (5.28 DOS)     | `83 FA FF` | `81 FA FF FF` | `CMP DX, -1/0xFFFF` |

Note: the 5.22 CP/M-86 original (`mbasic86.org`) had the more severe byte
compare bug (`80 FA FF` — `CMP DL, 0xFF`), which only checked the low byte.
The 5.28 DOS original (`mbasorig.com`) already used a word compare (`83 FA FF`)
but with sign-extended encoding; the patch upgrades it to the explicit form.

This matches the analysis published on win3x.org (user *gm86*), who identified
the same fix in the 5.22 CP/M-86 build at offset `0x1C51` of their distribution
copy.

## Patch 2 — Version string

Both binaries have a cosmetic version string change to identify the patched
build:

| Binary | Original | Patched |
|---|---|---|
| `mbasic86.cmd` (5.22 CP/M-86) | `[CPM/86 Version]` | `[CPM-86 Patched]` |
| `mbasic86.com` (5.28 DOS)     | `[DOS Version]`    | `[DOS Patched]`    |
