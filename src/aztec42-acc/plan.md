# acc: execution plan

Comprehensive plan for the next round of `acc` work. Companion documents:

- [design.md](design.md): engineering rationale and backend reference.
  It is reconciled with this plan as phases land (see Phase 7).
- `MANUAL.md` (to be created in Phase 1): the end-user reference. It grows
  progressively, one section per phase, and is the only document that
  `-help` points users to.

Facts marked **verified** were checked against the shipped Aztec C86
v4.10d binaries under emu2 or against the Aztec C86 4.10 manual. Nothing
in this plan relies on unverified backend behaviour; unverified items are
explicitly gated behind a probe.

---

## 1. Principles

1. **One output per source, one executable per link.** `-c`/`-S` accept
   one or more sources and produce one output per source. A link produces
   exactly one executable.
2. **No blind forwarding.** Every backend flag is either managed by a
   public option, explicitly allow-listed through `-Wc,`/`-Wa,`/`-Wl,`, or
   rejected with a clear message.
3. **Compilation is OS-neutral.** The target OS matters only at link time
   unless the user explicitly asks for it at compile time.
4. **The driver defines exactly two kinds of macro: OS and CPU.**
   Everything else is native to the compiler or lives in `azdefs.h`.
5. **One spelling per fact.** No alias macros, no double-dash options, and
   no second indirection mechanism next to `.inp`.
6. **Portable and self-hosting.** The same C89 sources build warning-free
   on Unix-like hosts and with Aztec C 4.2 on DOS 2.x, and `acc` can build
   itself.
7. **Testable by construction.** Planning is separate from execution, and
   `-###` is a stable, documented oracle that drives a golden test suite.

---

## 2. Invocation model

| Mode | Trigger | Positional inputs | Output | Default output name |
| --- | --- | --- | --- | --- |
| Compile | `-c` + `.c` | one or more `.c`/`.asm` | one `.o` per source | `<base>.o` |
| Assemble | `-c` + `.asm` | one or more `.c`/`.asm` | one `.o` per source | `<base>.o` |
| Compile to assembly | `-S` | one or more `.c` | one `.asm` per source | `<base>.asm` |
| Link | neither `-c` nor `-S` | **either** any mix of `.c`/`.asm`/`.o`/`.lib` **or** exactly one `.inp` | `.cmd`/`.exe`/`.com` | `a.cmd` without `-target`; `a.cmd` (cpm86) or `a.exe` (dos1/dos2) with it |
| Overlay root link | `-overlay-root=CODE,DATA` | as Link | `.cmd`/`.exe` + sibling `.rsm` | explicit `-o` required |
| Overlay link | `-overlay=ROOT.rsm` | as Link | `.ovr` | explicit `-o` required |

Rules:

- **Several sources with `-c`/`-S`** (current behaviour, kept): each
  source is compiled or assembled by its own backend command, in command
  line order, and published beside the invocation as `<base>.o` or
  `<base>.asm`. Rules:
  - `-o` names the output only when there is exactly one source; with
    several sources it is an error;
  - two sources with the same base name are an error (their outputs would
    collide);
  - objects, libraries and `.inp` files are not valid inputs with
    `-c`/`-S`;
  - all-or-nothing: every source is compiled first, and outputs are
    published only after all of them succeed. A failure leaves every
    existing output untouched.
- **Mixed link** (`acc -o t.exe a.c b.o`): sources are compiled first into
  the private workspace, then everything is linked. Intermediate objects
  are never published. This keeps today's behaviour.
- **`.inp` link-list file**: a plain list of `.o`/`.lib` paths. The rules
  are:
  - one path per line, relative to the invocation directory;
  - LF or CRLF line endings, blank lines ignored;
  - no options, comments, whitespace inside paths, sources, or nested
    `.inp` files.

  Options always stay on the command line, and `-l name` is still allowed
  next to an `.inp`. An `.inp` cannot be combined with other positional
  inputs.
- **The generic `@file` is removed** from the design. `.inp` is the only
  indirection mechanism, and the native `c.exe -f` maps onto it.
- **`-E` (preprocess only) and `-U` are not supported.** The native `cc`
  has neither. The standalone `cpp.exe` documents conditional-expression
  limitations, so it is not a faithful `-E`. Both are rejected with an
  explicit message.

---

## 3. Defaults

