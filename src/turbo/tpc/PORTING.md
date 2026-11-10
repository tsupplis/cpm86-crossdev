# Porting the Turbo command-line patches

## Scope and confidence

These are version-specific binary patches, not a generic Turbo executable
format. Do not apply a patch to another release, an installed/customized
image, or an 8087 variant by merely changing its filename or SHA-256.

The current patches were located by tracing startup and the IDE dispatch
table: the editor handler supplies the replacement space, while the compile
handler and generated-runtime area are retained. The validation has two
independent parts:

1. **Byte preservation:** input SHA-256, unchanged image length, and a
   byte-for-byte comparison outside the explicitly allowed patch windows.
   The guards also decode hook jumps and check their destinations.
2. **Behavior:** compile and execute COM/CMD programs, load CHN through a
   launcher, preserve shared variables, exercise includes, procedures, heap,
   floating-point math and file I/O, and check diagnostic paths. CP/M tests
   also inspect the emitted CMD memory descriptors.

This is **not a complete control-flow/reachability proof**. A byte guard can
prove that a retained routine was not modified; it cannot prove that every
byte in a selected editor window is unreachable from every compiler option.
Indirect calls, uncommon directives, overlays, terminal configurations and
error paths need additional tracing/tests. Installer compatibility currently
means layout preservation, not a verified TINST installation round-trip.

## Current patch map

Addresses below are code-segment offsets. Ranges are half-open: `[start,end)`.

| Purpose | DOS 3.02A | CP/M-86 3.01A |
|---|---|---|
| Code address to file offset | `address - 0100h` | `address + 0080h` |
| Original image length | 39731 bytes | 37760 bytes |
| Skip terminal-screen initialization | none | `[280Eh,2817h)` |
| Startup to CLI, 3-byte jump | `2E29h` | `285Bh` |
| IDE-loop failure exit, 3-byte jump | `2F64h` | `299Bh` |
| File-error/prompt exit, 3-byte jump | `3478h` | `2CB4h` (common ESC wait) |
| Compiler-error exit, 3-byte jump | `3A96h` | `3485h` |
| Editor replacement window | `[40CDh,5000h)` | `[398Ah,4900h)` |
| Load workfile | `33D9h` | `2D9Ah` |
| Compile and write output | `3912h` | `32E2h` |
| Console callback pointer in DS | `013Ch` | `0228h` |
| Output-mode byte in DS | `034Bh` | `0385h` |
| Code/data minimum words in DS | `0270h`, `0272h` | `02DFh`, `02E1h` |
| Free-memory min/max words in DS | `0274h`, `0276h` | `02E3h`, `02E5h` |
| Code/data runtime paragraph overhead | `02D5h`, `0025h` | `0278h`, `002Dh` |

The input named `turbo200.com` identifies itself as **2.00B**.
The current user-supplied CDOS image has SHA-256
`fad63d86ab63aa3267667167b59bb50b7f7381da249794025c2d5d06cc551473`.
It retains `00 00` at `02FCh` in place of `INT 10h`, as explicitly requested.
Those bytes execute `ADD [BX+SI],AL`, not NOPs: memory-write side effects
remain possible in both compiler startup and generated runtime. Passing
fixtures do not establish that this modification is generally safe.

Its separate `dospc200.asm` patch uses the following COM addresses:

| Purpose | DOS 2.00B (`dos200`) |
|---|---|
| Original image length | 36480 bytes |
| Startup / IDE-loop / file-error / compiler-error hooks | `28E4h`, `29FEh`, `2D0Eh`, `356Fh` |
| Editor replacement window | `[39A9h,3E00h)` |
| Load workfile / compile and write | `2DFDh`, `33C6h` |
| Console callback / output mode in DS | `00A9h`, `023Eh` |
| Code/data minima in DS | `019Eh`, `01A0h` |
| Free-memory min/max in DS | `01A2h`, `01A4h` |
| Code/data runtime paragraph overhead | `0283h`, `0019h` |
| Termination | `INT 20h`; explicit failure text, no status code |

The terminal MS-DOS 3.01A variant (`turms301.com`, `dmspc301.asm`) has
38144 bytes and uses COM address-to-file bias `-0100h`:

