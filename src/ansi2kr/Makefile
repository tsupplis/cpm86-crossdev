CMD_CC=aztec42_cc
CFLAGS=-I. -B +0 -D__CPM86__ 
STRIP=aztec42_sqz
LDFLAGS=-lc86
CMD_LD=aztec42_link

all: ansi2kr.cmd ansi2kr

ansi2kr: ansi2krn.o
	$(CC) -O2 -o $@ $^

ansi2krn.o: ansi2kr.c
	$(CC) -O2 -c -o $@ $<

ansi2kr.cmd: ansi2kr.o
	$(CMD_LD) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CMD_CC) $(CFLAGS) $<
	$(STRIP) $@

clean:
	$(RM) *.o *.h86 *.log *.sym *.prn *.lst *.obj ansi2kr ansi2kr.cmd