| Setting | Compile/assemble default | Link default | Native effect |
| --- | --- | --- | --- |
| Target OS | **none**: no OS macro | from the output suffix (D2): `.cmd` → `cpm86`; `.exe`/`.com` → `dos2`, or `dos1` when `-target=dos1`; no `-o` → `-target`, else `cpm86` (`a.cmd`) | Selects startup, runtime, suffix |
| CPU (`-march`) | `i8086` | n/a | `+0`; no CPU macro |
| Model (`-mcmodel`) | `small` | `small`; non-small requires `dos2` | no `+l*` flag; `sbegin.o`/`c.lib` family |
| Float (`-mfloat`) | `soft` | `soft` | no `+e`; `-lm` → `m.lib` family |
| Language | ANSI: cc native mode (D6) | n/a | no flag; `-ansi` opt-in strict; never `-3` |
| Optimisation | native default (neither `+c` nor `+f`) | n/a | none |
| Debug | off | off | cc `-n`; ln without `-g` |
| Warnings | on | on | cc without `-s` |
| Error pause | always disabled | n/a | cc `-b` |
| Tool root | `-toolroot`, then `ACCROOT`, then executable-relative (Phase 1) | same | n/a |

Compatibility checks move to the point where they matter:

- **Compile time:** `-mcmodel` non-small together with an *explicit*
  `-target=cpm86`/`dos1` is an error.
- **Link time:**
  - an explicit `-target` must agree with the `-o` suffix: `cpm86` needs
    `.cmd`; `dos1`/`dos2` need `.exe` or `.com`;
  - non-small models require `dos2`;
  - `.com` requires the small model.

There is **no tiny model**: Aztec C86 defines only small, `+LC`, `+LD`
and `+L`. The single-segment "tiny" layout is a `.com` program (manual,
linker "Segment specification for .com programs" and Tech Info 1.2.4 to
1.2.6). It uses small-model objects with the same startup and runtime as a
small `.exe`, and is selected only by the `.com` output suffix. It cannot
host overlays, and linker `-X` has no effect on it.

---

## 4. Public option map (driver → cc / as / ln)

P4 status: every "probe" entry of section 5 passed its execution probe
(`exec-check.sh`; the DOS build re-probes `-g` and `-Wl,-S` in
`exec-dos-check.sh`) and is enabled; allow-list in `raw.c`.

Status: **keep** = already implemented and unchanged; **change** = behaviour
changes in this plan; **new** = added; **remove** = dropped.

| Public option | cc | as (explicit `.asm`) | ln | Status / phase |
| --- | --- | --- | --- | --- |
| `-help`, `-version` | – | – | – | change (help text, P1) |
| `-c` | normal compile | normal assemble | not run | keep (one or more sources) |
| `-S` | `-A` | not allowed | not run | keep (one or more sources) |
| `-o file` | `-o` private name, then publish | same | `-o` private name, then publish | keep |
| `-I dir` | `-I<guest dir>` (ordered) | same | – | keep |
| `-D name[=v]` | `-Dname=v` | `-Dname=const` (constant only) | – | change: allow for `.asm` with constant validation (P4) |
| `-L dir` | – | – | driver library search; `CLIB` | keep (never forwarded) |
| `-l name` | – | – | resolved `name.lib` path; `m` is model/float aware | keep |
| `file.inp` | – | – | expanded into ordered link inputs | new (P2) |
| `-target=cpm86\|dos1\|dos2` | `-D__CPM86__=1` / `-D__MSDOS__=1` **only if explicit** | same | startup/runtime; must agree with the `-o` suffix | change (P3) |
| `-march=i8086\|i80186\|i80286` | `+0`/`+1`/`+2`; plus `-D__i186__=1` and `-D__i286__=1` as appropriate | `-186` for i80186; i80286 rejected until probed | – | change (macros, P6) |
| `-mcmodel=small\|medium\|compact\|large` | –/`+lc`/`+ld`/`+l` | user owns ABI | startup/runtime/math family | change (check timing, P3) |
| `-mfloat=soft\|auto\|8087` | –/–/`+e` | not rewritten | `-lm` → `m`/`m87s`/`m87` family | keep |
| `-mfloat=8087-fast` | `+ej` | – | hardware family | keep rejected until runtime-qualified |
| `-ansi` | `-ansi` (strict: disables `near`/`far`/`huge`, breaks `dos.h`) | – | – | keep, opt-in (D6) |
| `-traditional` | – | – | – | **remove**: K&R `-3` not supported (D6) |
| `-Osize` / `-Ospeed` | `+c` / `+f` | – | – | keep |
| `-g` | omit `-n` | – | `-g` before inputs; publish `.dbg` beside output | new (P4, after probe) |
| `-w` | `-s` | – | – | keep |
| `-nostartfiles` | – | – | omit startup | keep |
| `-nostdlib` | – | – | omit startup and runtime | keep |
| `-Wc,arg` | allow-listed arg (section 5) | – | – | new (P4) |
| `-Wa,arg` | – | allow-listed arg (section 5) | – | new (P4) |
| `-Wl,arg` | – | – | allow-listed arg (section 5) | new (P4) |
| `-overlay-root=CODE,DATA` | – | – | `+c CODE +d DATA -r`, adds `ovld.o`, emits `.rsm` | change: cpm86 **and** dos (P5) |
| `-overlay=ROOT.rsm` | – | – | adds `ROOT.rsm` and `ovbgn.o`, emits `.ovr` | change: cpm86 **and** dos (P5) |
| `-save-temps`, `-v`, `-###` | – | – | – | keep; `-###` format stabilised (P1) |
| `-toolroot=dir` | – | – | – | keep (+ executable-relative discovery, P1) |
| `-emu=path` | – | – | – | keep (Unix only) |
| `@file` | – | – | – | **remove** (replaced by `.inp`, P2) |

