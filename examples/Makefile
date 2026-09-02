AS=aztec34_as
CC=aztec34_cc
AR=aztec34_lib
CFLAGS=-I. +F -B +0 -D__CPM86__
STRIP=aztec34_sqz
LDFLAGS=-lc86
LD=aztec34_link
ACC=aztec42_cc
ALD=aztec42_link
LINK86=pcdev_linkcmd
RASM86=pcdev_rasm86
ASM86=cpm_asm86
GENCMD=cpm_gencmd
BIN2CMD=bin2cmd
EXE2BIN=pcdev_exe2bin
CMDINFO=cmdinfo
CBLINK=drcbcpm_link
BC=drcbcpm_bc
NASM=nasm
MASM=pcdev_masm
LINK=pcdev_link
PLM86=intel_plm86
IASM86=intel_asm86
MBASIC=cpm86_mbasic
M2CB=m2cb_cvt
XLT86=cpm_xlt86
EMU2=emu2
DRCCC=drccpm_cc
F77=drfcpm_f77
F77SHARE=$(shell dirname $(shell which drfcpm_f77))/../share/f7786cpm
PMT=drpmt_pc
PMTLINK=drpmt_link
XLISP=cpm86_xlisp
TOOLS=helloa.cmd hellob.cmd helloc.cmd hellod.cmd hellon.cmd hellor.cmd \
	helloans.cmd hellop.cmd hellodrc.cmd hellof.cmd hellomt.cmd hellocvt.cmd \
	hello80.cmd

all: binaries

binaries: $(TOOLS) 

hellon.cmd: hellon.bin
	$(BIN2CMD) $< $@
	$(CMDINFO) $@

hellod.cmd: hellod.exe
	$(EXE2BIN) $< hellod.bin
	$(BIN2CMD) hellod.bin $@
	$(CMDINFO) $@

hellor.cmd: hellor.cb
	$(BC) hellor.cb
	$(CBLINK) hellor 

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

helloans.cmd: helloans.c
	$(ACC) $(CFLAGS) $<
	$(ALD) -o $@ helloans.o $(LDFLAGS)
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

8087.sim:
	cp $(F77SHARE)/8087.sim .

hellof.cmd: hellof.f77 8087.sim
	$(F77) hellof.f77
	drfcpm_link 'hellof.cmd=hellof[$$LD]'
	ls -l $@
	$(CMDINFO) $@

hellodrc.obj: hellodrc.c
	$(DRCCC) -o$@ hellodrc.c

hellodrc.cmd: hellodrc.obj
	drccpm_link 'hellodrc.cmd=hellodrc.obj [$$LD]'
	ls -l $@
	$(CMDINFO) $@

hellocvt.cmd: hellocvt.cb
	$(BC) hellocvt.cb
	$(CBLINK) hellocvt

hellocvt.cb: hellomsb.bas
	$(M2CB) hellomsb.bas
	mv hellomsb.cb hellocvt.cb

hellomt.r86: hellomt.pas
	$(PMT) hellomt

hellomt.cmd: hellomt.r86
	$(PMTLINK) hellomt.cmd=hellomt,p:paslib/s
	ls -l $@
	$(CMDINFO) $@

scd.obj: scd.a86
	$(IASM86) scd.a86

hellop.obj: hellop.plm
	$(PLM86) hellop.plm debug 'optimize(3)'

hellop.cmd: hellop.obj scd.obj
	$(LINK86) hellop=scd,hellop '[data[ori[0]]'
	$(CMDINFO) $@

hello80.a86: hello80.asm
	$(XLT86) hello80 < /dev/null
	[ -f hello80.a86 ]

hello80.cmd: hello80.h86
	$(GENCMD) $<
	ls -l $@
	$(CMDINFO) $@

test: binaries
	@for cmd in $(TOOLS); do \
	    echo "test: $$cmd"; \
	    EMU2_DRIVE_C=. $(EMU2) ./$$cmd; \
	done
	@echo "test: hellomsb.bas"
	$(MBASIC) hellomsb.bas
	@echo "test: hello.lsp"
	$(XLISP) hello.lsp
	
clean:
	$(RM) *.prt *.prn *.o *.h86 *.log *.sym *.prn *.lst *.obj *.r86 *.bin *.lin \
		  *.mp2 *exe \
	      ctemp.tok pastemp.tok hellod.a 8087.sim hellocvt.cb hellomsb.cb \
	      hello80.a86 \
	      $(TOOLS)

