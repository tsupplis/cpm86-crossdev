AS=aztec34_as
CC=aztec34_cc
AR=aztec34_lib
CFLAGS=-I. +F +0 -D__CPM86__
DCFLAGS=-I. +F +0 -D__DOS__
STRIP=aztec34_sqz
LDFLAGS=-lc86
DLDFLAGS=-ld11
MLDFLAGS=-lm -lc86
DMLDFLAGS=-lm -ld11
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
NASM=nasm -O0
MASM=pcdev_masm
LINK=pcdev_link
PLM86=intel_plm86
IASM86=intel_asm86
MBASIC=cpm86_mbasic
BASIC=cpm86_basic
M2CB=m2cb_cvt
XLT86=cpm_xlt86
EMU2=emu2
TNYLPO=tnylpo
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

ASM86_CMDS=hellob.cmd hello801.cmd hello86.cmd
RASM86_CMDS=helloa.cmd helloab.cmd hello802.cmd
ASM_CMDS=hellomas.cmd hellonas.cmd hello803.cmd hello804.cmd
AZTEC_CMDS=helloazc.cmd helloaza.cmd helloaz7.cmd upxaz7.cmd hello802.cmd mathazc.cmd \
    helloazd.exe mathazd.exe upxazd.exe ovraza.cmd ovrfunc.ovr
DRC_CMDS=hellodrc.cmd hellodra.cmd hellodr7.cmd upxdrc.cmd upxdra.cmd upxdr7.cmd mathdrc.cmd
BASIC_CMDS=hellocbs.cmd hellocvt.cmd hellocba.int
MSC_CMDS=hellomsc.exe exepkmsc.exe lzexemsc.exe upxmsc.exe
PLM_CMDS=helloplm.cmd
PLI_CMDS=hellopli.cmd upxpli.cmd
COBOL_CMDS=hellocbl.int
F77_CMDS=hellof77.cmd hellopmt.cmd upxpli.cmd
PASCAL_CMDS=hellopmt.cmd

8086_CMDS=$(RASM86_CMDS) $(ASM86_CMDS) $(ASM_CMDS) \
    $(AZTEC_CMDS) $(DRC_CMDS) $(MSC_CMDS) $(BASIC_CMDS) \
    $(PLM_CMDS) $(PLI_CMDS) $(COBOL_CMDS) $(PASCAL_CMDS) $(F77_CMDS)

8080_CMDS:=hello80.com

all: binaries

binaries: $(8086_CMDS) $(8080_CMDS)

ovraza.cmd ovraza.rsm: ovrmain.o
	rm -f ovraza.cmd ovraza.rsm
	$(ALD) +c 1020 +d aOO  -o ovraza.cmd -r ovrmain.o ovld.o $(LDFLAGS)
	$(CMDINFO) ovraza.cmd

ovrmain.o: ovrmain.c
	$(ACC) $(CFLAGS) $<

ovrfunc.ovr: ovrfunc.o ovraza.rsm
	$(ALD) -o $@ ovrfunc.o ovraza.rsm ovbgn.o 

ovrfunc.o: ovrfunc.c 
	$(ACC) $(CFLAGS) $<

hellocba.int: hellocba.bas
	$(CBAS86) $<

upxaz7.cmd: helloaz7.cmd
	cp $< $@
	$(UPX) $@

	
upxazd.exe: helloazd.exe
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

helloab.cmd: helloab.obj
	$(LINK86) $@=$(basename $<) '[cod[ori[0]],dat[ori[0],max[0]]]'
	$(CMDINFO) $@

hello801.cmd: hello801.h86
	$(GENCMD) $< 8080
	$(CMDINFO) $@
  
hello802.cmd: hello802.obj
	$(LINK86) $@=$(basename $<) '[cod[ori[0]],dat[ori[0],max[0]]]'
	$(CMDINFO) $@

hello803.cmd: hello803.exe
	$(EXE2BIN) $< hello803.bin
	$(BIN2CMD) hello803.bin $@
	$(CMDINFO) $@

hello803.exe: hello803.obj
	$(LINK) $< \;

hello803.obj: hello803.asm
	$(MASM) $(basename $@),$(basename $@)\;
	[ -f $@ ]

hello804.cmd: hello804.bin
	$(BIN2CMD) $< $@
	$(CMDINFO) $@

hello804.bin: hello804.asm
	$(NASM) $< -fbin -o $@

