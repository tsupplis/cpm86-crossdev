# Developer Guide — cpm86-crossdev

This document explains the internal structure of the project so that contributors
can add new tools, fetchers, and wrappers without needing to reverse-engineer the
existing ones.

---

## Project layout

```
cpm86-crossdev/
├── fetch_tools          # top-level entry point — runs every fetcher in order
├── clear_tools          # removes all generated artefacts (keeps archive/)
├── src/
│   ├── fetch/           # one shell fragment per tool — run by fetch_tools
│   │   ├── _archive     # shared download-cache helper (fetch_get function)
│   │   ├── cross_aztec34      # download-only fetcher example
│   │   ├── native_emu2        # git-clone + build fetcher example
│   │   └── …
│   ├── aztec42-patch/   # patches applied during the aztec42 build step
│   ├── drtools/         # DR Personal Basic binary (stored in-tree)
│   ├── mstools/         # Microsoft tools stored in-tree
│   ├── patch/           # binary patch scripts for cpm86.exe, aztec34 cpp
│   ├── tools/           # native helper tools (doscat, hexcom) built by make
│   ├── turbo/           # Turbo Pascal binary/data files stored in-tree
├── archive/             # download cache + build trees for native tools (never removed by clear_tools)
│   ├── emu2/            # emu2-cpm86 source (cloned once, .git stripped)
│   ├── tnylpo/          # tnylpo source (cloned once, .git stripped)
│   ├── cmdtools/        # cpm86-cmdtools source (cloned once, .git stripped)
│   ├── nasm/            # nasm source (extracted from tarball)
│   ├── upx/             # upx source (extracted from tarball)
│   └── *.zip / *.tar.gz / …  # downloaded archives
├── share/               # staged tool trees consumed by wrappers (gitignored)
│   ├── aztec34/         # Aztec C 3.4 — bin/ lib/ include/
│   ├── aztec42/         # Aztec C 4.2 — bin/ lib/ include/
│   ├── cpm/             # CP/M-80 and CP/M-86 .com/.cmd binaries
│   ├── pcdev/           # DOS tools (.exe/.com)
│   ├── emu/             # cpm86.exe emulator image
│   ├── cb86cpm/         # CB-86 2.0 for CP/M-86
│   ├── cb86dos/         # CB-86 2.1 for DOS
│   ├── drc86cpm/        # DR C 1.11 for CP/M-86
│   ├── drpmtcpm/        # Pascal MT+ 3.3 for CP/M-86
│   ├── f77cpm/          # DR Fortran-77 4.0 for CP/M-86
│   ├── intel/           # Intel PL/M-86 tools
│   ├── m2cbcpm/         # M2CBASIC 1.4
│   ├── plicpm/          # DR PL/I-86 1.0
│   ├── turbocpm/        # Turbo Pascal 3.02 for CP/M-86
│   └── xlisp/           # XLISP 1.1 (compiled CP/M-86 binary + sources)
├── bin/                 # wrapper scripts and compiled native binaries
│   ├── emu2             # compiled native binary (gitignored)
│   ├── tnylpo           # compiled native binary (gitignored)
│   ├── nasm             # compiled native binary (gitignored)
│   ├── upx              # compiled native binary (gitignored)
│   ├── cmdinfo          # compiled native binary (gitignored)
│   ├── bin2cmd          # compiled native binary (gitignored)
│   ├── exe2cmd          # compiled native binary (gitignored)
│   ├── doscat           # compiled native binary (gitignored)
│   ├── hexcom           # compiled native binary (gitignored)
│   └── aztec42_cc       # shell wrapper (tracked in git)
│   └── …               # all other shell wrappers (tracked in git)
├── local/               # user scratch area (gitignored)
└── examples/            # sample source files
```

`fetch_tools` runs every fetcher in `src/fetch/` in a fixed order.  Each
fetcher receives `$root` (the absolute project root) and the `fetch_get`
function from `src/fetch/_archive`.  Each fetcher runs in its own isolated
subshell (via `env -i sh -c`), with all output redirected to a timestamped
`fetch-YYYYMMDD-HHMMSS.log` at the project root.  Only `INF:`/`ERR:` summary
lines appear on the terminal.  After all fetchers complete, `share/` is
write-protected and the tool chain is ready.

