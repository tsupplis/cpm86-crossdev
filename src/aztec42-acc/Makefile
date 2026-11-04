CC = cc
CFLAGS = -O2 -std=c89 -pedantic -Wall -Wextra -Werror
BIN = ../../bin
AZCC = $(BIN)/aztec42_cc
AZLD = $(BIN)/aztec42_link
EMU = $(BIN)/emu2
AZCFLAGS = +0 -b -n -DACC_DOS
SOURCES = acc.c args.c plan.c exec.c
OBJECTS = acc.o args.o plan.o exec.o

.DELETE_ON_ERROR:
.PHONY: all dos check check-unix check-dos check-exec clean

all: acc

acc: $(SOURCES) acc.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $(SOURCES) $(LDLIBS)

dos: acc.exe

$(OBJECTS): acc.h

.c.o:
	$(AZCC) $(AZCFLAGS) -o $@ $<

acc.exe: $(OBJECTS)
	$(AZLD) -o $@ $(OBJECTS) -lc

check: check-unix check-dos check-exec

check-unix: acc
	sh check.sh "$(abspath ../../share/aztec42)" "$(CURDIR)/acc"

check-dos: acc.exe
	EMU2_DRIVE_D="$(abspath ../../share/aztec42)" sh check.sh 'D:\' "$(abspath $(EMU))" "$(CURDIR)/acc.exe"

check-exec: acc
	sh exec-check.sh "$(abspath ../../share/aztec42)" "$(CURDIR)/acc" "$(abspath $(EMU))"

clean:
	rm -f acc acc.exe $(OBJECTS) acc.dbg acc.sym sbegin.o
