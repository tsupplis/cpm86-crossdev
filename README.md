# CP/M-86 Cross Development Environment

## Key tools for CP/M-86 development
- aztec c compiler version 3.4
- rasm86/link86,lib86 DOS version from Digital Research 
- masm, link, asm, exe2bin, hex2bin from Microsoft

- The Super Cool emu2 DOS emulator to run the DR tools on macOS and Linux (https://github.com/dmsc/emu2). This is an incredible way to bring dos command line development tools to a modern and up to date shell/make/whatever based dev environment. Another stunning emulator. Emu2 and PCE are an incredible pair.

## Fetching the tools
The development environment can be assembled by using the following steps:
```
./fetch_tools
export PATH=`pwd`/bin
```
it pulls the following:
- aztec 3.4 c compiler  (https://www.aztecmuseum.ca/az8634b.zip)
- link86, lib86 and rasm86 (http://www.cpm.z80.de/download/tools86.zip)
- masm, link, asm, exe2bin, hex2bin (local copies from https://github.com/microsoft/MS-DOS)
- cmdtools (https://github.com/tsupplis/cpm86-cmdtools)
- emu2 (https://github.com/dmsc/emu2)