The `archive/` directory is the **persistent download cache**.  `clear_tools`
deliberately leaves it intact so that the tree can be rebuilt offline with
`ARCHIVE_FIRST=1 ./fetch_tools`.

---

## Fetcher naming convention

Fetchers in `src/fetch/` follow a two-part `<tier>_<toolname>` naming scheme
that immediately communicates whether the fetcher downloads a cross-development
tool (run under an emulator) or builds a native host binary:

| Prefix | Meaning | Examples |
|---|---|---|
| `cross_` | Downloads/stages a cross-development tool into `share/` — runs under emu2 or tnylpo at build time | `cross_aztec34`, `cross_drtools`, `cross_turbo` |
| `native_` | Clones or downloads source and compiles a native host binary into `bin/` | `native_emu2`, `native_nasm`, `native_tnylpo` |

Some fetchers do both (e.g. `cross_aztec42_build` downloads the archive *and*
builds native support tools from source); name them after their primary purpose
and add a `_build` suffix when the build step is distinct from the download.

---

## Writing a fetcher

A fetcher is a POSIX shell fragment stored in `src/fetch/`.  `fetch_tools`
runs each one in its own isolated subshell, so:

- `$root` and `fetch_get` are injected via `env -i sh -c`; no other environment
  variables are inherited — pass anything extra through `env` if needed.
- `set -e` is active inside the subshell.
- Do **not** call `exit 0` on success — a zero exit from the last command is
  sufficient.  A non-zero exit is reported as a failure by `fetch_tools`.

### Template — download-only fetcher (`cross_`)

```sh
#!/bin/sh   # <- makes editors happy; not executed directly

set -e

echo INF: Checking <Tool Name> <version> ...
if [ ! -f "$root/share/<share-subdir>/<sentinel-file>" ]; then
    fetch_get "<URL>"            # downloads to archive/<basename> if needed

    tmp="$root/share/temp-<toolname>"
    rm -rf "$tmp"
    mkdir -p "$tmp"
    trap 'rm -rf "$tmp"' EXIT
    (cd "$tmp" && unzip -L "$ARCHIVED_FILE")   # or: tar zxf "$ARCHIVED_FILE" -C "$tmp"

    mkdir -p "$root/share/<share-subdir>"
    cp -R "$tmp/<extracted-dir>/<subdir>" "$root/share/<share-subdir>/"
    # … more cp lines …
    chmod 644 "$root/share/<share-subdir>"/*

    rm -rf "$tmp"
    trap - EXIT
fi
```

Key points:
- Use a **private staging directory** (`temp-<toolname>`) so that a `cp` failure
  mid-way leaves no partial state under `share/`.  The `trap` ensures cleanup on
  any error.
- Use **absolute paths** (`$root/share/...`) throughout — never `cd` to a
  directory and use relative paths.

### Template — git-clone + build fetcher (`native_`)

```sh
#!/bin/sh

set -e

echo INF: Checking <Tool Name> ...
mkdir -p "$root/archive"
if [ ! -d "$root/archive/<tool>" ]; then
    git clone "<repo-url>" "$root/archive/<tool>"
    rm -rf "$root/archive/<tool>/.git"
fi
if [ ! -f "$root/bin/<binary>" ]; then
    ncpus="$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null \
             || getconf NPROCESSORS_ONLN 2>/dev/null \
             || getconf _NPROCESSORS_ONLN 2>/dev/null || printf '%s\n' 1)"
    (cd "$root/archive/<tool>" && "${MAKE:-make}" -j"$ncpus" && cp <binary> "$root/bin/")
fi
```

### Template — tarball + build fetcher (`native_`)

```sh
#!/bin/sh

set -e

echo INF: Checking <Tool Name> ...
mkdir -p "$root/archive"
if [ ! -d "$root/archive/<tool>" ]; then
    fetch_get "<url>"
    tar axf "$ARCHIVED_FILE" -C "$root/archive"
    mv "$root/archive/<tool>-<version>" "$root/archive/<tool>"
fi
if [ ! -f "$root/bin/<binary>" ]; then
    ncpus="$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null \
             || getconf NPROCESSORS_ONLN 2>/dev/null \
             || getconf _NPROCESSORS_ONLN 2>/dev/null || printf '%s\n' 1)"
    (cd "$root/archive/<tool>" && ./configure && "${MAKE:-make}" -j"$ncpus" && cp <binary> "$root/bin/")
fi
```

### Rules and conventions

