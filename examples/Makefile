AS=aztec34_as
CC=aztec34_cc
AR=aztec34_lib
CFLAGS=-I. +F -B +0 -D__CPM86__
STRIP=aztec34_sqz
LDFLAGS=-lc86
MLDFLAGS=-lm -lc86
LD=aztec34_link
ACC=aztec42_cc
ALD=aztec42_link
LINK86=pcdev_linkcmd
RASM86=pcdev_rasm86
ASM86=cpm86_asm86
ASM80=cpm_asm80
GENCMD=cpm86_gencmd
BIN2CMD=bin2cmd
HEXCOM=hexcom
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
CBAS86=cpm86_cbas
CRUN86=cpm86_crun
F77=drfcpm_fc
F77LINK=drfcpm_link
PMT=drpmtcpm_pc
PMTLINK=drpmtcpm_link
XLISP=cpm86_xlisp
PLI=drplicpm_pc
PLILINK=drplicpm_link
COBOL=mfcobol_cc
COBRUN=mfcobol_run
GWBASIC=pcdev_gwbasic
LZEXE=pcdev_lzexe
EXEPACK=pcdev_exepack
UPX=../bin/upx --8086
MSCCL=msc5_cl
MSCLINK=msc5_link
TOOLS=helloa.cmd helloab.cmd hellob.cmd helloc.cmd hellod.cmd hellon.cmd hellor.cmd \
    helloans.cmd helloaz7.cmd hellop.cmd hellodrc.cmd hellof77.cmd hellopmt.cmd \
    hellocvt.cmd hello86.cmd hellocbl.int helloi.int hellopli.cmd hellodra.cmd \
    hellodr7.cmd hellomsc.exe \
	lzexemsc.exe upxmsc.exe upxpli.cmd upxdra.cmd upxdrc.cmd upxdr7.cmd upxaz7.cmd \
    exepkmsc.exe

EXTRAS=hello80.com

all: binaries

binaries: $(TOOLS) $(EXTRAS)

helloi.int: helloi.bas
	$(CBAS86) $<

upxaz7.cmd: helloaz7.cmd
	cp $< $@
	$(UPX) $@

upxpli.cmd: hellopli.cmd
	cp $< $@
	$(UPX) $@

upxdra.cmd: hellodra.cmd
	cp $< $@
	$(UPX) $@

upxdrc.cmd: hellodrc.cmd
	cp $< $@
	$(UPX) $@

upxdr7.cmd: hellodr7.cmd
	cp $< $@
	$(UPX) $@

upxmsc.exe: hellomsc.exe
	cp $< $@
	$(UPX) $@

exepkmsc.exe: hellomsc.exe
	$(EXEPACK) $< $@

lzexemsc.exe: hellomsc.exe
	cp $< $@
	$(LZEXE) $@

hellomsc.exe: hellomsc.obj
	$(MSCLINK) $<,$@ \;

hellomsc.obj: hellomsc.c
	$(MSCCL) -c $@ $< 

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
	$(ALD) -o $@ $< $(LDFLAGS)
	$(CMDINFO) $@

helloans.o: helloans.c
	$(ACC) $(CFLAGS) $<

helloaz7.cmd: helloaz7.o
	$(ALD) -o $@ $< $(MLDFLAGS)
	$(CMDINFO) $@

helloaz7.o: helloaz7.c
	$(ACC) $(CFLAGS) $<

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

hellodr7.cmd: hellodr7.obj
	drccpm_link '$@=$(basename $<)'
	$(CMDINFO) $@

hellodr7.obj: hellodr7.c
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

hello80.com: hello80.hex
	$(HEXCOM) $<

hello80.hex: hello80.asm
	$(ASM80) $(basename $<)
	[ -f $@ ]

hello86.a86: hello80.asm
	$(XLT86) hello80 < /dev/null
	doscat hello80.a86 | sed -e 's/asm 8080/asm 8086/g' > hello86.a86
	rm -f hello80.a86
	[ -f $@ ]

hello86.cmd: hello86.h86
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
	$(MAKE) test-misc
	$(MAKE) test-negative

test-misc:
	env MBASIC="$(MBASIC)" BASIC="$(BASIC)" CRUN86="$(CRUN86)" \
	    XLISP="$(XLISP)" COBRUN="$(COBRUN)" GWBASIC="$(GWBASIC)" sh test_misc

test-negative:
	@echo "--------- NEG TEST: cpm86_asm86 badasm86.a86 (expect failure, badasm86.h86 removed)"
	@rm -f badasm86.h86 badasm86.lst badasm86.sym
	@if $(ASM86) badasm86.a86; then \
		echo "--------- ERROR: cpm86_asm86 badasm86.a86 unexpectedly succeeded"; exit 1; \
	fi
	@if [ -f badasm86.h86 ]; then \
		echo "--------- ERROR: badasm86.h86 was not removed after a failed assembly"; exit 1; \
	fi
	@echo "--------- NEG TEST: cpm86_gencmd badgencm.h86 (expect failure, badgencm.cmd removed)"
	@rm -f badgencm.cmd
	@if $(GENCMD) badgencm.h86; then \
		echo "--------- ERROR: cpm86_gencmd badgencm.h86 unexpectedly succeeded"; exit 1; \
	fi
	@if [ -f badgencm.cmd ]; then \
		echo "--------- ERROR: badgencm.cmd was not removed after a failed gencmd run"; exit 1; \
	fi

emu2-test: binaries
	env TOOLS="$(TOOLS)" EMU2="$(EMU2)" sh test_emu2

hellocbl.int: hellocbl.cbl
	$(COBOL) hellocbl.cbl
	[ -f $@ ]

clean:
	$(RM) *.prt *.prn *.o *.log *.sym *.prn *.lst *.obj *.r86 *.bin *.lin \
		  *.mp2 *exe *.map *.err *.int *.prn *.hex *.out *.raw \
	      ctemp.tok pastemp.tok hellod.a 8087.sim hellocvt.cb hellomsb.cb \
	      hello86.a86 \
	      badasm86.cmd badgencm.cmd \
	      $(TOOLS) $(EXTRAS)
	@for f in *.h86; do \
		case "$$f" in badgencm.h86) ;; *) rm -f "$$f" ;; esac; \
	done