---

## 5. Native option disposition (complete)

`-Wc,`/`-Wa,`/`-Wl,` take exactly one joined backend token (no separate
operand). Every argument is checked against the tables below. Two tiers:

- **Verified:** enabled as soon as the phase lands.
- **Probe:** enabled only after an emu2 probe proves the spelling and
  effect, and adds a golden plus an execution test.

### 5.1 Compiler `cc.exe` (optimizing, v4.10d)

| Native | Meaning | Disposition |
| --- | --- | --- |
| `-A` | stop before assembler | managed by `-S` |
| `-B` | no pause after errors | always emitted (`-b`) |
| `-D` | define macro | managed by `-D` |
| `-I` | include area | managed by `-I` |
| `-O file` | output file | managed by `-o` |
| `-S` | suppress warnings | managed by `-w` |
| `-T` | C source as comments in assembly | `-Wc,-T`, only with `-S` (probe) |
| `-E<n>` | expression table size | `-Wc,-E<n>` (**verified**) |
| `-L<n>` | local symbol table size | `-Wc,-L<n>` (**verified**: overflow message `Local table full! (use -L)` is fixed by `-L300`) |
| `-y<n>` | outstanding switch cases | `-Wc,-y<n>` (**verified** spelling; lowercase) |
| `-z<n>` | literal string table | `-Wc,-z<n>` (**verified** spelling; lowercase) |
| `-n` | no debug information | managed by `-g` |
| `-ansi` / `-3` | language mode | `-ansi` opt-in strict (D6); `-3` never emitted |
| `-c` | prototype-conversion warning toggle | rejected (toggle semantics) |
| `-k` | disable extended keywords | `-Wc,-k` (probe) |
| `-ze` | Microsoft near/far/huge | rejected (foreign ABI) |
| `+F` / `+C` | fast / compact code | managed by `-Ospeed` / `-Osize` |
| `+0` / `+1` / `+2` | CPU | managed by `-march` |
| `+LC` / `+LD` / `+L` | memory model | managed by `-mcmodel` |
| `+e` / `+ej` | inline 8087 | managed by `-mfloat` |
| `+N` | save registers around foreign calls | rejected (foreign ABI) |
| `+D` / `+DF` | Lattice long conventions | rejected (foreign ABI) |
| `+U` | uninitialised globals become externs | `-Wc,+U` (probe) |
| `+R` | no inter-statement register tracking | `-Wc,+R` (probe) |
| `+A` | no-alias assertion | `-Wc,+A` (probe) |
| `+M` | always use MUL for constants | `-Wc,+M` (probe) |
| `+s` | strings in code (large data) | `-Wc,+s` only with compact/large (probe) |
| `+b` | stack-overflow checks | rejected until runtime support is qualified |

Do not reuse table-option meanings from the non-optimising `ccb` (for
example its `+F`/`+J`); they are outside this backend.

### 5.2 Assembler `as.exe` (explicit `.asm` inputs only)

`-Wa,` never reaches the assembler that `cc` starts internally for `.c`
inputs.