| Purpose | `dms301` |
|---|---|
| Terminal initialization bypass | `[2AE9h,2AEFh)`, six NOPs |
| Startup / IDE-loop / file-error / compiler-error hooks | `2B30h`, `2C70h`, `3163h`, `3781h` |
| Editor replacement window | `[3DB8h,4CEBh)` |
| Load workfile / compile and write | `30C4h`, `35FDh` |
| Keyboard-status / console callback in DS | `0114h`, `0118h` |
| Output-mode byte in DS | `0327h` |
| Code/data minima in DS | `024Ch`, `024Eh` |
| Free-memory min/max in DS | `0250h`, `0252h` |
| Code/data runtime paragraph overhead | `02A6h`, `0022h` |
| Termination | Close original file lists, restore divide vector, `INT 20h` |

The CLI keyboard-status callback returns false in AL and consumes its
reserved result byte (`RET 1`); this avoids terminal polling without
unbalancing the compiler stack. Generated runtime callbacks remain original.

`check-image.sh` verifies the entire complement of these windows, not just
several selected runtime bytes. NASM's `original_to` macro copies original
bytes up to each boundary; a payload that crosses the next boundary cannot
be assembled with that copy length.

The editor replacement windows are allocation limits, not claims that all
their bytes were changed. Only the assembled driver and its data replace
bytes; the remaining image is copied unchanged.

## 1. Inventory the candidate

Keep the candidate read-only and use separate output names. Record:

| Evidence | Required detail |
|---|---|
| Identity | Exact release, platform, standard/8087/BCD variant, SHA-256 |
| Format | COM or CMD; don't infer it solely from the filename |
| Layout | Code/data origin, stored lengths, allocation sizes, fixups |
| Installer | Matching TINST version and required compiler filename |
| Runtime | Software floating point or hardware 8087; overlays/resources |

Read-only inspection, from this directory:

```sh
file turbo-87.com
shasum -a 256 turbo-87.com
ndisasm -b 16 -o 0x100 turbo-87.com > dos87.dis

../../../bin/cmdinfo turbo-87.cmd
shasum -a 256 turbo-87.cmd
```

For CMD, extract in a dedicated scratch directory using
`cmdinfo -e /absolute/path/to/turbo-87.cmd`. It creates group files named
`c<index>-<base>.bin`, `d<index>-<base>.bin`, etc. Disassemble the extracted
code using the origin established by the group and startup, not the COM
origin `0100h`.

The current CMD happens to have a 128-byte header, one 37264-byte code
group, a 256-byte initialized data group, and no fixups. Establish those
facts again for the candidate; do not assume the same file bias.

## 2. Recover the entry points and data contract

Trace from the initial jump, not by linearly treating all bytes as code.
Turbo interleaves instructions, tables, string literals and symbol data.
Some print helpers consume a zero-terminated literal immediately after
their call; ordinary disassembly will misinterpret the literal as code.

Build a candidate-specific equivalent of the patch-map table:

1. Find DS, SS, SP and free-memory initialization.
2. Find defaults for source buffers and the four numeric options.
3. Decode the main menu's `E` and `C` dispatch entries to locate editor and
   compile handlers. Check the compile handler's callees and error exits.
4. Trace filename parsing, source loading, include handling, output creation,
   close/delete paths, and callback setup.
5. Trace the compiler output writer: identify exactly which original bytes
   it copies into generated programs and which fields it specializes.
6. Locate terminal callbacks and the native termination routine.

Record each reused routine's register/segment assumptions, stack convention,
arguments, results, side effects and non-returning exits. A matching string
or similar opcode sequence alone is not sufficient.

DOS reads the PSP command tail via `CS:80h`. The current CP/M image reads the
base-page tail via `DS:80h` and copies it into private CLI storage. CP/M source
names use FCBs, not DOS ASCIIZ paths. Verify these conventions on the variant.

The IDE stores nonzero code/data minima *including* runtime overhead, while
its displayed values exclude that overhead. Recover both overhead constants
and limits from the candidate's option handlers; do not copy the existing
`-sc`/`-sd` limits. Preserve the special `0000` value.

## 3. Establish safe replacement space

Before placing code in an editor range:

- Trace direct calls, branches, dispatch tables and callback assignments
  into that range from startup, compiler, include handling and diagnostics.
- Resolve indirect references where possible; list unresolved references.
- Keep runtime templates, compiler/symbol tables, configuration identifiers
  and installer-visible offsets intact.
- Keep retained routines at their original code offsets. A uniform address
  delta between versions is not a valid general relocation strategy.
- Set a hard end boundary and check it at assembly time.

If sufficient space is not demonstrably available, stop and redesign instead
of overwriting the next apparently unused region.

