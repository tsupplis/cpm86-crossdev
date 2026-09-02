# CP/M-86 & MS-DOS Cross Development Environment

This project regroups a simple method to create a cross development environment for CP/M-86. It happens that all DOS based tools can also be used for building DOS programs.
This project was developed for myself in a nerdy spirit and for a lot of fun. Just sharing without any guarantees.

Languages supported are:
- C (K&R and almost ANSI)
- Assembler
- Basic (DR CBASIC, DR Personal Basic, Microsoft Basic)
- Pascal (Pascal MT+, Turbo Pascal)
- Fortran-77
- PL/M-86

## Thanks

- dmsc (https://github.com/dmsc) for the emu2 emulator, without which this project would not be nearly as pleasant to use.
- johnsonjh (https://github.com/johnsonjh) for the emu2-cpm86 fork, the hexcom rewrite, and portability/build fixes contributed to this project.

## Where to find CP/M-86?

The source for CP/M-86 doc, sources and binaries is http://www.cpm.z80.de.

A cleaned-up distribution and kernel is available at https://github.com/tsupplis/cpm86-kernel. This distribution is working well in virtual environments, patched with all known patches, 'y2k' friendly (it contains the version of tod which sources are in this project) and AT friendly.

## Key tools for CP/M-86 development
- aztec c compiler version 4.2/4.10d (**preferred**, almost ANSI; the CP/M-86 library (c86.lib) is patched and recompiled from sources and actively improved, a DOS 1.1 library (d11.lib) is also provided), documentation at (https://www.aztecmuseum.ca/docs/Aztec_C_MSDOS_4.10C_Commercial_Apr88.pdf)
- aztec c compiler version 3.4/3.40a (K&R legacy; the CP/M-86 library is provided as c86.lib, patched but otherwise left as-is)
- rasm86 1.4a / link86 2.02 / lib86 1.3 DOS versions from Digital Research
- asm86 1.1 and gencmd from Digital Research (CP/M-80 and CP/M-86 versions)
- XLT86 1.3 from Digital Research (http://www.cpm.z80.de/download/xlt86.zip) — 8080/8085 assembly language to 8086 assembly language translator (`xlt86.com` + overlays `xlt00.ovl`, `xlt01.ovl`); CP/M-80 version, run via `tnylpo`
- DR C 1.11 for CP/M-86 (http://www.cpm.z80.de/download/drc_86.zip) — compiler passes (`drc860.cmd`–`drc862.cmd`), preprocessor (`drcrpp.cmd`), runtime (`startup.a86`, `clearl.l86`, `clears.l86`) and standard headers (`ctype.h`, `errno.h`, `portab.h`, `setjmp.h`, `stdio.h`); extended with extra headers from (http://www.cpm.z80.de/download/drc86111.zip): `alloc.h`, `dos.h`, `bios.h`, `float.h`, `conio.h` and PC BIOS glue source `pcbios.a86` (needs assembling with `cpm86_asm86` before use)
- DR Fortran-77 4.0 for CP/M-86 (http://www.cpm.z80.de/download/f77-86.zip) — compiler (`f77.cmd`, `codegen.cmd`), runtime (`clears.l86`, `clearl.l86`, `f32s.obj`, `f32l.obj`, `8087.sim`), Fortran/assembly interface example (`pkmemras.a86`, assembled to `pkmemras.obj`)
- Pascal MT+ 3.3 for CP/M-86 (http://www.cpm.z80.de/download/mt8633cp.zip) — compiler (`mt+86.cmd`), linker (`linkmt.cmd`), assembler (`asmt86.cmd`), runtime (`paslib.r86`, floating point modules, I/O modules), utilities (`strip.cmd`, `sz.cmd`, `nm.cmd`, `dis86.cmd`)
- Turbo Pascal 3.02 for CP/M-86 — the last CP/M-86 release from Borland; stored in `src/turbo`, extracted from the RC750 disk image at (https://rc700.dk/software/RC750_TurboPascal_v3.01a/files/RC750_TurboPascal_v3.01a.img.bz2); includes the compiler (`turbo.cmd`), installer (`tinst.cmd`) and message/data files
- CB-86 CBASIC compiler version 2.0 (CP/M-86) / 2.1 (DOS) and libraries from Digital Research
- M2CBASIC 1.4 for CP/M-86 (http://www.cpm.z80.de/download/m2cbasic.zip) — Microsoft BASIC to CBASIC source converter; includes the translator (`m2cbasic.cmd` with overlays `m2cb1.ovr`–`m2cb9.ovr`), runtime libraries (`m2cblib.bas`, `m2cbrio.bas`, `m2cbrio.rlo`, `m2cbrio.am`) and companion utility (`m2cbconv.cmd`)
- DR Personal Basic 1.2 for CP/M-86 (recovered from https://datamuseum.dk/wiki/Bits:30002879, stored in `src/dr/basic.cmd`; the original 1.1 release is also available at http://www.cpm.z80.de/download/pbasic86.zip)
- nasm netwide assembler version 3.02
- masm 1.10, link 2.0, asm 2.44a, exe2bin 1.1, hex2bin from Microsoft (the version of masm in this repository has been patched to work with emu2 and other emulators, see https://github.com/tsupplis/pcdos11-hacking for details). asm.com and hex2bin.com have been rebuilt from modified sources at https://github.com/tsupplis/pcdos11-hacking .
- Microsoft Basic 5.22 for CP/M-86 (`mbasic86.cmd`) and 5.28 for DOS (`mbasic86.com`), both included in the repository as patched binaries (unpatched originals kept as `mbasic86.org` / `mbasorig.com`); patch notes at [src/microsoft/msbasic-patch.md](src/microsoft/msbasic-patch.md)
- Intel PL/M-86 3.30 compiler (plm86.exe), Intel ASM-86 2.1 assembler (asm86.exe), Intel linker 2.30 (link.exe) and librarian 2.1 (lib86.exe) and locator 2.5(loc86.exe) from the retroarchive (http://www.retroarchive.org/dos/lang/PLM8086Tools.zip). A real-world example of a CP/M-86 project written in PL/M-86 is available at (https://github.com/tsupplis/ccpm86-y2k).

- The Super Cool emu2 DOS/CP/M-86 emulator version 2021.01 (https://github.com/dmsc/emu2). This is an incredible way to bring dos command line development tools to a modern and up to date shell/make/whatever based dev environment. Another stunning emulator. Emu2 and PCE are an incredible pair. We use the emu2-cpm86 fork (https://github.com/johnsonjh/emu2-cpm86), which adds and keeps improving CP/M-86 support on top of upstream emu2.
- We also need to run some CP/M-80 programs, for that, the tnylpo emulator is used (https://gitlab.com/gbrein/tnylpo). It works very well with asm86.com and gencmd.com programs.
- upx executable packer version 5.2.1 (https://github.com/upx/upx)

## License Considerations

- Microsoft MS-DOS tools (DOS 1.1 and Basic 86) are under MITS license (https://github.com/microsoft/MS-DOS/blob/master/LICENSE.md)
The following tools are not included and downloaded by the fetch tool but require you to understand the conditions of usage:
- The Aztec C use conditions is documented at (https://www.aztecmuseum.ca/intro.htm#intro)
- The DR tools usage is documented at (http://www.cpm.z80.de/license.html) and (http://www.cpm.z80.de/faq.html)
- XLT86 1.3 is a Digital Research product; usage is documented at (http://www.cpm.z80.de/license.html) and (http://www.cpm.z80.de/faq.html)
- The DR C 1.11 for CP/M-86 is documented at (http://www.cpm.z80.de/license.html) and (http://www.cpm.z80.de/faq.html)
- The DR Fortran-77 4.0 for CP/M-86 is documented at (http://www.cpm.z80.de/license.html) and (http://www.cpm.z80.de/faq.html)
- Pascal MT+ 3.3 for CP/M-86 is a Digital Research product; usage is documented at (http://www.cpm.z80.de/license.html) and (http://www.cpm.z80.de/faq.html)
- Turbo Pascal 3.01A for CP/M-86 is a Borland product. It is freely usable but not open source. The files are stored in this repository as extracted from the RC750 disk image at (https://rc700.dk/software/RC750_TurboPascal_v3.01a/files/RC750_TurboPascal_v3.01a.img.bz2).
- The DR CBASIC compiler 2.0 for CP/M-86 and 2.1 for DOS is documented at (http://www.cpm.z80.de/license.html) and (http://www.cpm.z80.de/faq.html)
- M2CBASIC 1.4 is a Digital Research product; usage is documented at (http://www.cpm.z80.de/license.html) and (http://www.cpm.z80.de/faq.html)
- The DR Personal Basic 1.2 for CP/M-86 is documented at (http://www.cpm.z80.de/license.html) and (http://www.cpm.z80.de/faq.html)
- emu2 and tnylpo are open source with their licenses described respectively at (https://github.com/dmsc/emu2/blob/master/LICENSE) and (https://gitlab.com/gbrein/tnylpo/-/blob/master/LICENSE); the emu2-cpm86 fork we build is licensed under GPL-2.0, see (https://github.com/johnsonjh/emu2-cpm86/blob/local/cpm86/LICENSE)
- nasm license terms can be found at (https://www.nasm.us)
- src/tools/hexcom.c is a DRI HEXCOM 3.00 compatible reimplementation by Jeffrey H. Johnson, imported from https://github.com/johnsonjh/tpzasm/blob/master/src/hexcom.c under the MIT-0 license
- I am not guaranteeing in any ways the components mentioned above. You are of course free to use these at your own risk if you accept the conditions of usage given above.

> [!WARNING]
> The Intel PL/M-86 3.30 tools licensing is unclear; their usage is left to the discretion of the end user.

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
| cpm86_asm86   | asm86.cmd   | DR assembler (CP/M-86, via emu2)   |
| cpm86_gencmd  | gencmd.cmd  | DR H86 converter (CP/M-86, via emu2) |
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
| turbo         | turbo.cmd      | Turbo Pascal 3.01 (CP/M-86)             |
| drpmt_pc      | mt+86.cmd      | Pascal MT+ 3.3 compiler                 |
| drpmt_link    | linkmt.cmd     | Pascal MT+ 3.3 linker                   |
| drpmt_asm     | asmt86.cmd     | Pascal MT+ 3.3 assembler                |
| drfcpm_f77    | f77.cmd        | DR Fortran-77 4.0 compiler              |
| drfcpm_link   | link86.cmd     | DR Fortran-77 4.0 linker                |
| drccpm_cc     | drc860+861.cmd | DR C 1.11 compiler (two-pass)           |
| drccpm_link   | link86.cmd     | DR C 1.11 linker                        |
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

## Fetching the tools

The development environment can be assembled by using the following steps:
```
./fetch_tools
export PATH=`pwd`/bin
```
it pulls the following:
- aztec 3.4 c compiler  (https://www.aztecmuseum.ca/az8634b.zip)
- link86, lib86 and rasm86 (http://www.cpm.z80.de/download/tools86.zip)
- asm86 and gencmd CP/M-80 versions (http://www.cpm.z80.de/download/mpm862sr.zip)
- XLT86 1.3 (http://www.cpm.z80.de/download/xlt86.zip)
- asm86 and gencmd CP/M-86 versions (https://github.com/tsupplis/cpm86-kernel)
- DR C 1.11 (http://www.cpm.z80.de/download/drc_86.zip) + extra headers (http://www.cpm.z80.de/download/drc86111.zip)
- DR Fortran-77 4.0 (http://www.cpm.z80.de/download/f77-86.zip)
- Turbo Pascal 3.02 for CP/M-86 (stored in `src/turbo`, extracted from https://rc700.dk/software/RC750_TurboPascal_v3.01a/files/RC750_TurboPascal_v3.01a.img.bz2)
- Pascal MT+ 3.3 (http://www.cpm.z80.de/download/mt8633cp.zip)
- cb86 2.0/2.1 and libraries (http://www.cpm.z80.de/download/cbasic86.zip) and (http://www.cpm.z80.de/download/cb86toys.zi)
- M2CBASIC 1.4 (http://www.cpm.z80.de/download/m2cbasic.zip)
- DR Personal Basic 1.2 (stored in `src/dr/basic.cmd`, recovered from https://datamuseum.dk/wiki/Bits:30002879; original 1.0 at http://www.cpm.z80.de/download/pbasic86.zip)
- masm, link, asm, exe2bin, hex2bin (local copies from https://github.com/microsoft/MS-DOS)
- cmdtools — cmdinfo, bin2cmd, exe2cmd built natively from (https://github.com/tsupplis/cpm86-cmdtools)
- nasm (https://www.nasm.us/pub/nasm/releasebuilds/3.02/nasm-3.02.tar.gz)
- upx (https://github.com/upx/upx/releases/download/v5.2.0/upx-5.2.0-src.tar.xz)
- emu2-cpm86, a CP/M-86 enabled fork of emu2 (https://github.com/johnsonjh/emu2-cpm86), based on the upstream emu2 project (https://github.com/dmsc/emu2)
- tnylpo (https://gitlab.com/gbrein/tnylpo.git)
- PL/M-86 3.30 tools (http://www.retroarchive.org/dos/lang/PLM8086Tools.zip)

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

A Dockerfile is provided for this environment. To build the image from the docker directory, jusr run make, alternatively:
```
docker build --rm=true -t cpm86:latest -f Dockerfile .
```

to user the created image, just ensure that the local path where the compilation happense ismounted properly:

```
docker run -it --rm -h cpm86 -v `pwd`:/work -w /work cpm86 pcdev_rasm86 helloa.a86
docker run -it --rm -h cpm86 -v `pwd`:/work -w /work cpm86 aztec34_cc helloc.c
```
The only difference on the command line happens with \;
```
pcdev_masm hellod \; 
```
... becomes ...
```
docker run -it --rm -h cpm86 -v `pwd`:/work -w /work cpm86 pcdev_masm hellod '\\;' 
```

## Using the tools

Finally, a simple Makefile with a sample c, assembler for rasm86, assembler for asm86 is provided:
```
./build_demo
```

### Basic Programs

DR CBASIC — compiled to a CP/M-86 binary:
```
drcbcpm_bc hellor.bas
drcbcpm_link hellor.cmd=hellor.o
cmdinfo hellor.cmd
```

Microsoft BASIC to CBASIC — convert then compile:
```
m2cb_cvt hellomsb.bas
mv hellomsb.cb hellocvt.cb
drcbcpm_bc hellocvt.cb
drcbcpm_link hellocvt
cmdinfo hellocvt.cmd
```

Microsoft Basic 5.22 — interpreted, run directly (CP/M-86 version):
```
cpm86_mbasic hellomsb.bas
```

Microsoft Basic 5.28 — interpreted, run directly (DOS version):
```
pcdev_mbasic hellomsb.bas
```

DR Personal Basic 1.2 — interactive interpreter:
```
cpm86_basic
```

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

Drive A is always mapped to the CWD by both wrappers, so `.obj` files in the
current directory are found automatically without needing `$OA`.

```
aztec42_cc helloc.c
aztec42_sqz helloc.o
aztec42_link -o helloc.cmd helloc.o -lc86
cmdinfo helloc.cmd
```
or with the legacy K&R compiler ...
```
aztec34_cc helloc.c
aztec34_sqz helloc.o
aztec34_link -o helloc.cmd helloc.o -lc86
cmdinfo helloc.cmd
```
or with DR C 1.11 ...
```
drccpm_cc -ohellodrc.obj hellodrc.c
drccpm_link 'hellodrc.cmd=hellodrc.obj [$LD]'
cmdinfo hellodrc.cmd
```

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
(`src/fetch/buildstartups`). Without this, Aztec's single-pass `ln` only pulls
the startup from the library on demand, so a program that references no libc
symbol — e.g. `int main(void){ return 0; }` — would link with **no** startup and
crash on exit, and a program that references the startup indirectly (e.g.
`exit()`) could fail to link (`Undefined symbol: _exit_`) unless the library was
ordered with `ord` or passed twice (`-lc86 -lc86`). Prepending the startup as a
command-line object fixes all of these cases, so neither `ord` nor a doubled
library is needed. To restore the old behaviour set `AZTEC_NOSTARTUP=1`, or force
a specific startup with `AZTEC_STARTUP=<obj-in-lib>`.

### Assembler Programs with rasm86
```
pcdev_rasm86 helloa.a86 '$' pz sz
pcdev_linkcmd helloa '[$sz]'
cmdinfo helloa.cmd
```

### Assembler Programs with asm86
```
cpm86_asm86 hellob.a86
cpm86_gencmd hellob.h86
cmdinfo hellob.cmd
```
or using the legacy CP/M-80 binaries via tnylpo:
```
cpm_asm86 hellob.a86
cpm_gencmd hellob.h86
cmdinfo hellob.cmd
```

### Assembler Programs with masm
```
pcdev_masm hellod \;
pcdev_link hellod \;
pcdev_exe2bin hellod.exe
bin2cmd hellod.bin hellod.cmd
cmdinfo hellod.cmd
```

### Assembler Programs with nasm
```
nasm hellon.asm -fbin -o hellon.bin
bin2cmd hellon.bin hellon.cmd
cmdinfo hellon.cmd
```

### 8080 to 8086 translation with XLT86

`cpm_xlt86` translates an 8080/8085 assembly source file (`.asm`) into an
8086 assembly source file (`.a86`) ready for `asm86`. It is a CP/M-80 tool
run via `tnylpo`; the wrapper maps the tool directory (with overlays
`xlt00.ovl` and `xlt01.ovl`) as the default drive A, and the CWD as drive B
so translated output lands alongside the source.

Pass the basename without extension — XLT86 appends `.asm` for input and
writes `.a86` as output. Then assemble and package with the `asm86`/`gencmd`
pipeline:

```
cpm_xlt86 hello80
cpm86_asm86 hello80.a86
cpm86_gencmd hello80.h86
cmdinfo hello80.cmd
```

XLT86 translates `CALL 5` (CP/M BDOS entry) directly to `INT 224` (0E0h),
so no special handling is needed for standard CP/M calls. XLT86 reports
"Number of Errors: 1" for an `ORG 100h` directive (which has no 8086
equivalent); this is expected and the output `.a86` is complete and correct.

### Pascal Programs

Pascal MT+ 3.3 — compiled to a CP/M-86 binary:
```
drpmt_pc hellomt
drpmt_link hellomt.cmd=hellomt,p:paslib/s
cmdinfo hellomt.cmd
```

### Fortran-77 Programs

`drfcpm_f77` handles both compiler passes internally (`f77.cmd` then
`codegen.cmd`), cleaning up the `.cil`/`.cym` intermediates via a trap.
`drfcpm_link` sets drive A to the CWD and drive D to `share/f7786cpm/`.
The linker finds `.obj` files on drive A (CWD) and `.l86` runtime libraries
on drive D via `[$LD]`. The `8087.sim` floating-point simulator must be
copied into the CWD before linking — the linker picks it up automatically
from the current directory without needing a special option.

DR Fortran-77 4.0 — compiled to a CP/M-86 binary (small model):
```
cp $(dirname $(which drfcpm_f77))/../share/f7786cpm/8087.sim .
drfcpm_f77 hellof.f77
drfcpm_link 'hellof.cmd=hellof [$LD]'
cmdinfo hellof.cmd
```

`8087.sim` must be present in the CWD before linking — the linker picks it
up automatically. In the examples `Makefile` an `8087.sim` target copies it
from `share/f7786cpm/` automatically as a dependency of `hellof.cmd`.

### PL/M-86 Programs
```
intel_asm86 scd.a86
intel_plm86 hellop.plm debug 'optimize(3)'
pcdev_linkcmd hellop=scd,hellop '[data[ori[0]]'
cmdinfo hellop.cmd
```