| Native | Meaning | Disposition |
| --- | --- | --- |
| `-O file` | output object | managed by `-o` |
| `-ZAP` | delete source after assembly | forbidden |
| `-186` | 80186 instructions | managed by `-march=i80186` |
| `-2` | 80287 without WAIT | rejected until the CPU/8087 policy is probed |
| `-S<n>` | squeeze passes | `-Wa,-S<n>` (probe) |
| `-I` | include area | managed by `-I` |
| `-D` | constant symbol | managed by `-D` (constant values only) |
| `-L`, `-LA`, `-LS` | listing file | rejected (unowned side artifact) |
| `-C`, `-CA`, `-CS` | listing to console | `-Wa,-C*` (probe) |
| `-X` | omit false conditionals in listing | rejected (only meaningful with listing files) |

### 5.3 Linker `ln.exe`

| Native | Meaning | Disposition |
| --- | --- | --- |
| `-O file` | output; suffix selects format | managed by `-o` and target |
| `-L name` | library via `CLIB` | managed by `-l` (resolved paths) |
| `-F file` | argument file | **always used** by the driver (section 7) |
| `-T` | `.sym` symbol table | rejected (unowned side artifact) |
| `-M` | no multiple-definition warnings | `-Wl,-M` (probe) |
| `-N` | link despite undefined symbols | forbidden (breaks the success contract) |
| `-S<hex>` | DOS minimum stack+heap | `-Wl,-S<hex>`, dos targets only (probe) |
| `-X<hex>` | DOS 2 maximum paragraphs | `-Wl,-X<hex>`, `dos2` `.exe` only (probe) |
| `-V` | verbose | `-Wl,-V` (probe) |
| `-B`/`-C`/`-D`/`-U addr` | base and segment layout, ROM | rejected (custom layout out of scope) |
| `-R`, `+C`, `+D` | overlay root table and reservations | managed by `-overlay-root` |
| `-g` / `-q` | collect / stop collecting `.dbg` | managed by `-g` (whole program) |
| `CLIB` env | library search list | managed from `-L` |

### 5.4 Native `c.exe` driver (migration only)

| Native | acc |
| --- | --- |
| `c -c file` | `acc -c file` |
| `c -f list` | `acc list.inp` (link inputs only) |
| `c +l foo.c` | `acc -target=dos2 -mcmodel=large foo.c` |
| `c -s` (warnings) | `acc -w` |

---

## 6. Preprocessor

Facts **verified** with the deliberate-error probe (`#ifdef X` guarding a
negative-size array):

| Source | Macros |
| --- | --- |
| Native compiler, always | `__STDC__`, `__FILE__`, `__LINE__`, `__DATE__`, `__TIME__`, `__FUNC__`, `MPU8086` (with any of `+0`/`+1`/`+2`) |
| Not defined natively | `M_I86`, `I86`, `MSDOS`, `CPM86`, `AZTEC_C`, any CPU-tier or optimisation signal |
| `char` signedness | **signed** (`(char)-1 < 0` holds), so `CHAR_UNSIGNED` is never defined |

**Driver-defined macros (the only two kinds):**

| Fact | Macro | Emitted when |
| --- | --- | --- |
| OS | `__CPM86__=1` or `__MSDOS__=1` (D3) | `-target=` given explicitly |
| CPU tier | `__i186__=1` | `-march=i80186` or `i80286` |
| CPU tier | `__i286__=1` | `-march=i80286` |

**`share/aztec42/include/azdefs.h`** (opt-in `#include <azdefs.h>`, no
driver code):

- `__AZTEC_C__`, `__AZTEC_C_MINOR__`, `__AZTEC_C_PATCHLEVEL__`,
  `__AZTEC_C_VERSION__` (`major*10000 + minor*100 + patch`), `__VERSION__`
  (Aztec C 4.3.0, decision D1)
- `__i86__` (guarded by native `MPU8086`)
- `__CHAR_BIT__`, `__STDC_NO_ATOMICS__`, `__STDC_NO_THREADS__`
- `__ORDER_LITTLE_ENDIAN__`, `__ORDER_BIG_ENDIAN__`,
  `__ORDER_PDP_ENDIAN__`, `__BYTE_ORDER__`

**Cut from issue #12:** alias spellings (`_M_I86`, `__I86__`, `__I186__`,
`__I286__`, `AZTEC_C`, `M_I86`, `I86`); `__OPTIMIZE__` and
`__OPTIMIZE_SIZE__`; `__STDC_HOSTED__`; `__TIMESTAMP__`; the
`SIZEOF_*`/`*_MAX`/`*_WIDTH`/`HAS_*`/`STDC_IEC_559*` catalog. The catalog
consists of model-dependent constants; if it is ever wanted, it is a
header-only follow-up.

