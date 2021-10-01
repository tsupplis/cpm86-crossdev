AS=aztec34_as
CC=aztec34_cc
AR=aztec34_lib
CFLAGS=-I. +F -B +0 -D__CPM86__ 
STRIP=aztec34_sqz
#LDFLAGS=-lm -lc86
LDFLAGS=-lc86
LD=aztec34_link
LINK86=pcdev_linkcmd
RASM86=pcdev_rasm86
ASM86=cpm_asm86
GENCMD=cpm_gencmd
BIN2CMD=pcdev_bin2cmd
EXE2BIN=pcdev_exe2bin
CMDINFO=pcdev_cmdinfo
NASM=nasm
MASM=pcdev_masm
LINK=pcdev_link

TOOLS=helloa.cmd hellob.cmd helloc.cmd hellod.cmd hellon.cmd

all: binaries

binaries: $(TOOLS) 

hellon.cmd: hellon.bin
	$(BIN2CMD) $< $@
	$(CMDINFO) $@

hellod.cmd: hellod.exe
	$(EXE2BIN) $< hellod.bin
	$(BIN2CMD) hellod.bin $@
	$(CMDINFO) $@

hellod.exe: hellod.obj
	$(LINK) hellod \;

hellon.bin: hellon.asm
	$(NASM) $< -fbin -o $@

hellod.obj: hellod.asm
	$(MASM) hellod \;

hellob.cmd: hellob.h86
	$(GENCMD) $^
	ls -l $@
	$(CMDINFO) $@

helloc.cmd: helloc.o
	$(LD) -o $@ $^ $(LDFLAGS)
	ls -l $@
	$(CMDINFO) $@

%.cmd: %.obj
	$(LINK86) $* '[$$sz]'
	ls -l $@
	$(CMDINFO) $@

%.h86: %.a86
	$(ASM86) $<

%.obj: %.a86
	$(RASM86) $< $$ pz sz

%.o: %.c
	$(CC) $(CFLAGS) $<
	$(STRIP) $@

clean:
	$(RM) *.o *.h86 *.log *.sym *.prn *.lst *.obj *.bin *exe $(TOOLS) 

