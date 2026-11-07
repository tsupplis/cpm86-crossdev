BIN := $(abspath ../../bin)
ACC := $(abspath ../../bin/aztec43_acc)
ACCROOT := $(abspath ../../share/aztec43)
LIBDIR := ../../share/aztec43/lib
LIB := $(LIBDIR)/c86.lib

# The compiler driver reuses fixed DOS temporary filenames.
.NOTPARALLEL:
.DELETE_ON_ERROR:

.PHONY: check clean
check: regress.cmd legacy.o status-check header-check link-check response-check inf-check close-check cpmtime-check
	@output=$$("$(BIN)/emu2" regress.cmd) && printf '%s\n' "$$output" && printf '%s\n' "$$output" | tr -d '\r' | grep -qx 'AZTEC PASS' && printf '%s\n' "$$output" | tr -d '\r' | grep -qx 'ATEXIT PASS'

regress.cmd: regress.o $(LIB) $(LIBDIR)/m.lib
	$(ACC) -toolroot=$(ACCROOT) -target=cpm86 regress.o -lm -o $@

regress.o: regress.c $(wildcard ../../share/aztec43/include/*.h)
	$(ACC) -toolroot=$(ACCROOT) -target=cpm86 -c -o $@ $<

clean:
	rm -f dosreg.o dosreg.com dosreg.exe legacy.o regress.o regress.cmd status.o probe31.o probe32.o exit31.cmd exit32.cmd

status.o: status.c
	$(ACC) -toolroot=$(ACCROOT) -target=cpm86 -c -o $@ $<

probe31.o probe32.o: probe%.o: probe.asm
	$(ACC) -toolroot=$(ACCROOT) -c -DVERSION=$$((0x$*)) -o $@ $<

exit31.cmd exit32.cmd: exit%.cmd: status.o probe%.o $(LIB)
	$(ACC) -toolroot=$(ACCROOT) -target=cpm86 status.o probe$*.o -o $@

.PHONY: status-check
status-check: exit31.cmd exit32.cmd
	@for version in 31 32; do \
		output=$$("$(BIN)/emu2" exit$$version.cmd) || exit 1; \
		marker=$$(printf '%s\n' "$$output" | tr -d '\r' | grep -E '^V[YN]*$$' || :); \
		case "$$version:$$marker" in 31:V|32:VY) ;; *) printf '%s\n' "$$output"; exit 1;; esac; \
	done
	@output=$$("$(BIN)/emu2" exit32.cmd negative) && printf '%s\n' "$$output" | tr -d '\r' | grep -qx VF

legacy.o: legacy.c $(wildcard ../../share/aztec43/include/*.h)
	$(ACC) -toolroot=$(ACCROOT) -target=cpm86 -c -o $@ $<

.PHONY: dos-check
dos-check: time-check
	@set -e; for model in small medium compact large; do \
		$(ACC) -toolroot=$(ACCROOT) -target=dos2 -mcmodel="$$model" \
		    -DDOS_TIME -c -o dosreg.o regress.c; \
		$(ACC) -toolroot=$(ACCROOT) -target=dos2 -mcmodel="$$model" \
		    dosreg.o -lm -o dosreg.exe; \
		output=$$($(BIN)/emu2 dosreg.exe); printf '%s\n' "$$output"; \
		printf '%s\n' "$$output" | tr -d '\r' | grep -qx 'AZTEC PASS' && printf '%s\n' "$$output" | tr -d '\r' | grep -qx 'ATEXIT PASS' || exit 1; \
	done
	@$(ACC) -toolroot=$(ACCROOT) -target=dos1 -DDOS_TIME regress.c -lm -o dosreg.com
	@output=$$($(BIN)/emu2 dosreg.com) && printf '%s\n' "$$output" && printf '%s\n' "$$output" | tr -d '\r' | grep -qx 'AZTEC PASS' && printf '%s\n' "$$output" | tr -d '\r' | grep -qx 'ATEXIT PASS'

.PHONY: header-check
header-check:
	sh headers "$(BIN)" "$(abspath $(LIBDIR)/../include)"

.PHONY: link-check
link-check:
	sh linkcheck "$(BIN)"

.PHONY: response-check
response-check:
	sh responsecheck "$(BIN)"

# Installed runtime infinity coverage; unordered diagnostic remains separate.
.PHONY: inf-check inf-unordered-check
inf-check:
	sh infcheck "$(BIN)"

# Expected to fail until the compiler's three-way comparison ABI is addressed.
inf-unordered-check:
	sh infcheck "$(BIN)" --unordered

EXPR_SOURCE ?= $(abspath ../../../cpm86-ports/expr)
.PHONY: expr-inf-check
expr-inf-check:
	sh exprinfcheck "$(BIN)" "$(EXPR_SOURCE)"

DOS11_SOURCE ?= $(HOME)/Projects/DOS/pcdos11-hacking
.PHONY: truncate-pce-check
truncate-pce-check:
	sh truncatepcecheck "$(BIN)" "$(DOS11_SOURCE)"

.PHONY: close-check
close-check:
	sh closecheck "$(BIN)"

.PHONY: time-check
time-check:
	sh timecheck "$(BIN)"

.PHONY: cpmtime-check
cpmtime-check:
	sh cpmtimecheck "$(BIN)"