Include handling stays as it is: ordered `-I` arguments, then
`<toolroot>/include`, with an empty child `INCLUDE` so no area is added
twice.

---

## 7. Command-line length

| Tool | Limit | Strategy |
| --- | --- | --- |
| `cc.exe`, `as.exe` | DOS command tail, 126 bytes | Each source gets its own command, so the length depends only on the shared options and that one source path. On overflow, fail before running anything: `compiler command line too long (N > 126 bytes): reduce -I/-D or shorten paths`. |
| `ln.exe` | DOS command tail | **Always** a driver-written CRLF response file passed with `-F E:\link.rsp`, independent of length. |
| driver input | host shell | `.inp` for long link input lists |

Guest paths stay short through the fixed `C:`/`D:`/`E:` mapping. Paths
longer than 63 characters, and arguments containing whitespace or control
characters, remain rejected.

---

## 8. Code quality and structure

Target layout (C89, documented, no behaviour hidden in `#ifdef` mazes):

Status: **done** (P1). The C4 component view in design.md is the
reference.

| Module | Responsibility | Filesystem |
| --- | --- | --- |
| `acc.c` | `main`, `-help`, `-version` | no |
| `words.c` | word lists, diagnostics | no |
| `args.c` | argv → `Request` (syntax only) | no |
| `profile.c` | flags, startup, runtime and math tables per profile | no |
| `raw.c` | `-Wc`/`-Wa`/`-Wl` allow-list | no |
| `path.c` | canonical paths, guest mapping, DOS validation | probes only |
| `plan.c` | `Request` → `Plan` (validation, mapping, serialisation) | probes only |
| `print.c` | `-###`/`-v` rendering, the test oracle | no |
| `execunix.c`, `execdos.c` | run a `Plan`: workspace, children, publication | yes |
| `acc.h`, `request.h`, `plan.h`, `profile.h`, `raw.h`, `path.h` | documented types and contracts | – |

Quality rules:

- Replace the magic integers (`target == 0`, `classify()` returning 0–4)
  with enums: `Stage`, `Target`, `Model`, `Cpu`, `Float`, `Language`,
  `Optimize`, `InputKind`.
- Every function (static included) has a comment stating its contract,
  ownership and failure behaviour; every file has a header; non-obvious
  logic states its reason and source (manual section or probe).
- Null tests compare with `0`: Aztec's `NULL` is `((void *)0)` and warns
  against const pointers. Aztec warnings fail the DOS build.
- Temporary files are unique per invocation and always removed: every
  intermediate lives in the private workspace (guest `E:`), which is
  removed recursively on success, failure and interruption unless
  `-save-temps` is given.
- Use one error path (`fail`) with consistent messages. Every message is
  covered by a test (section 9).
- **Portability:** the code must compile warning-free with
  `cc -std=c89 -pedantic -Wall -Wextra -Werror` (gcc and clang) **and**
  with Aztec C 4.2 `cc +0` small model for DOS 2.x. That means:
  - 16-bit `int` and `size_t`, a 64 KB data segment, no `//` comments,
    no `stdint.h`;
  - no large stack buffers: reduce the 1024-byte `ACC_PATH` locals on
    DOS, and heap-allocate or share large buffers.
- **Self-hosting:**
  - The Makefile builds `acc.exe` **with `acc`**
    (`acc -target=dos2 ... -o acc.exe`) instead of the
    `aztec42_cc`/`aztec42_link` wrappers.
  - `build.bat` remains the native DOS bootstrap.
  - A check proves the fixed point: the `acc.exe` built this way rebuilds
    itself under emu2 with an identical plan.

Documentation style (design.md, MANUAL.md): seasoned engineers as the
audience, minimal prose, tables first; design.md keeps C4 views (context,
containers, components, data) as Mermaid diagrams in sync with the code.

---

## 9. Testability

- **Planning is the unit under test.** `-###` validates everything and
  prints the full plan without executing anything. Its output format is
  stabilised and documented in `MANUAL.md`:
  - a header with **all** resolved settings, including defaults (today
    it only shows target, model and CPU);
  - then the `tool`, `response` and `publish` lines;
  - output is deterministic, with guest paths only.
