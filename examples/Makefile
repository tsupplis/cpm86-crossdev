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
CBBC=drcbcpm_bc
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
F77LINK=drfcpm_link
PMT=drpmt_pc
PMTLINK=drpmt_link
XLISP=cpm86_xlisp
PLI=drpli_pc
PLILINK=drpli_link
COBOL=mfcobol_cc
COBRUN=mfcobol_run
TOOLS=helloa.cmd helloab.cmd hellob.cmd helloc.cmd hellod.cmd hellon.cmd hellor.cmd \
	helloans.cmd hellop.cmd hellodrc.cmd hellof77.cmd hellopmt.cmd hellocvt.cmd \
	hello80.cmd hellocbl.int hellopli.cmd hellodra.cmd

all: binaries

binaries: $(TOOLS) 

hellon.cmd: hellon.bin
	$(BIN2CMD) $< $@
	$(CMDINFO) $@

hellod.cmd: hellod.exe
	$(EXE2BIN) $< hellod.bin
	$(BIN2CMD) hellod.bin $@
	$(CMDINFO) $@

helloab.cmd: helloab.obj
	$(LINK86) $@=$(basename $<) '[cod[ori[0]],dat[ori[0],max[0]]]'
	$(CMDINFO) $@

hellor.cmd: hellor.obj
	$(CBLINK) $@=$(basename $<)
	$(CMDINFO) $@

hellod.exe: hellod.obj
	$(LINK) $< \;

hellon.bin: hellon.asm
	$(NASM) $< -fbin -o $@

hellod.obj: hellod.asm
	$(MASM) $(basename $@),$(basename $@)\;
	[ -f $@ ]

hellob.cmd: hellob.h86
	$(GENCMD) $< 8080
	$(CMDINFO) $@
  
helloans.cmd: helloans.o
	$(LD) -o $@ $< $(LDFLAGS)
	$(CMDINFO) $@

helloans.o: helloans.c
	$(ACC) $(CFLAGS) $<
	$(CMDINFO) $@

helloc.cmd: helloc.o
	$(LD) -o $@ $< $(LDFLAGS)
	$(CMDINFO) $@

hellof77.cmd: hellof77.obj
	$(F77LINK) '$@=$<'
	$(CMDINFO) $@

hellodrc.cmd: hellodrc.obj
	drccpm_link '$@=$<'
	$(CMDINFO) $@

hellodrc.obj: hellodrc.c
	$(DRCCC) -v4 -o$@ -l$(@:.obj=.lst) hellodrc.c

hellodra.cmd: hellodra.obj
	drccpm_link '$@=$(basename $<)'
	$(CMDINFO) $@

hellodra.obj: hellodra.c
	$(DRCCC) -v4 -e -o$@ -l$(@:.obj=.lst) $<

hellocvt.cmd: hellocvt.obj
	$(CBLINK) $@=$(basename $<)

hellocvt.cb: hellomsb.bas
	$(M2CB) $<
	mv $(<:.bas=.cb) $@

hellopmt.r86: hellopmt.pas
	$(PMT) $<
	[ -f $@ ]

hellopmt.cmd: hellopmt.r86
	$(PMTLINK) $@=$(basename $<),p:paslib/s
	$(CMDINFO) $@

scd.obj: scd.a86
	$(IASM86) $<

hellop.obj: hellop.plm
	$(PLM86) $< debug 'optimize(3)'
	[ -f $@ ]

hellop.cmd: hellop.obj scd.obj
	$(LINK86) $@=scd,hellop '[data[ori[0]]'
	$(CMDINFO) $@

hellopli.cmd: hellopli.obj
	$(PLILINK) '$@=$(basename $<)'
	$(CMDINFO) $@

hello80.a86: hello80.asm
	$(XLT86) hello80 < /dev/null
	[ -f $@ ]

hello80.cmd: hello80.h86
	$(GENCMD) $< 8080
	$(CMDINFO) $@

%.cmd: %.obj
	$(LINK86) $@=$(basename $<) '[$$sz]'
	$(CMDINFO) $@

%.obj: %.cb
	$(CBBC) $<
	[ -f $@ ]

%.h86: %.a86
	$(ASM86) $<
	[ -f $@ ]

%.obj: %.a86
	$(RASM86) $< $$ pz sz
	[ -f $@ ]

%.o: %.c
	$(CC) $(CFLAGS) $<
	$(STRIP) $@

%.obj: %.f77
	$(F77) $<
	[ -f $@ ]

%.obj: %.pli
	$(PLI) $<
	[ -f $@ ]

test: emu2-test
	@echo "--------- TEST: mbasic hellomsb.bas"
	$(MBASIC) hellomsb.bas
	@echo "--------- TEST: basic hellopba.bas"
	$(BASIC) < hellopba.bas
	@echo "--------- TEST: hello.lsp"
	$(XLISP) hello.lsp
	@echo "--------- TEST: cobol hellocbl.cbl"
	$(COBRUN) hellocbl.int

emu2-test: binaries
	@for cmd in $(TOOLS); do \
            if [ "$$cmd" = "hellocbl.int" ]; then continue; fi; \
	    echo "--------- EMU2 TEST: $$cmd"; \
	    env EMU2_DRIVE_C=. $(EMU2) ./$$cmd || { echo "--------- ERROR with $$cmd"; exit 1; } \
	done

hellocbl.int: hellocbl.cbl
	$(COBOL) hellocbl.cbl
	[ -f $@ ]

clean:
	$(RM) *.prt *.prn *.o *.h86 *.log *.sym *.prn *.lst *.obj *.r86 *.bin *.lin \
		  *.mp2 *exe *.map *.err *.int \
	      ctemp.tok pastemp.tok hellod.a 8087.sim hellocvt.cb hellomsb.cb \
	      hello80.a86 \
	      $(TOOLS)

