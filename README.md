# CP/M-86/MS-DOS Cross Development Environment

This project regroups a simple method to create a cross development environment for CP/M-86. It happens that all DOS based tools can also be used for building DOS programs.
This project was developed for myself in a nerdy spirit and for a lot of fun. Just sharing without any guarantees.

## Key tools for CP/M-86 development
- aztec c compiler version 3.4
- rasm86/link86,lib86 DOS version from Digital Research 
- asm86.com  and gendef.com from Digital Research
- nasm Netwide assembler
- masm, link, asm, exe2bin, hex2bin from Microsoft (the version of masm in this repository has been patched to work with emu2 and other emulators, see https://github.com/tsupplis/pcdos11-hacking for details). asm.com and hex2bin.com have been rebuilt for modified sources at https://github.com/tsupplis/pcdos11-hacking .

- The Super Cool emu2 DOS emulator to run the DR tools on macOS and Linux (https://github.com/dmsc/emu2). This is an incredible way to bring dos command line development tools to a modern and up to date shell/make/whatever based dev environment. Another stunning emulator. Emu2 and PCE are an incredible pair.
- We also need to run some cp/m-80 programs, for that, the tinylpo emulator is used. It works very well with asm86.com and gencmd.com programs

## License Considerations

- Microsoft MS-DOS tools are under MITS license (https://github.com/microsoft/MS-DOS/blob/master/LICENSE.md)
The following tools are not included and downloaded by the fetch tool but require you to understand the conditions of usage:
- The Aztec C use coniditions is documented at (https://www.aztecmuseum.ca/intro.htm#intro)
- The DR tools usage is documented at (http://www.cpm.z80.de/license.html) and (http://www.cpm.z80.de/faq.html)
- emu2 and tnylpo are open source with their licenses described respectively at (https://github.com/dmsc/emu2/blob/master/LICENSE) and (https://gitlab.com/gbrein/tnylpo/-/blob/master/LICENSE)
- nasm license terms can be found at (https://www.nasm.us)
- I am not guaranteeing in any ways the components mentioned above. You are of course free to use these at your own risk if you accept the conditions of usage given above.

## Script Mapping

All the tools are wrapped in the bin directory for direct usage:

| script name   | program     | quick description                  |
|---------------|-------------|------------------------------------|
| cpm_asm86     | asm86.cmd   | DR assembler (CP/M-80 tool)        | 
| cpm_gencmd    | gencmd.cmd  | DR H86 converter (CP/M-80 tool)    |
| pcdev_rasm86  | rasm86.exe  | DR assembler (OBJ)                 |
| pcdev_linkcmd | linkcmd.exe | DR linker for CP/M-86              |
| pcdev_link86  | linkcmd.exe | "  "  "  "  "  "                   |
| pcdev_linkexe | linkexe.exe | DR linker for DOS                  |
| pcdev_lib86   | lib86.exe   | DR librarian                       |
| pcdev_masm    | masm.exe    | Microsoft assembler                |
| pcdev_link    | link.exe    | Microsoft linker                   |
| pcdev_exe2bin | exe2bin.exe | Microsoft EXE converter            |
| pcdev_hex2bin | hex2bin.com | Microsoft HEX converter            |
| pcdev_asm     | asm.com     | Microsoft/86-DOS assembler         |
| pcdev_cmdinfo | cmdinfo.com | CMD info tool                      |
| pcdev_bin2cmd | bin2cmd.com | CMD converter                      |
| aztec34_cc    | cc.exe      | Aztec C compiler                   |
| aztec34_sqz   | sqz.exe     | Aztec C object optimizer           |
| aztec34_link  | ln.exe      | Aztec C linker                     |
| aztec34_lib   | lb.exe      | Aztec C lib                        |
| aztec34_hex86 | hex86.exe   | Aztec C H86 generator              |
| cpm86         | cpm.exe     | CPM86 Emulator coming with Aztec C |

## Fetching the tools

The development environment can be assembled by using the following steps:
```
./fetch_tools
export PATH=`pwd`/bin
```
it pulls the following:
- aztec 3.4 c compiler  (https://www.aztecmuseum.ca/az8634b.zip)
- link86, lib86 and rasm86 (http://www.cpm.z80.de/download/tools86.zip)
- asm86 and gencmd (ttp://cpmarchives.classiccmp.org/cpm/Software/rlee)
- masm, link, asm, exe2bin, hex2bin (local copies from https://github.com/microsoft/MS-DOS)
- cmdtools (https://github.com/tsupplis/cpm86-cmdtools)
- nasm (https://www.nasm.us/pub/nasm/releasebuilds/2.15.05/nasm-2.15.05.tar.gz)
- emu2 (https://github.com/dmsc/emu2)
- tnylpo (https://gitlab.com/gbrein/tnylpo.git)

Clearing the directory is achieved by:
```
./clear_tools
```

## Testing

The cpm86 tool is a cpm86 emulator for dos pulled with Aztec C. It works-ish. I do not rely on it for build tools but it can be experimented with. 
DOS/CP/M-80 emulations and DOS/CP/M-80 cross compilations are far better and ore options. In fine, proper emulation and CP/M-86 is the best option for testing. 
PCE is a very good choice (http://www.hampa.ch/pce/).

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

### C Programs

```
aztec34_cc helloc.c
aztec34_sqz helloc.o
aztec34_link -o helloc.cmd helloc.o -lc86
pcdev_cmdinfo helloc.cmd
```

### Assembler Programs with rasm86
```
pcdev_rasm86 helloa.a86 '$' pz sz
pcdev_linkcmd helloa '[$sz]'
pcdev_cmdinfo helloa.cmd
```

### Assembler Programs with asm86
```
cpm_asm86 hellob.a86
cpm_gencmd hellob.h86
pcdev_cmdinfo hellob.cmd
```

### Assembler Programs with masm 
```
pcdev_masm hellod \;
pcdev_link hellod \;
pcdev_exe2bin hellod.exe
pcdev_bin2cmd hellod.bin hellod.cmd
```

### Assembler Programs with nasm 
```
nasm hellon.asm -fbin -o hellon.bin 
pcdev_bin2cmd hellon.bin hellon.cmd
```
You can build a native unix/dos version of bin2cmd/cmdinfo from (https://github.com/tsupplis/cpm86-cmdtools)

