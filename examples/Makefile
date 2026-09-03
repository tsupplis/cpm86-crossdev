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
ASM86=cpm86_asm86
GENCMD=cpm86_gencmd
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
BASIC=cpm86_basic
M2CB=m2cb_cvt
XLT86=cpm_xlt86
EMU2=emu2
CPM86=cpm86
DRCCC=drccpm_cc
F77=drfcpm_f77
PMT=drpmt_pc
PMTLINK=drpmt_link
XLISP=cpm86_xlisp
PLI=drpli_pc
PLILINK=drpli_link
TOOLS=helloa.cmd helloab.cmd hellob.cmd helloc.cmd hellod.cmd hellon.cmd hellor.cmd \
	helloans.cmd hellop.cmd hellodrc.cmd hellof77.cmd hellopmt.cmd hellocvt.cmd \
	hello80.cmd # hellopli.cmd

all: binaries

binaries: $(TOOLS) 

hellon.cmd: hellon.bin
	$(BIN2CMD) $< $@
	ls -l $@
	$(CMDINFO) $@

hellod.cmd: hellod.exe
	$(EXE2BIN) $< hellod.bin
	$(BIN2CMD) hellod.bin $@
	ls -l $@
	$(CMDINFO) $@

hellor.cmd: hellor.cb
	$(BC) hellor.cb
	$(CBLINK) hellor 
	ls -l $@
	$(CMDINFO) $@

hellod.exe: hellod.obj
	$(LINK) hellod \;

hellon.bin: hellon.asm
	$(NASM) $< -fbin -o $@

hellod.obj: hellod.asm
	$(MASM) hellod \;

hellob.cmd: hellob.h86
	$(GENCMD) $^ 8080
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

%.cmd: %.obj
	$(LINK86) $* '[$$sz]'
	ls -l $@
	$(CMDINFO) $@

%.h86: %.a86
	$(ASM86) $<
	ls -l $@

%.obj: %.a86
	$(RASM86) $< $$ pz sz
	ls -l $@

%.o: %.c
	$(CC) $(CFLAGS) $<
	$(STRIP) $@

hellof77.cmd: hellof77.f77
	$(F77) hellof77.f77
	drfcpm_link 'hellof77.cmd=hellof77'
	ls -l $@
	$(CMDINFO) $@

hellodrc.obj: hellodrc.c
	$(DRCCC) -o$@ hellodrc.c

hellodrc.cmd: hellodrc.obj
	drccpm_link 'hellodrc.cmd=hellodrc.obj'
	ls -l $@
	$(CMDINFO) $@

hellocvt.cmd: hellocvt.cb
	$(BC) hellocvt.cb
	$(CBLINK) hellocvt

hellocvt.cb: hellomsb.bas
	$(M2CB) hellomsb.bas
	mv hellomsb.cb hellocvt.cb

hellopmt.r86: hellopmt.pas
	$(PMT) hellopmt
	ls -l $@

hellopmt.cmd: hellopmt.r86
	$(PMTLINK) hellopmt.cmd=hellopmt,p:paslib/s
	ls -l $@
	$(CMDINFO) $@

scd.obj: scd.a86
	$(IASM86) scd.a86

hellop.obj: hellop.plm
	$(PLM86) hellop.plm debug 'optimize(3)'
	ls -l $@

hellop.cmd: hellop.obj scd.obj
	$(LINK86) hellop=scd,hellop '[data[ori[0]]'
	ls -l $@
	$(CMDINFO) $@

hellopli.obj: hellopli.pli
	$(PLI) hellopli
	ls -l $@

hellopli.cmd: hellopli.obj
	$(PLILINK) 'hellopli.cmd=hellopli'
	ls -l $@
	$(CMDINFO) $@

hello80.a86: hello80.asm
	$(XLT86) hello80 < /dev/null
	ls -l $@

hello80.cmd: hello80.h86
	$(GENCMD) $< 8080
	ls -l $@
	$(CMDINFO) $@

test: emu2-test cpm86-test
	@echo "--------- TEST: mbasic hellomsb.bas"
	$(MBASIC) hellomsb.bas
	@echo "--------- TEST: basic hellopba.bas"
	$(BASIC) < hellopba.bas
	@echo "--------- TEST: hello.lsp"
	$(XLISP) hello.lsp

emu2-test: binaries
	@for cmd in $(TOOLS); do \
	    echo "--------- CPM86 TEST: $$cmd"; \
	    EMU2_DRIVE_C=. $(CPM86) $$cmd || echo "--------- ERROR with $$cmd"; \
	done

cpm86-test: binaries
	@for cmd in $(TOOLS); do \
	    echo "--------- EMU2 TEST: $$cmd"; \
	    EMU2_DRIVE_C=. $(EMU2) ./$$cmd || echo "--------- ERROR with $$cmd"; \
	done

clean:
	$(RM) *.prt *.prn *.o *.h86 *.log *.sym *.prn *.lst *.obj *.r86 *.bin *.lin \
		  *.mp2 *exe *.map \
	      ctemp.tok pastemp.tok hellod.a 8087.sim hellocvt.cb hellomsb.cb \
	      hello80.a86 \
	      $(TOOLS)

