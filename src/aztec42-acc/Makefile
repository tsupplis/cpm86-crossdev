CC = cc
CFLAGS = -O2 -std=c89 -pedantic -Wall -Wextra -Werror
BIN = ../../bin
AZCC = $(BIN)/aztec42_cc
AZLD = $(BIN)/aztec42_link
EMU = $(BIN)/emu2
INSTALL_ROOT = ../../share/aztec42
INSTALL_BIN = $(INSTALL_ROOT)/bin
AZCFLAGS = +0 -b -n -DACC_DOS
SOURCES = acc.c args.c plan.c exec.c
OBJECTS = acc.o args.o plan.o exec.o

.DELETE_ON_ERROR:
.PHONY: all dos install check check-unix check-dos check-exec check-exec-dos clean

all: acc

acc: $(SOURCES) acc.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $(SOURCES) $(LDLIBS)

dos: acc.exe

install: $(INSTALL_BIN)/acc $(INSTALL_BIN)/acc.exe

$(INSTALL_BIN)/acc: acc
	mkdir -p $(INSTALL_BIN)
	cp -f acc $@
	chmod 755 $@

$(INSTALL_BIN)/acc.exe: acc.exe
	mkdir -p $(INSTALL_BIN)
	cp -f acc.exe $@
	chmod 644 $@

$(OBJECTS): acc.h

.c.o:
	$(AZCC) $(AZCFLAGS) -o $@ $<

acc.exe: $(OBJECTS)
	$(AZLD) -o $@ $(OBJECTS) -lc

check: check-unix check-dos check-exec check-exec-dos

check-unix: acc
	sh check.sh "$(abspath ../../share/aztec42)" "$(CURDIR)/acc"

check-dos: acc.exe
	EMU2_DRIVE_D="$(abspath ../../share/aztec42)" sh check.sh 'D:\' "$(abspath $(EMU))" "$(CURDIR)/acc.exe"

check-exec: acc
	sh exec-check.sh "$(abspath ../../share/aztec42)" "$(CURDIR)/acc" \
		"$(abspath $(EMU))" "$(abspath ../../examples)"

check-exec-dos: acc.exe
	sh exec-dos-check.sh "$(CURDIR)/acc.exe" "$(abspath $(EMU))" \
		"$(abspath ../../share/aztec42)" "$(abspath ../../examples)"

clean:
	rm -f acc acc.exe $(OBJECTS) acc.dbg acc.sym sbegin.o
