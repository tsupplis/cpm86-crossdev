CC = cc
CFLAGS = -O2 -std=c89 -pedantic -Wall -Wextra -Werror
BIN = ../../bin
AZCC = $(BIN)/aztec42_cc
AZLD = $(BIN)/aztec42_link
EMU = $(BIN)/emu2
AZCFLAGS = +0 -b -n

.DELETE_ON_ERROR:
.PHONY: all dos check check-unix check-dos clean

all: acc

acc: acc.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ acc.c $(LDLIBS)

dos: acc.exe

acc.o: acc.c
	$(AZCC) $(AZCFLAGS) -o $@ acc.c

acc.exe: acc.o
	$(AZLD) -o $@ acc.o -lc

check: check-unix check-dos

check-unix: acc
	sh check.sh ./acc

check-dos: acc.exe
	sh check.sh $(EMU) acc.exe

clean:
	rm -f acc acc.exe acc.o acc.dbg acc.sym sbegin.o