| Rule | Detail |
|---|---|
| Follow the naming convention | Name the file `cross_<toolname>` for emulated tools, `native_<toolname>` for host-compiled binaries. |
| Always guard with a sentinel | Wrap every action in `if [ ! -f ... ]` or `if [ ! -d ... ]` so re-running `fetch_tools` is idempotent. |
| Use absolute paths | Always use `$root/share/...`, `$root/bin/...`, etc.  Never `cd` to a directory and then use relative paths — the fetcher runs in an isolated subshell and relative paths are fragile. |
| Use a staging temp dir for unzip/tar | Extract into `$root/share/temp-<toolname>`, copy what you need, then `rm -rf` it.  Guard with `trap 'rm -rf "$tmp"' EXIT` so partial state is cleaned on error. |
| Use `fetch_get` for every download | Never call `wget` or `curl` directly.  `fetch_get` handles the `archive/` cache and the `ARCHIVE_FIRST` offline mode automatically. |
| Stage into `share/<subdir>/` | Never install files directly into `bin/`.  Wrappers resolve paths to `share/` at runtime. |
| `chmod 644` after staging | `fetch_tools` does a final `chmod a-w` pass over all of `share/` so the sentinel check (`[ ! -f ... ]`) stays reliable. |
| Source/build trees go into `archive/` | Clone with `"$root/archive/<tool>"` as the target, or extract tarballs with `-C "$root/archive"`.  Build in-place under `archive/`.  Never create build directories at the project root.  Strip `.git` after cloning (`rm -rf "$root/archive/<tool>/.git"`). |
| Use `${MAKE:-make}` | Never call bare `make` — use `"${MAKE:-make}"` so the caller can override the make binary. |
| No `exit 0` | Exit status of the last command is used as the fetcher result.  A bare `exit` with a non-zero code is fine to signal failure. |
| Register in `fetch_tools` | Add `run_fetcher <name>` at the appropriate position in `fetch_tools` (after any dependency fetchers). |
| Register in `clear_tools` if needed | If the fetcher stages a `share/` subtree, add `"$root/share/<subdir>"` to the `rm -rf` list in `clear_tools`. |

### `fetch_get` reference

```sh
fetch_get "<url>"             # saves to archive/<basename of url>
fetch_get "<url>" "<name>"    # saves to archive/<name>
```

After the call `$ARCHIVED_FILE` holds the full path to the cached file.

---

## Writing a wrapper

A wrapper is a `chmod +x` shell script placed directly in `bin/`.  It is tracked
in git.  Its sole job is to locate the right emulator and the right `share/`
subtree, then exec the real tool.

There are five wrapper patterns in use.  Pick the one that matches your tool.

---

### Pattern 1 — Native binary pass-through

The binary lives in `bin/` itself (built by a fetcher).  The wrapper is not
needed at all; the binary **is** the command.  Examples: `bin/emu2`,
`bin/tnylpo`, `bin/nasm`, `bin/upx`, `bin/cmdinfo`, `bin/doscat`.

No wrapper file is required for these.

---

### Pattern 2 — Simple DOS `.exe` / `.com` under `emu2`

The tool is a single DOS executable with no special environment needs.

```sh
#!/bin/sh
prog=`basename $0 | sed -e 's/<prefix>_//g'`.exe   # or .com
emu2=`dirname $0`/emu2
EMU2_DRIVE_D="`dirname $0`/../share/<share-subdir>"
export EMU2_DRIVE_D
EMU2_PROGNAME="D:\\$prog"
export EMU2_PROGNAME
"$emu2" "$EMU2_DRIVE_D/$prog" "$@" -- PATH=D:\\ LIB=D:\\
rc=$?
if [ -z "$*" ]; then
    echo ""
fi
exit $rc
```

Examples: [`bin/pcdev_rasm86`](bin/pcdev_rasm86), [`bin/intel_plm86`](bin/intel_plm86),
[`bin/cpm86_asm86`](bin/cpm86_asm86).

The trailing `if [ -z "$*" ]` block prints a blank line when the tool is
invoked with no arguments, which prevents the shell prompt from overwriting the
tool's last output line.

---

### Pattern 3 — DOS `.exe` under `emu2` with compiler-specific env

Used for Aztec C tools, which need `CLIB`, `INCLUDE`, and a writable temp
directory (`CCTEMP`).