- **Golden suite:** `tests/plan/<case>.args` + `<case>.expect` (expected
  `-###` output, or the expected error message and exit status), run over
  a fixture tree by a single runner.
  - The runner is used by `check-unix` (native `acc`) **and** `check-dos`
    (`acc.exe` under emu2). Identical expectations prove that both builds
    plan identically.
- **Coverage gate** (`make check-coverage`, part of `make check`; done):
  a script fails the check unless:
  - every public option has at least one case;
  - every row of sections 4–5 marked managed, verified or rejected has at
    least one case;
  - every `fail("...")` message appears in at least one `.expect`.
- **Execution suites** (`exec-check.sh`, `exec-dos-check.sh`) stay
  end-to-end. They build and run programs under emu2 and cover:
  - CP/M-86 and DOS overlays;
  - table-size overflow and its fix;
  - the `.inp` link;
  - failure preservation and concurrency.

---

## 10. Decisions

| Id | Question | Status |
| --- | --- | --- |
| D1 | Aztec version reported by `azdefs.h` | **closed**: 4.3.0; `__AZTEC_C_VERSION__` is `40300L`, a `long`, because the issue's `int` formula overflows the 16-bit `int` (caught by the P6 probe); separate from `acc`'s own version |
| D2 | Link target when `-target` is absent | **closed**: follows the `-o` suffix (`.cmd` → `cpm86`; `.exe`/`.com` → `dos2`, `dos1` only when explicit); an explicit `-target` must agree with the suffix; no `-o` and no `-target` → `cpm86`, `a.cmd` (current behaviour, kept by default) |
| D3 | OS macro spelling | **closed**: `__CPM86__` and `__MSDOS__`. `__DOS__` was only defined by acc itself (no source or Aztec header tests it) |
| D4 | K&R `aztec34` toolchain | **closed**: out of scope; acc supports only the ANSI compiler |
| D5 | `CHAR_UNSIGNED` | **closed**: `char` is signed (verified), never defined |
| D6 | Language mode | **closed**: ANSI only = cc native mode (ANSI plus the `near`/`far` extensions Aztec's own `dos.h` needs; forcing `-ansi` breaks it, verified). `-ansi` stays as opt-in strict mode; `-traditional` (`-3`, K&R) is removed |

---

## 11. Phases

Each phase lands with its golden cases, `MANUAL.md` section and
`design.md` reconciliation. Every phase must pass the Unix build, the DOS
build (Aztec C 4.2) and all checks.

| Phase | Content | Depends on |
| --- | --- | --- |
| **P0 Decisions** | D1–D6 settled (section 10). | done |
| **P1 Foundation** (**done**) | Module split and enums (section 8); stable `-###` format with full settings header; golden harness capturing **current** behaviour on Unix and DOS; self-hosted `acc.exe` build plus fixed-point check; executable-relative tool root; `-help` cleanup (drop the `design.md` pointer and the wrong overlay wording); `MANUAL.md` skeleton. | – |
| **P2 Invocation model** (**done**) | `-c`/`-S` with several sources (kept, rules documented and tested); mixed link compiles first and then links; `.inp`; unconditional linker response file; remove `@file`; length errors. | P1 |
| **P3 Defaults and target-neutral compile** (**done**) | No OS macro without `-target`; `__DOS__` renamed `__MSDOS__`; link target from the `-o` suffix with explicit-target agreement (D2); native ANSI mode, `-ansi` opt-in strict, `-traditional` removed (D6); model/target checks moved; defaults documented and covered by golden cases. | P1 |
| **P4 Native option completion** (**done**) | `-Wc,`/`-Wa,`/`-Wl,` allow-lists, verified tier first, probe tier as each probe passes; `-g` with `.dbg` publication; `-D` for `.asm`. | P2 |
| **P5 Overlays on DOS** (**done**) | Accept `dos1`/`dos2` with a `.exe` root (a `.com` root is rejected: `.com` programs cannot use overlays); DOS example in `examples/Makefile`; execution tests on Unix and DOS builds. | P2 |
| **P6 Preprocessor** (**done**) | Driver CPU macros; `azdefs.h` (4.3.0); golden and execution tests. | P3 |
| **P7 Reconciliation** (**done**) | `design.md` brought in line (remove the `@file` grammar, multi-source `-c`, outdated claims); `MANUAL.md` complete; coverage gate enforced. | all |

P3 can run in parallel with P2. P4 and P5 can run in parallel once P2 has
landed.
