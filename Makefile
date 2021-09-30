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

TOOLS=helloa.cmd helloc.cmd

all: binaries

binaries: $(TOOLS) 

helloc.cmd: helloc.o
	$(LD) -o $@ $^ $(LDFLAGS)
	ls -l $@
	pcdev_cmdinfo $@

%.cmd: %.obj
	$(LINK86) $* '[$$sz]'
	ls -l $@
	pcdev_cmdinfo $@

%.obj: %.a86
	$(RASM86) $< $$ pz sz

%.o: %.c
	$(CC) $(CFLAGS) $<
	$(STRIP) $@

clean:
	$(RM) *.o *.h86 *.log *.sym *.prn *.lst *.obj $(TOOLS) 

