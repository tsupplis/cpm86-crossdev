# CP/M-86 Cross Development Environment

## Key tools for CP/M-86 development
- aztec c compiler version 3.4
- rasm86/linkcmd, DOS version from Digital Research (http://www.cpm.z80.de/binary.html)

```
LINK86 Linkage Editor  02/Feb/87      Version 2.02
Cross Linker: DOS -> CPM-86 & CDOS-286      1/2/86
```
```
RASM-86 Assembler   12-Mar-87  PC-DOS Version 1.4a
```

- The Super Cool emu2 DOS emulator to run the DR tools on macOS and Linux (https://github.com/dmsc/emu2). This is an incredible way to bring dos command line development tools to a modern and up to date shell/make/whatever based dev environment. Another stunning emulator. Emu2 and PCE are an incredible pair.

## Fetching the tools
The development environment can be assembled by using the following steps:
```
./fetch
export PATH=`pwd`/bin
```
it pulls the following:
- aztec 3.4 c compiler  (https://www.aztecmuseum.ca/az8634b.zip)
- linkcmd and rasm86 (http://www.cpm.z80.de/download/tools86.zip)
- emu2 (https://github.com/dmsc/emu2)