```sh
#!/bin/sh
prog=`basename $0 | sed -e 's/aztec42_//g'`.exe
emu2=`dirname $0`/emu2
EMU2_DRIVE_D="`dirname $0`/../share/aztec42"
export EMU2_DRIVE_D
if [ -z "$TMPDIR" ]; then
    TMPDIR="/tmp/emu2-`id -u`"
fi
mkdir -p "$TMPDIR/AZTEC/TEMP"
EMU2_DRIVE_E="$TMPDIR/AZTEC"
export EMU2_DRIVE_E
EMU2_PROGNAME="D:\\bin\\$prog"
export EMU2_PROGNAME
"$emu2" "$EMU2_DRIVE_D/bin/$prog" "$@" -- PATH=D:\\BIN CLIB=D:\\LIB\\ INCLUDE=D:\\INCLUDE \
  CCTEMP="E:\\TEMP\\"
```

Examples: [`bin/aztec42_cc`](bin/aztec42_cc), [`bin/aztec34_cc`](bin/aztec34_cc).

---

### Pattern 4a — CP/M-86 `.cmd` run directly under `emu2`

Used for tools that are CP/M-86 binaries invoked directly by `emu2` (no
`cpm86.exe` intermediary).  Use `EMU2_CPM_APPEND` (not `EMU2_APPEND`) so
that emu2 appends the drive to the CP/M PATH rather than the DOS PATH.

```sh
#!/bin/sh
prog=<name>.cmd
share="`dirname $0`/../share/<share-subdir>"
emu2=`dirname $0`/emu2
EMU2_CPM_APPEND="D:"
export EMU2_CPM_APPEND
EMU2_DRIVE_D="$share"
export EMU2_DRIVE_D
EMU2_DRIVE_P="$share"
export EMU2_DRIVE_P
EMU2_PROGNAME="D:\\$prog"
export EMU2_PROGNAME
"$emu2" "$share/$prog" "$@"
if [ -z "$*" ]; then
    echo ""
fi
```

Examples: [`bin/drpmt_pc`](bin/drpmt_pc), [`bin/drpmt_asm`](bin/drpmt_asm),
[`bin/drpmt_link`](bin/drpmt_link), [`bin/drpli_pc`](bin/drpli_pc),
[`bin/m2cb_cvt`](bin/m2cb_cvt).

---

### Pattern 4b — CP/M-86 `.cmd` run inside `cpm86.exe` under `emu2`

Used for tools that are CP/M-86 binaries that must run inside the `cpm86.exe`
emulator image (itself run under `emu2`).

```sh
#!/bin/sh
prog=<name>
share="`dirname $0`/../share/<share-subdir>"
emu2=`dirname $0`/emu2
emudir="`dirname $0`/../share/emu"
EMU2_APPEND="D:"
export EMU2_APPEND
EMU2_DRIVE_D="$share"
export EMU2_DRIVE_D
EMU2_DRIVE_P="$share"
export EMU2_DRIVE_P
EMU2_DRIVE_E="$emudir"
export EMU2_DRIVE_E
EMU2_PROGNAME="E:\\cpm86.exe"
export EMU2_PROGNAME
"$emu2" "$emudir/cpm86.exe" "D:$prog" "$@"
if [ -z "$*" ]; then
    echo ""
fi
```

No wrappers currently use this pattern — it is documented for reference should a tool require it.

---

### Pattern 5 — CP/M-80 `.com` under `tnylpo`

Used for the handful of CP/M-80 tools (`asm86.com`, `gencmd.com`).

```sh
#!/bin/sh
prog=`basename $0 | sed -e 's/cpm_//g'`.com
tnylpo=`dirname $0`/tnylpo
exec "$tnylpo" "`dirname $0`/../share/cpm/$prog" "$@"
```

Example: [`bin/cpm_asm86`](bin/cpm_asm86).

For tools that also need a configuration file (e.g. drive mappings), see
[`bin/cpm_xlt86`](bin/cpm_xlt86) which writes a temporary `tnylpo` config and
cleans it up via `trap`.

---

### Pattern 6 — Multi-pass compiler wrapper

Used when a language tool requires two or more sequential invocations that the
caller should not have to manage, with intermediate files cleaned up via `trap`.

