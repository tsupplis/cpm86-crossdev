AS=aztec42_as
CC=aztec42_cc
AR=aztec42_lib
CFLAGS=-I. +F -B +0 -DAZTEC
STRIP=aztec42_sqz
LDFLAGS=-lm -lc86
LD=aztec42_link
LINK86=pcdev_linkcmd
RASM86=pcdev_rasm86

all: xlisp.cmd

xlisp.cmd: xlisp.o xllist.o xlmath.o xlstr.o xlread.o xlsubr.o xlprin.o \
    xlbind.o xldmem.o xleval.o xlfio.o xlio.o xlkmap.o xlobj.o
	$(LD) -o $@ xlisp.o -F xlisp.lnk $(LDFLAGS)
    

%.o: %.c
	$(CC) $(CFLAGS) $<
	$(STRIP) $@

clean:
	$(RM) *.o *.h86 *.log *.sym *.prn *.lst *.obj xlisp.cmd


