# CP/M-86 & MS-DOS Cross Development Environment

This project regroups a simple method to create a cross development environment for CP/M-86. It happens that all DOS based tools can also be used for building DOS programs.
This project was developed for myself in a nerdy spirit and for a lot of fun. Just sharing without any guarantees.

| Language   | Compilers / Interpreters                                                   |
|------------|----------------------------------------------------------------------------|
| C          | Aztec C 3.4 (K&R), Aztec C 4.2 (almost ANSI), DR C 1.11                   |
| Assembler  | RASM-86, ASM-86, MASM 1.10, NASM 3.02                                     |
| Basic      | DR CBASIC 2.0, DR Personal Basic 1.2, MS Basic 5.21/5.22/5.28, TDL BASIC 3.05  |
| Pascal     | Pascal MT+ 3.3, Turbo Pascal 3.02, PolyPascal-86 3.11                     |
| Fortran-77 | DR Fortran-77 4.0                                                          |
| COBOL      | Micro Focus Level II COBOL V2.1                                            |
| PL/M-86    | Intel PL/M-86 3.30                                                         |
| PL/I       | DR PL/I-86 1.0                                                             |
| Lisp       | XLISP 1.1                                                                  |

## Thanks

- dmsc (https://github.com/dmsc) for the emu2 emulator, without which this project would not be nearly as pleasant to use.
- johnsonjh (https://github.com/johnsonjh) for the emu2-cpm86 fork, the hexcom rewrite, and portability/build fixes contributed to this project.

## Where to find CP/M-86?

The source for CP/M-86 doc, sources and binaries is http://www.cpm.z80.de.

A cleaned-up distribution and kernel is available at https://github.com/tsupplis/cpm86-kernel. This distribution is working well in virtual environments, patched with all known patches, 'y2k' friendly (it contains the version of tod which sources are in this project) and AT friendly.

## Key tools for CP/M-86 development

### Compilers and language tools

| Tool | Version | Description |
|------|---------|-------------|
| Aztec C | 4.2/4.10d (**preferred**) | Almost-ANSI C; `c86.lib` patched & improved, `d11.lib` for DOS 1.1 |
| Aztec C | 3.4/3.40a | K&R legacy C; `c86.lib` patched, otherwise as-is |
| DR C | 1.11 | CP/M-86 C compiler; passes `drc860`–`drc862`, runtime `clearl/clears.l86` |
| DR Fortran-77 | 4.0 | CP/M-86; compiler `f77.cmd`+`codegen.cmd`, runtime `f32s/l.obj`, `8087.sim` |
| Pascal MT+ | 3.3 | CP/M-86; compiler `mt+86.cmd`, linker `linkmt.cmd`, assembler `asmt86.cmd` |
| Turbo Pascal | 3.02 | Last CP/M-86 Borland release; compiler `turbo.cmd`, installer `tinst.cmd` |
| PolyPascal-86 | 3.11 | Integrated editor+compiler; small/BCD/8087 variants (`ppas`, `ppbs`, `pp87`) |
| DR CBASIC | 2.0 (CP/M-86) / 2.1 (DOS) | Compiled BASIC; compiler `cb86.exe`, linker `link86.exe` |
| M2CBASIC | 1.4 | MS BASIC → CBASIC source converter; translator + runtime libs |
| DR Personal Basic | 1.2 | Interactive CP/M-86 BASIC interpreter |
| MS Basic | 5.22 (CP/M-86) / 5.28 (DOS) | Interpreted; patched binaries included |
| MS Basic | 5.21 (CP/M-80) / 4.51 "Old Basic" | Run via `tnylpo` |
| TDL BASIC | 3.05 | Z-80 CP/M-80 BASIC; run via `tnylpo` |
| Intel PL/M-86 | 3.30 | Compiler `plm86.exe`, assembler `asm86.exe`, linker, librarian, locator |
| DR PL/I-86 | 1.0 | CP/M-86; compiler `pli.cmd`, runtime `plilib.l86`, DIO sources |
| XLISP | 1.1 | Public-domain Lisp interpreter; built from source with Aztec C 4.2 |
| Micro Focus COBOL | Level II V2.1 | CP/M-86; compiler `cobol.cmd`, runtime `run.cmd` |

### Assemblers

| Tool | Version | Description |
|------|---------|-------------|
| RASM-86 / LINK-86 / LIB-86 | 1.4a / 2.02 / 1.3 | DR DOS assembler toolchain |
| ASM-86 + GENCMD | 1.1 | DR assembler; CP/M-80 and CP/M-86 versions |
| XLT86 | 1.3 | DR 8080/8085 → 8086 translator; `xlt86.com` + overlays; run via `tnylpo` |
| MASM / LINK / ASM / EXE2BIN / HEX2BIN | 1.10 / 2.0 / 2.44a / 1.1 / — | Microsoft DOS assembler toolchain; `masm` patched for emu2 |
| NASM | 3.02 | Netwide assembler (native) |
| Intel ASM-86 | 2.1 | Part of Intel PL/M-86 toolchain (`asm86.exe`) |

### Emulators and utilities

| Tool | Version | Description |
|------|---------|-------------|
| emu2-cpm86 | 2021.01 | DOS/CP/M-86 emulator (fork of emu2 with CP/M-86 support) |
| tnylpo | — | CP/M-80 emulator; used for `asm86.com`, `gencmd.com`, XLT86, MS Basic 80 |
| upx | 5.2.1 | Executable packer |

## License Considerations

No guarantees are made for any component. Use each at your own risk subject to the conditions below.

| Component | License / Status | Reference |
|-----------|-----------------|-----------|
| MS-DOS tools (DOS 1.1, Basic 86) | MITS license | [LICENSE](https://github.com/microsoft/MS-DOS/blob/master/LICENSE.md) |
| Aztec C | Freeware (conditions apply) | [aztecmuseum.ca](https://www.aztecmuseum.ca/intro.htm#intro) |
| DR tools (RASM-86, ASM-86, CBASIC, DR C, Fortran-77, Pascal MT+, Personal Basic, PL/I-86, XLT86, M2CBASIC) | DR/Caldera open-source license | [license](http://www.cpm.z80.de/license.html) · [faq](http://www.cpm.z80.de/faq.html) |
| Turbo Pascal 3.01A (Borland) | Freely usable, not open source | [RC750 image](https://rc700.dk/software/RC750_TurboPascal_v3.01a/files/RC750_TurboPascal_v3.01a.img.bz2) |
| emu2 / emu2-cpm86 | GPL-2.0 | [emu2](https://github.com/dmsc/emu2/blob/master/LICENSE) · [emu2-cpm86](https://github.com/johnsonjh/emu2-cpm86/blob/local/cpm86/LICENSE) |
| tnylpo | Open source | [LICENSE](https://gitlab.com/gbrein/tnylpo/-/blob/master/LICENSE) |
| NASM | BSD-2-Clause | [nasm.us](https://www.nasm.us) |
| hexcom.c (Jeffrey H. Johnson) | MIT-0 | [source](https://github.com/johnsonjh/tpzasm/blob/master/src/hexcom.c) |
| XLISP 1.1 (David Betz) | Public domain | [cpm86-ports](https://github.com/tsupplis/cpm86-ports) |
| Intel PL/M-86 3.30 | **Unclear** ⚠️ | [PLM8086Tools.zip](http://www.retroarchive.org/dos/lang/PLM8086Tools.zip) |
| PolyPascal-86 V3.11 (PolyData MicroCenter A/S) | **Unclear** ⚠️ | — |
| Micro Focus Level II COBOL V2.1 | **Unclear** ⚠️ | — |
| TDL BASIC 3.05 (Technical Design Labs) | **Unclear** ⚠️ | — |

## Prerequisites

Install any missing prerequisites with your favorite package manager.

| | Prerequisites |
|---|---|
| **Required** | C/C++ toolchain (C99/C++14): [GCC](https://gcc.gnu.org/) or [Clang](https://clang.llvm.org/) · [`curl`](https://curl.se/) · [CMake](https://cmake.org/) · [`dos2unix`](https://dos2unix.sourceforge.io/) · [`git`](https://git-scm.com/) · [Bash](https://www.gnu.org/software/bash/) · [Coreutils](https://www.gnu.org/software/coreutils/) · [Gzip](https://www.gnu.org/software/gzip/) · [Make](https://www.gnu.org/software/make/) · [`patch`](https://savannah.gnu.org/projects/patch/) · [Screen](https://www.gnu.org/software/screen/) · [`tar`](https://www.gnu.org/software/tar/) · [Wget](https://www.gnu.org/software/wget/) · [Unzip](https://infozip.sourceforge.net/UnZip.html) · [ncurses](https://invisible-island.net/ncurses/) · [XZ Utils](https://tukaani.org/xz/) |
| **Optional** | [Docker](https://www.docker.com/), [Podman](https://podman.io/), or equivalent (for containerised builds) |

## Script Mapping

When several tools can perform the same job, use the highest tier available:

1. **Native** `(native)` — compiled for the host OS, no emulation overhead.
2. **DOS** — `.exe`/`.com` binary run under `emu2`.
3. **CP/M-86** — `.cmd` binary run under `emu2`.
4. **CP/M-80** — `.com` binary run under `tnylpo` — legacy fallback only.

For example, `cpm86_asm86` (CP/M-86, tier 3) is preferred over `cpm_asm86`
(CP/M-80, tier 4), and `cmdinfo` (native, tier 1) is preferred over any DOS wrapper.

All the tools are wrapped in the bin directory for direct usage:

| tool          | program     | quick description                  |
|---------------|-------------|------------------------------------|
| cpm_asm86     | asm86.com   | DR assembler (CP/M-80 tool)        |
| cpm_gencmd    | gencmd.com  | DR H86 converter (CP/M-80 tool)    |
| cpm_xlt86     | xlt86.com   | DR 8080→8086 translator (CP/M-80, via tnylpo) |
| cpm_mbasic    | mbasic.com  | Microsoft Basic 5.21 (8080 CP/M-80, via tnylpo) |
| cpm_obasic    | obasic.com  | Microsoft Basic 4.51 (8080 CP/M-80, via tnylpo) |
| cpm_tdlbasic  | tdlbasic.com | TDL BASIC 3.05 (Z-80 CP/M-80, via tnylpo)       |
| cpm86_asm86   | asm86.cmd   | DR assembler (CP/M-86, via emu2, not working yet)   |
| cpm86_gencmd  | gencmd.cmd  | DR H86 converter (CP/M-86, via emu2, not working yet) |
| cpm86_basic   | basic.cmd   | DR Personal Basic 1.2 (via cpm86)  |
| cpm86_mbasic  | mbasic86.cmd | Microsoft Basic 5.22 (CP/M-86, via emu2) |
| pcdev_mbasic  | mbasic86.com | Microsoft Basic 5.28 (DOS, via emu2)     |
| pcdev_rasm86  | rasm86.exe  | DR assembler                      |
| pcdev_linkcmd | linkcmd.exe | DR linker for CP/M-86              |
| pcdev_link86  | linkcmd.exe | "  "  "  "  "  "                   |
| pcdev_linkexe | linkexe.exe | DR linker for DOS                  |
| pcdev_lib86   | lib86.exe   | DR librarian                       |
| pcdev_masm    | masm.exe    | Microsoft assembler                |
| pcdev_link    | link.exe    | Microsoft linker                   |
| pcdev_exe2bin | exe2bin.exe | Microsoft EXE converter            |
| pcdev_hex2bin | hex2bin.com | Microsoft HEX converter            |
| pcdev_asm     | asm.com     | Microsoft/86-DOS assembler         |
| cmdinfo       | (native)    | CMD info tool (native)             |
| bin2cmd       | (native)    | CMD converter (native)             |
| exe2cmd       | (native)    | EXE to CMD converter (native)      |
| pcdev_cmdinfo | -           | obsolete alias for cmdinfo         |
| pcdev_bin2cmd | -           | obsolete alias for bin2cmd         |
| pcdev_exe2cmd | -           | obsolete alias for exe2cmd         |
| aztec34_as    | as.exe      | Aztec Assembler.                   |
| aztec34_cc    | cc.exe      | Aztec K&R C compiler               |
| aztec34_sqz   | sqz.exe     | Aztec C object optimizer           |
| aztec34_link  | ln.exe      | Aztec C linker                     |
| aztec34_lib   | lb.exe      | Aztec C lib                        |
| aztec34_ord   | ord.exe     | Aztec C library ordering helper    |
| aztec34_obd   | obd.exe     | Aztec C object dump                |
| aztec34_obj   | obj.exe     | Aztec C object lister              |
| aztec34_hex86 | hex86.exe   | Aztec C H86 generator              |
| aztec42_as    | as.exe      | Aztec Assembler.                   |
| aztec42_cc    | cc.exe      | Aztec ANSI C compiler              |
| aztec42_sqz   | sqz.exe     | Aztec C object optimizer           |
| aztec42_link  | ln.exe      | Aztec C linker                     |
| aztec42_lib   | lb.exe      | Aztec C lib                        |
| aztec42_ord   | ord.exe     | Aztec C library ordering helper    |
| aztec42_obd   | obd.exe     | Aztec C object dump                |
| aztec42_obj   | obj.exe     | Aztec C object lister              |
| aztec42_hex86 | hex86.exe   | Aztec C H86 generator              |
| ppcpm_ppas    | ppas.cmd       | PolyPascal-86 V3.11 compiler+editor (CP/M-86) |
| ppcpm_ppbs    | ppbs.cmd       | PolyPascal-86 V3.11 BCD business variant (CP/M-86) |
| ppcpm_pp87    | pp87.cmd       | PolyPascal-86 V3.11 8087 FPU variant (CP/M-86) |
| ppdos_ppas    | ppas.com       | PolyPascal-86 V3.11 compiler+editor (DOS) |
| ppdos_ppbs    | ppbs.com       | PolyPascal-86 V3.11 BCD business variant (DOS) |
| ppdos_pp87    | pp87.com       | PolyPascal-86 V3.11 8087 FPU variant (DOS) |
| cpm86_turbo   | turbo.cmd      | Turbo Pascal 3.01 (CP/M-86)             |
| drpmt_pc      | mt+86.cmd      | Pascal MT+ 3.3 compiler                 |
| drpmt_link    | linkmt.cmd     | Pascal MT+ 3.3 linker                   |
| drpmt_asm     | asmt86.cmd     | Pascal MT+ 3.3 assembler                |
| drfcpm_f77    | f77.cmd        | DR Fortran-77 4.0 compiler              |
| drfcpm_link   | linkcmd.exe     | DR Fortran-77 4.0 linker                |
| drccpm_cc     | drc860+861.cmd | DR C 1.11 compiler (two-pass)           |
| drccpm_link   | linkcmd.exe     | DR C 1.11 linker                        |
| m2cb_cvt      | m2cbasic.cmd | M2CBASIC 1.4 MBASIC to CBASIC converter |
| drcbcpm_bc    | cb86.exe    | DR cbasic compiler for CP/M-86     |
| drcbcpm_link  | link86.exe  | DR cbasic linker for CP/M-86       |
| drcbdos_bc    | cb86.exe    | DR cbasic compiler for DOS         |
| drcbdos_link  | linkexe.exe | DR cbasic linker for DOS           |
| cpm86         | cpm86.exe   | CP/M-86 emulator (via emu2)        |
| emu2          | (native)    | x86 DOS/CP/M-86 emulator           |
| tnylpo        | (native)    | CP/M-80 emulator                   |
| hexcom        | (native)    | HEX to COM translation             |
| doscat        | (native)    | Truncate files beyond ^Z           |
| nasm          | (native)    | Netwide assembler                  |
| upx           | (native)    | executable packer                  |
| intel_plm86   | plm86.exe   | Intel PL/M-86 3.30 compiler        |
| intel_asm86   | asm86.exe   | Intel ASM-86 assembler             |
| intel_link    | link.exe    | Intel linker                       |
| intel_lib86   | lib86.exe   | Intel librarian                    |
| intel_loc86   | loc86.exe   | Intel locator                      |
| drpli_pc      | pli.cmd     | DR PL/I-86 1.0 compiler                     |
| drpli_link    | linkcmd.exe  | DR PL/I-86 1.0 linker                        |
| cpm86_xlisp   | xlisp.cmd   | XLISP 1.1 interpreter (built from source) |
| mfcobol_cc    | cobol.cmd   | Micro Focus Level II COBOL compiler (CP/M-86) |
| mfcobol_run   | run.cmd     | Micro Focus Level II COBOL runtime (CP/M-86)  |

## Fetching the tools

The development environment can be assembled by using the following steps:
```
./fetch_tools
export PATH="$(pwd -P)"/bin
```

| Component | Source |
|-----------|--------|
| Aztec C 4.2 | [AztecC86.zip](https://www.aztecmuseum.ca/AztecC86.zip) |
| Aztec C 3.4 | [az8634b.zip](https://www.aztecmuseum.ca/az8634b.zip) |
| RASM-86, LINK-86, LIB-86 | [tools86.zip](http://www.cpm.z80.de/download/tools86.zip) |
| ASM-86 + GENCMD (CP/M-80) | [mpm862sr.zip](http://www.cpm.z80.de/download/mpm862sr.zip) |
| XLT86 1.3 | [xlt86.zip](http://www.cpm.z80.de/download/xlt86.zip) |
| ASM-86 + GENCMD (CP/M-86) | [cpm86-kernel](https://github.com/tsupplis/cpm86-kernel) (GitHub) |
| DR C 1.11 + extra headers | [drc_86.zip](http://www.cpm.z80.de/download/drc_86.zip) · [drc86111.zip](http://www.cpm.z80.de/download/drc86111.zip) |
| DR Fortran-77 4.0 | [f77-86.zip](http://www.cpm.z80.de/download/f77-86.zip) |
| Turbo Pascal 3.02 (CP/M-86) | [RC750 image](https://rc700.dk/software/RC750_TurboPascal_v3.01a/files/RC750_TurboPascal_v3.01a.img.bz2) (stored in `src/turbo`) |
| PolyPascal-86 3.11 | [PolyPascal](https://github.com/johnsonjh/PolyPascal) (GitHub) |
| Pascal MT+ 3.3 | [mt8633cp.zip](http://www.cpm.z80.de/download/mt8633cp.zip) |
| DR CBASIC 2.0/2.1 + libs | [cbasic86.zip](http://www.cpm.z80.de/download/cbasic86.zip) · [cb86toys.zip](http://www.cpm.z80.de/download/cb86toys.zi) |
| M2CBASIC 1.4 | [m2cbasic.zip](http://www.cpm.z80.de/download/m2cbasic.zip) |
| DR Personal Basic 1.2 | [datamuseum.dk](https://datamuseum.dk/wiki/Bits:30002879) (stored in `src/drtools/basic.cmd`) |
| MS Basic 5.21 (CP/M-80) | repository (local copy) |
| MASM, LINK, ASM, EXE2BIN, HEX2BIN, MS Basic 86/80 | [MS-DOS](https://github.com/microsoft/MS-DOS) (GitHub) + repository |
| cmdtools (cmdinfo, bin2cmd, exe2cmd) | [cpm86-cmdtools](https://github.com/tsupplis/cpm86-cmdtools) (GitHub, built natively) |
| NASM | [nasm.us snapshots](https://nasm.us/pub/nasm/snapshots/) |
| upx | [upx releases](https://github.com/upx/upx/releases/download/v5.2.1/upx-5.2.1-src.tar.xz) (GitHub) |
| emu2-cpm86 | [emu2-cpm86](https://gitlab.com/johnsonjh/emu2-cpm86) (GitLab, fork of [emu2](https://github.com/dmsc/emu2)) |
| tnylpo | [tnylpo](https://gitlab.com/gbrein/tnylpo.git) (GitLab) |
| Intel PL/M-86 3.30 tools | [PLM8086Tools.zip](http://www.retroarchive.org/dos/lang/PLM8086Tools.zip) |
| DR PL/I-86 1.0 | [pli86.zip](http://www.cpm.z80.de/download/pli86.zip) |
| XLISP 1.1 | [cpm86-ports](https://github.com/tsupplis/cpm86-ports) (GitHub, built from source with aztec42) |
| Micro Focus Level II COBOL | [LII-COBOL-CPM86.zip](https://www.roug.org/retrocomputing/languages/cobol/microfocus/LII-COBOL-CPM86.zip) |
| TDL BASIC 3.05 (CP/M-80) | [Nutting_ICE.zip](https://bitsavers.org/bits/Nutting_Assoc/Nutting_ICE.zip) |

Clearing the directory is achieved by:
```
./clear_tools
```

### Download archive / offline rebuilds

Every file downloaded by `fetch_tools` (the Aztec, DR, CB86 and NASM archives,
but not the `git` checkouts) is cached under `archive/` instead of being deleted.

This keeps the build environment reproducible even if an upstream disappears:

- If a download fails, `fetch_tools` falls back to the cached copy in `archive/`.
- Set `ARCHIVE_FIRST=1` to rebuild entirely from `archive/` without touching the
  network (e.g. offline, or to pin exact versions):
  ```
  ARCHIVE_FIRST=1 ./fetch_tools
  ```
- `clear_tools` will **not** remove the `archive/`, so a cleared tree can always
  be rebuilt from the local cache.

## Testing

The cpm86 tool is a cpm86 emulator for dos pulled with Aztec C.  We have reverse engineered it and patch it to fix a few bugs and add some new
features.  It should not be relied on fully as some bugs surely remain, but it can be experimented with and works for many programs.  For build
tools DOS/CP/M-80 emulations and DOS/CP/M-80 cross compilations are far better options and proper emulation of CP/M-86 is the best option for
testing; PCE is a very good choice (http://www.hampa.ch/pce/).

Note that the enhanced cpm86 emulator supports padding records with `EOF` instead of `NULL` which may make working with text files easier, but
because this behavior does not match real CP/M-86, it's disabled by default and only enabled if you set `CPM86_EOF=1`, for example:
```
env CPM86_EOF=1 cpm86 program.cmd
```

This may be a next step:
- automating pce, cpmtools
- bootable/up-to-date CP/M-86 floppy ...

## Docker image

A `Dockerfile` is provided for this environment.

To build the container, *from the repository root*, run:

```
docker build --progress=plain --rm=true -t cpm86:latest -f docker/Dockerfile .
```

to user the created image, just ensure that the local path where the compilation happens is mounted properly:

```
docker run -it --rm -h cpm86 -v "$(pwd -P)":/work -w /work cpm86 pcdev_rasm86 helloa.a86
docker run -it --rm -h cpm86 -v "$(pwd -P)":/work -w /work cpm86 aztec34_cc helloc.c
```
The only difference on the command line happens with \;
```
pcdev_masm hellod \;
```
... becomes ...
```
docker run -it --rm -h cpm86 -v "$(pwd -P)":/work -w /work cpm86 pcdev_masm hellod '\\;'
```

## Using the tools

Finally, a simple Makefile with a sample c, assembler for rasm86, assembler for asm86 is provided:
```
./test_crossdev
```

### Basic Programs

<table width="100%">
<tr><th>Variant</th><th>Commands</th></tr>
<tr><td>DR CBASIC (compiled, CP/M-86)</td><td><pre>
drcbcpm_bc hellor.bas
drcbcpm_link hellor.cmd=hellor.o
cmdinfo hellor.cmd
</pre></td></tr>
<tr><td>MS BASIC → CBASIC (convert + compile)</td><td><pre>
m2cb_cvt hellomsb.bas
mv hellomsb.cb hellocvt.cb
drcbcpm_bc hellocvt.cb
drcbcpm_link hellocvt
cmdinfo hellocvt.cmd
</pre></td></tr>
<tr><td>MS Basic 5.22 (interpreted, CP/M-86)</td><td><pre>cpm86_mbasic hellomsb.bas</pre></td></tr>
<tr><td>MS Basic 5.28 (interpreted, DOS)</td><td><pre>pcdev_mbasic hellomsb.bas</pre></td></tr>
<tr><td>MS Basic 5.21 (CP/M-80, via tnylpo)</td><td><pre>cpm_mbasic hellomsb.bas</pre></td></tr>
<tr><td>MS Basic 4.51 "Old Basic" (CP/M-80, via tnylpo)</td><td><pre>cpm_obasic hellomsb.bas</pre></td></tr>
<tr><td>TDL BASIC 3.05 (Z-80 CP/M-80, via tnylpo)</td><td><pre>cpm_tdlbasic hellomsb.bas</pre></td></tr>
<tr><td>DR Personal Basic 1.2 (interactive)</td><td><pre>cpm86_basic</pre></td></tr>
</table>

### C Programs

`aztec42` is the preferred compiler — it supports almost-ANSI syntax and its
runtime and CP/M-86 library are actively improved. Use `aztec34` only when
strict K&R compatibility is required or for legacy builds.

`drccpm_cc` is the DR C 1.11 compiler for CP/M-86. It runs the two passes
internally: `drc860.cmd` (preprocessor) produces `ctemp.tok`, then
`drc861.cmd` (code generator) consumes it. The `ctemp.tok` file is cleaned up
via a trap regardless of success or failure. The DR C runtime libraries
(`clearl.l86`, `clears.l86`) and `link86.cmd` are all staged in
`share/drc86cpm/`. The redundant DISK3 tools (`rasm86.cmd`, `lib86.cmd`,
`xref86.cmd`) are not staged.

`drccpm_link` and `drfcpm_link` are thin wrappers around `link86.cmd` — they
set drive A to the current working directory (so `.obj` files are found there)
and drive D to the tool share directory (so runtime libraries are found there).
Link options are passed explicitly by the caller using the LINK-86 `[option]`
syntax. The most common options are:

| Option | Description |
|---|---|
| `$LD` | Search drive D for `.l86` runtime libraries |

This is not necessary with the wrappers.

<table width="100%">
<tr><th>Variant</th><th>Commands</th></tr>
<tr><td>Aztec C 4.2 (preferred, almost-ANSI)</td><td><pre style="margin:0">
aztec42_cc helloc.c
aztec42_sqz helloc.o
aztec42_link -o helloc.cmd helloc.o -lc86
cmdinfo helloc.cmd
</pre></td></tr>
<tr><td>Aztec C 3.4 (K&R legacy)</td><td><pre style="margin:0">
aztec34_cc helloc.c
aztec34_sqz helloc.o
aztec34_link -o helloc.cmd helloc.o -lc86
cmdinfo helloc.cmd
</pre></td></tr>
<tr><td>DR C 1.11</td><td><pre style="margin:0">
drccpm_cc -ohellodrc.obj hellodrc.c
drccpm_link 'hellodrc.cmd=hellodrc.obj'
cmdinfo hellodrc.cmd
</pre></td></tr>
</table>

#### C runtime startup

`aztec34_link`/`aztec42_link` automatically prepend the matching C runtime
startup object, so the `$begin -> Croot_ -> main -> exit` entry/exit code is
always present. The startup is selected from the C library you link against:

| library            | target / model | startup     |
|--------------------|----------------|-------------|
| `-lc86`            | CP/M-86 small  | `begin86.o` |
| `-lc`   / `-lclc`  | MS-DOS small   | `sbegin.o`  |
| `-lcl`  / `-lcld`  | MS-DOS large   | `lbegin.o`  |

These startup objects are produced from the C libraries by `fetch_tools`
(`src/fetch/cross_aztec_startups`). Without this, Aztec's single-pass `ln` only pulls
the startup from the library on demand, so a program that references no libc
symbol — e.g. `int main(void){ return 0; }` — would link with **no** startup and
crash on exit, and a program that references the startup indirectly (e.g.
`exit()`) could fail to link (`Undefined symbol: _exit_`) unless the library was
ordered with `ord` or passed twice (`-lc86 -lc86`). Prepending the startup as a
command-line object fixes all of these cases, so neither `ord` nor a doubled
library is needed. To restore the old behaviour set `AZTEC_NOSTARTUP=1`, or force
a specific startup with `AZTEC_STARTUP=<obj-in-lib>`.

### Assembler Programs

<table width="100%">
<tr><th>Variant</th><th>Commands</th></tr>
<tr><td>RASM-86</td><td><pre style="margin:0">
pcdev_rasm86 helloa.a86 '$' pz sz
pcdev_linkcmd helloa '[$sz]'
cmdinfo helloa.cmd
</pre></td></tr>
<tr><td>ASM-86 (CP/M-80)</td><td><pre style="margin:0">
cpm_asm86 hellob.a86
cpm_gencmd hellob.h86
cmdinfo hellob.cmd
</pre></td></tr>
<tr><td>MASM</td><td><pre style="margin:0">
pcdev_masm hellod \;
pcdev_link hellod \;
pcdev_exe2bin hellod.exe
bin2cmd hellod.bin hellod.cmd
cmdinfo hellod.cmd
</pre></td></tr>
<tr><td>NASM</td><td><pre style="margin:0">
nasm hellon.asm -fbin -o hellon.bin
bin2cmd hellon.bin hellon.cmd
cmdinfo hellon.cmd
</pre></td></tr>
<tr><td>XLT86 (8080→8086) + ASM-86</td><td>Pass basename without extension. XLT86 translates <code>CALL 5</code> to <code>INT 224</code> (0E0h). "Number of Errors: 1" for <code>ORG 100h</code> is expected and harmless.<pre style="margin:0">
cpm_xlt86 hello80
cpm86_asm86 hello80.a86
cpm86_gencmd hello80.h86
cmdinfo hello80.cmd
</pre></td></tr>
</table>

### Misc Languages

<table width="100%">
<tr><th>Language</th><th>Commands</th></tr>
<tr><td>PolyPascal-86 V3.11 (CP/M-86)</td><td>
Interactive editor+compiler. Standard (<code>ppas</code>), BCD (<code>ppbs</code>), 8087 (<code>pp87</code>) variants.<pre style="margin:0">
ppcpm_ppas
</pre>
The editor opens first. Use <code>COMPILE</code> then <code>RUN</code>. <code>PPAS.HLP</code> and <code>PPAS.ERM</code> found automatically on drive D.
</td></tr>
<tr><td>Pascal MT+ 3.3 (CP/M-86)</td><td><pre style="margin:0">
drpmt_pc hellomt
drpmt_link hellomt.cmd=hellomt,p:paslib/s
cmdinfo hellomt.cmd
</pre></td></tr>
<tr><td>DR Fortran-77 4.0 (CP/M-86, small model)</td><td>
Both compiler passes (<code>f77.cmd</code> + <code>codegen.cmd</code>) run internally; <code>.cil</code>/<code>.cym</code> intermediates cleaned up automatically.<pre style="margin:0">
drfcpm_f77 hellof.f77
drfcpm_link 'hellof.cmd=hellof'
cmdinfo hellof.cmd
</pre></td></tr>
<tr><td>Micro Focus Level II COBOL V2.1 (CP/M-86)</td><td>
Compiles <code>.cbl</code> to bytecode <code>.int</code>; runtime executes it.<pre style="margin:0">
mfcobol_cc hellocbl.cbl
mfcobol_run hellocbl.int
</pre>
The <code>cobol</code> overlay is staged automatically and cleaned up on exit.
</td></tr>
<tr><td>Intel PL/M-86 3.30</td><td><pre style="margin:0">
intel_asm86 scd.a86
intel_plm86 hellop.plm debug 'optimize(3)'
pcdev_linkcmd hellop=scd,hellop '[data[ori[0]]'
cmdinfo hellop.cmd
</pre></td></tr>
<tr><td>DR PL/I-86 1.0 (CP/M-86)</td><td><pre style="margin:0">
drpli_pc hellopi
drpli_link 'hellopi.cmd=hellopi'
cmdinfo hellopi.cmd
</pre></td></tr>
<tr><td>XLISP 1.1 (CP/M-86)</td><td>
Interactive by default. Pass a <code>.lsp</code> file to load and run it; use <code>(exit)</code> to terminate without waiting for input.<pre style="margin:0">
cpm86_xlisp hello.lsp
</pre>
For an interactive session:<pre style="margin:0">
cpm86_xlisp
</pre>
Drive D = tool dir (contains <code>xlisp.cmd</code>), drive C = CWD, so <code>.lsp</code> files are found automatically.
</td></tr>
</table>

