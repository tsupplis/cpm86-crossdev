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
MASM=pcdev_masm
LINK=pcdev_link

TOOLS=helloa.cmd hellob.cmd helloc.cmd hellod.cmd

all: binaries

binaries: $(TOOLS) 

hellod.cmd: hellod.exe

hellod.exe: hellod.obj
	$(LINK) hellod \;

hellod.obj: hellod.asm
	$(MASM) hellod \;

hellob.cmd: hellob.h86
	$(GENCMD) $^
	ls -l $@
	pcdev_cmdinfo $@

helloc.cmd: helloc.o
	$(LD) -o $@ $^ $(LDFLAGS)
	ls -l $@
	pcdev_cmdinfo $@

%.cmd: %.obj
	$(LINK86) $* '[$$sz]'
	ls -l $@
	pcdev_cmdinfo $@

%.h86: %.a86
	$(ASM86) $<

%.cmd: %.bin
	$(BIN2CMD) $< $@

%.bin: %.exe
	$(EXE2BIN) $< $@

%.obj: %.a86
	$(RASM86) $< $$ pz sz

%.o: %.c
	$(CC) $(CFLAGS) $<
	$(STRIP) $@

clean:
	$(RM) *.o *.h86 *.log *.sym *.prn *.lst *.obj *.bin *exe $(TOOLS) 