hellocbs.cmd: hellocbs.obj
	$(CBLINK) $@=$(basename $<)
	$(CMDINFO) $@

hellomas.cmd: hellomas.exe
	$(EXE2BIN) $< hellomas.bin
	$(BIN2CMD) hellomas.bin $@
	$(CMDINFO) $@

hellomas.exe: hellomas.obj
	$(LINK) $< \;

hellomas.obj: hellomas.asm
	$(MASM) $(basename $@),$(basename $@)\;
	[ -f $@ ]

hellonas.cmd: hellonas.bin
	$(BIN2CMD) $< $@
	$(CMDINFO) $@

hellonas.bin: hellonas.asm
	$(NASM) $< -fbin -o $@

hellob.cmd: hellob.h86
	$(GENCMD) $< 8080
	$(CMDINFO) $@
  
helloazd.exe: helloaza.o
	$(ALD) -o $@ $< $(DLDFLAGS)

helloaza.cmd: helloaza.o
	$(ALD) -o $@ $< $(LDFLAGS)
	$(CMDINFO) $@

helloaza.o: helloaza.c
	$(ACC) $(CFLAGS) $<

mathazd.exe: mathazc.o
	$(ALD) -o $@ $< $(DMLDFLAGS)

mathazc.cmd: mathazc.o
	$(ALD) -o $@ $< $(MLDFLAGS)
	$(CMDINFO) $@

mathazc.o: mathazc.c
	$(ACC) $(CFLAGS) $<

helloaz7.cmd: helloaz7.o
	$(ALD) -o $@ $< $(MLDFLAGS)
	$(CMDINFO) $@

helloaz7.o: helloaz7.c
	$(ACC) $(CFLAGS) $<

helloazc.cmd: helloazc.o
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

mathdrc.cmd: mathdrc.obj
	drccpm_link '$@=$(basename $<)'
	$(CMDINFO) $@

mathdrc.obj: mathdrc.c
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

helloplm.cmd: helloplm.obj scd.obj
	$(LINK86) $@=scd,helloplm '[data[ori[0]]'
	$(CMDINFO) $@

helloplm.obj: helloplm.plm
	$(PLM86) $< debug 'optimize(3)'
	[ -f $@ ]

scd.obj: scd.a86
	$(IASM86) $<

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

hellocbl.int: hellocbl.cbl
	$(COBOL) hellocbl.cbl
	[ -f $@ ]

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

test: 
	$(MAKE) test-8086
	$(MAKE) test-8080
	$(MAKE) test-lbr
	$(MAKE) test-misc
	$(MAKE) test-negative

test-lbr:
	sh test_lbr

test-misc:
	sh test_misc

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

test-asm86: $(ASM86_CMDS)
	sh test_emu2 $(ASM86_CMDS)

test-rasm86: $(RASM86_CMDS)
	sh test_emu2 $(RASM86_CMDS)

test-msc: $(MSC_CMDS)
	sh test_emu2 $(MSC_CMDS)

test-pli: $(PLI_CMDS)
	sh test_emu2 $(PLI_CMDS)

test-plm: $(PLM_CMDS)
	sh test_emu2 $(PLM_CMDS)

test-basic: $(BASIC_CMDS)
	sh test_emu2 $(BASIC_CMDS)

test-drc: $(DRC_CMDS)
	sh test_emu2 $(DRC_CMDS)

test-pascal: $(PASCAL_CMDS)
	sh test_emu2 $(PASCAL_CMDS)

test-aztec: $(AZTEC_CMDS)
	sh test_emu2 $(AZTEC_CMDS)

test-asm: $(ASM_CMDS)
	sh test_emu2 $(ASM_CMDS)

test-8086: $(8086_CMDS)
	sh test_emu2 $(8086_CMDS)

test-8080: $(8080_CMDS) 
	sh test_tnylpo $(8080_CMDS)

clean:
	$(RM) *.ovr *.rsm .prt *.prn *.o *.log *.sym *.prn *.lst *.obj *.r86 *.bin *.lin \
		  *.mp2 *exe *.map *.err *.int *.prn *.hex *.out *.raw *.com \
	      ctemp.tok pastemp.tok hellomas.a 8087.sim hellocvt.cb hellomsb.cb \
	      hello86.a86 \
	      badasm86.cmd badgencm.cmd \
	      $(8086_CMDS) $(8080_CMDS)
	@for f in *.h86; do \
		case "$$f" in badgencm.h86) ;; *) rm -f "$$f" ;; esac; \
	done