For a CMD group replacement, work only on a new copy. `cmdmod -s code.bin
output.cmd 0` replaces group 0, with 16-byte padding. Its `-b` value uses
paragraphs; `-n` and `-m` use **bytes**. Replacing or growing a group can alter
later groups' file positions, memory allocation, fixups and installer
expectations. Review those effects explicitly and compare `cmdinfo` before
and after. The existing patches avoid all group-layout changes.

## 4. Implement a separate variant target

Create a separate patch source, for example `dospc302-87.asm` or
`c86pc301-87.asm`, with its own:

- expected input SHA-256 and exact length;
- address map, data fields and runtime-overhead constants;
- native console and termination handling;
- patch-site allowlist and hook-destination checks;
- output name and Makefile build/test targets.

Use a distinct platform/version key (`dos200`, `dos302`, `dms301`, `c86301`) in the Makefile
and image-guard scripts. Keep the corresponding input/output variables
version-qualified (`DOS200_INPUT`, `DOS200_OUTPUT`, etc.) and retain each
suite's generated files under `tests/<os>/<version>/`. Register the variant
with `compiler_rules` and add a version-specific CLI target; do not make a
new version reuse an existing version's address allowlist.

Do not weaken the existing input guards to accept a renamed candidate.
Compile and test through Makefile targets, not ad-hoc compiler commands.

Preserve native error behavior while removing interactive waits. On DOS,
propagate nonzero exit status where the target DOS supports it. For DOS 1,
use `INT 20h` with CS pointing at the PSP and explicit failure text instead.
CP/M-86 1.x has no DOS-style command return
code; retain explicit console failure markers and do not call an unsupported
status service or leave the process resident.

## 5. Validate in increasing scope

| Gate | Required evidence |
|---|---|
| Image safety | Exact length; all changed bytes confined to reviewed windows |
| Guard self-tests | Reject a modified runtime/header byte, bad hook and wrong length |
| Container | Unchanged CMD descriptors/fixups unless a reviewed redesign requires otherwise |
| CLI | All switches, missing values, invalid hex, limits, conflicting modes, overlong tails |
| Diagnostics | Missing source/include, syntax errors, output errors; no IDE/editor wait |
| COM/CMD | Compile, confirm it was not run by compilation, execute, compare exact result |
| CHN | Compile child and launcher, enter via `Chain`, verify shared data and non-return |
| Runtime | Includes, procedures, heap, real arithmetic, file I/O; more features as required |
| Options | Inspect emitted headers for zero, ordinary and boundary sizes |
| Installer | Configure a copy using matching TINST, then re-run compilation and runtime tests |
| Differential | Where practical, compile identical fixtures with original IDE and patch, compare output and behavior |

Existing reusable targets:

```sh
make check-images
make check-com
make check-chain
make check
make check-dos200
make check-dos302
make check-c86301
```

The Pascal fixtures are stored as ordinary text and converted to **CRLF**
in their per-OS/version directories under `tests/dos/200/`,
`tests/dos/302/` and `tests/c86/301/`.
Generated binaries, result files and compiler/runtime logs are retained
there after checks. `check-cmd.sh` runs the CP/M CLI suite; the parameterized
`check-chain.sh` runs once for each compiler version. Native Turbo input should use CRLF;
CP/M record files may additionally use `1Ah` EOF/padding.

For installer testing, stage the patched compiler under TINST's expected
`TURBO.COM`/`TURBO.CMD` name in a separate directory. Never configure the
pristine build input in place. Compare before/after bytes against the
candidate's established configuration region, then repeat the runtime tests.
The current exact-input SHA guards intentionally reject configured originals.

## 6. Additional requirements for TURBO-87

An 8087 variant is a separate compiler/runtime, even when much of its IDE
looks identical. The floating-point entry points, generated instruction
sequences, runtime size, startup checks and data layout may differ.

1. Re-establish the complete map and protected regions.
2. Verify that the execution environment actually supports the emitted x87
   instructions. Passing the software-float variants under emu2 does not
   establish this.
3. Execute real arithmetic, conversions, transcendental functions and
   representative numeric edge cases on an x87-capable environment.
4. Execute both standalone and chained programs under that environment.
5. Keep build/image checks separate from execution checks. If no suitable
   environment is available, mark x87 runtime validation **UNVERIFIED**,
   not passed.

No TURBO-87 image has been patched or certified by the current test suite.
