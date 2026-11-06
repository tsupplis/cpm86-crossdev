# acc build and test driver: every build and test goes through these targets.
#
#   make                 both builds: acc (Unix) and acc.exe (DOS)
#   make unix            Unix acc   (objects in obj/unix, strict C89)
#   make dos             DOS acc.exe (objects in obj/dos), compiled and
#                        linked by the Unix acc itself (self-hosting)
#   make check           every suite on both builds
#   make check-coverage  options, values, raw options, diagnostics all tested
#   make check-plan      golden -### plans, Unix build (tests/plan/*.cases)
#   make check-plan-dos  the same cases, DOS build under emu2
#   make bless           rewrite golden expectations from both builds
#   make install         copy acc and acc.exe into the Aztec tool root
#   make clean           remove binaries and objects
#   SHFLAGS=-x           trace the shell test scripts, e.g. make check-exec SHFLAGS=-x

# Unix toolchain.
CC = cc
CFLAGS = -O2 -std=c89 -pedantic -Wall -Wextra -Werror

# DOS build: Aztec C 4.2 driven by the Unix acc built above.  acc runs the
# Aztec tools under emu2 and moves long linker commands into a response
# file, which the DOS 126-byte command tail requires for this many objects.
BIN = ../../bin
EMU = $(abspath $(BIN)/emu2)
ROOT = $(abspath ../../share/aztec42)
ACC_DOS_BUILD = ./acc -toolroot=$(ROOT) -emu=$(EMU) -target=dos2
DOS_CFLAGS = -DACC_DOS

EXAMPLES = $(abspath ../../examples)
# The fetcher installs with INSTALL_ROOT=<tool root>.
INSTALL_ROOT = $(ROOT)
INSTALL_BIN = $(INSTALL_ROOT)/bin
INSTALL_INCLUDE = $(INSTALL_ROOT)/include

# Shared by both builds; each build adds its own executor.
COMMON = acc.c words.c args.c profile.c raw.c path.c plan.c print.c
HEADERS = acc.h request.h profile.h raw.h path.h plan.h
UNIX_OBJECTS = $(COMMON:%.c=obj/unix/%.o) obj/unix/execunix.o
DOS_OBJECTS = $(COMMON:%.c=obj/dos/%.o) obj/dos/execdos.o

.DELETE_ON_ERROR:
.PHONY: all unix dos install check check-coverage check-plan check-plan-dos check-unix \
	check-dos check-exec check-exec-dos bless clean

all: unix dos

unix: acc

dos: acc.exe

# --- Unix build ---------------------------------------------------------

obj/unix/%.o: %.c $(HEADERS)
	@mkdir -p obj/unix
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

acc: $(UNIX_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(UNIX_OBJECTS) $(LDLIBS)

# --- DOS build ----------------------------------------------------------

# Aztec cc has no -Werror: any WARNING in its output fails the object.
obj/dos/%.o: %.c $(HEADERS) acc
	@mkdir -p obj/dos
	@echo "$(ACC_DOS_BUILD) $(DOS_CFLAGS) -c -o $@ $<"
	@$(ACC_DOS_BUILD) $(DOS_CFLAGS) -c -o $@ $< > $@.log 2>&1 || \
		{ cat $@.log; rm -f $@ $@.log; exit 1; }
	@cat $@.log; \
	if grep -q 'WARNING' $@.log; then \
		rm -f $@ $@.log; echo "$<: Aztec warnings are errors" >&2; exit 1; \
	fi; \
	rm -f $@.log

acc.exe: $(DOS_OBJECTS) acc
	$(ACC_DOS_BUILD) -o $@ $(DOS_OBJECTS)

# --- Installation -------------------------------------------------------

install: $(INSTALL_BIN)/acc $(INSTALL_BIN)/acc.exe $(INSTALL_INCLUDE)/azdefs.h

$(INSTALL_INCLUDE)/azdefs.h: include/azdefs.h
	mkdir -p $(INSTALL_INCLUDE)
	cp -f include/azdefs.h $@
	chmod 644 $@

$(INSTALL_BIN)/acc: acc
	mkdir -p $(INSTALL_BIN)
	cp -f acc $@
	chmod 755 $@

$(INSTALL_BIN)/acc.exe: acc.exe
	mkdir -p $(INSTALL_BIN)
	cp -f acc.exe $@
	chmod 644 $@

# --- Tests --------------------------------------------------------------

check: check-coverage check-plan check-plan-dos check-unix check-dos check-exec check-exec-dos

# Every option, option value, raw option and diagnostic must be tested.
check-coverage:
	sh $(SHFLAGS) tests/coverage.sh "$(CURDIR)"

check-plan: acc
	sh $(SHFLAGS) tests/run-plan.sh "$(ROOT)" "$(CURDIR)/acc"

check-plan-dos: acc.exe
	EMU2_DRIVE_D="$(ROOT)" sh $(SHFLAGS) tests/run-plan.sh 'D:\' "$(EMU)" "$(CURDIR)/acc.exe"

# The Unix build owns the expected output; the DOS run only records the
# cases where the DOS build legitimately differs.
bless: acc acc.exe
	ACC_BLESS=1 sh $(SHFLAGS) tests/run-plan.sh "$(ROOT)" "$(CURDIR)/acc"
	ACC_BLESS=1 EMU2_DRIVE_D="$(ROOT)" sh $(SHFLAGS) tests/run-plan.sh 'D:\' "$(EMU)" "$(CURDIR)/acc.exe"

check-unix: acc
	sh $(SHFLAGS) check.sh "$(ROOT)" "$(CURDIR)/acc"

check-dos: acc.exe
	EMU2_DRIVE_D="$(ROOT)" sh $(SHFLAGS) check.sh 'D:\' "$(EMU)" "$(CURDIR)/acc.exe"

check-exec: acc
	sh $(SHFLAGS) exec-check.sh "$(ROOT)" "$(CURDIR)/acc" "$(EMU)" "$(EXAMPLES)" \
		"$(CURDIR)/include/azdefs.h"

check-exec-dos: acc.exe
	sh $(SHFLAGS) exec-dos-check.sh "$(CURDIR)/acc.exe" "$(EMU)" "$(ROOT)" "$(EXAMPLES)"

clean:
	rm -rf obj
	rm -f acc acc.exe acc.dbg acc.sym *.o