```sh
#!/bin/sh
share="`dirname $0`/../share/<share-subdir>"
emu2=`dirname $0`/emu2
EMU2_APPEND="D:\\"
export EMU2_APPEND
EMU2_DRIVE_D="$share"
export EMU2_DRIVE_D

trap 'rm -f <intermediate-files>' EXIT INT TERM

# Pass 1
EMU2_PROGNAME="D:\\<pass1>.cmd"
export EMU2_PROGNAME
"$emu2" "$share/<pass1>.cmd" "$@" -- PATH=D:\\
rc=$?

# Pass 2 (conditional on pass 1 producing its output)
if [ $rc -eq 0 ] && [ -f <intermediate> ]; then
    EMU2_PROGNAME="D:\\<pass2>.cmd"
    export EMU2_PROGNAME
    "$emu2" "$share/<pass2>.cmd" ... -- PATH=D:\\
    rc=$?
fi

exit $rc
```

Examples: [`bin/drccpm_cc`](bin/drccpm_cc) (DR C two-pass compiler),
[`bin/drfcpm_f77`](bin/drfcpm_f77) (DR Fortran-77 two-pass compiler).

---

## EMU2 environment variables

| Variable | Purpose |
|---|---|
| `EMU2_DRIVE_A` … `EMU2_DRIVE_Z` | Map a DOS drive letter to a host directory. |
| `EMU2_APPEND` | Colon-separated list of DOS drives appended to the DOS PATH inside emu2. |
| `EMU2_CPM_APPEND` | Colon-separated list of drives appended to the CP/M PATH when running a `.cmd` directly under emu2 (use instead of `EMU2_APPEND` for Pattern 4a wrappers). |
| `EMU2_PROGNAME` | Overrides the displayed program name (cosmetic only). |

Drives are resolved at runtime; the convention used in this project is:

| Drive | Conventional use |
|---|---|
| `D:` | Primary tool share directory for the wrapped program. |
| `E:` | Secondary share (e.g. `emu/` for `cpm86.exe`, or `TMPDIR` scratch). |
| `P:` | Extra overlay / resource directory (e.g. Pascal MT+ overlays). |

---

## Adding a new tool — end-to-end checklist

1. **Create `src/fetch/<tier>_<toolname>`** — download and stage the tool into
   `share/<toolname>/`.  Use `cross_` for emulated tools, `native_` for host
   binaries.  Follow the fetcher template above.

2. **Add to `fetch_tools`** — insert `run_fetcher <tier>_<toolname>` at the
   right point in the sequence (after any dependency fetchers).

3. **Add to `clear_tools`** — insert `"$root/share/<toolname>"` into the
   `rm -rf` list.

4. **Create `bin/<prefix>_<prog>`** — write the wrapper using the appropriate
   pattern.  `chmod +x bin/<prefix>_<prog>`.

5. **Update `README.md`** — add an entry to the Script Mapping table and
   document usage examples.

6. **Update `.gitignore`** if the tool produces build artefacts at the project
   root (only applies to native build tools whose source is checked out at the
   root level).

---

## Emulation tiers

When several tools can perform the same job, the project uses the highest
available tier (lower number = preferred):

| Tier | Kind | Emulator |
|---|---|---|
| 1 | Native host binary | none |
| 2 | DOS `.exe` / `.com` | `emu2` |
| 3 | CP/M-86 `.cmd` | `emu2` (direct) or `emu2` → `cpm86.exe` |
| 4 | CP/M-80 `.com` | `tnylpo` |

---

## Local / offline workflow

```sh
# First run — downloads everything and caches in archive/
./fetch_tools

# Subsequent offline rebuild from the cache
ARCHIVE_FIRST=1 ./fetch_tools

# Wipe generated artefacts but keep the download cache
./clear_tools
```

The `local/` directory at the project root is gitignored and is a safe place
for personal experiments that should not be committed.

---

## CP/M-86 reference documentation

The authoritative public reference for CP/M-86 internals.  Use these when
investigating emulator bugs or adding new BDOS support.

| Topic | URL |
|---|---|
| BDOS function overview | https://www.seasip.info/Cpm/bdos.html |
| BDOS function reference (all calls) | https://www.seasip.info/Cpm/bdosfunc.html |
| FCB layout (offset table) | https://www.seasip.info/Cpm/fcb.html |
| CP/M file formats (OMF-86, SYM, …) | https://www.seasip.info/Cpm/formats.html |
| Last-record byte count (`r2`) | https://www.seasip.info/Cpm/bytelen.html |

