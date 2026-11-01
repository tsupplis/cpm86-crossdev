BIN := $(abspath ../../bin)
LIBDIR := ../../share/aztec42/lib
LIB := $(LIBDIR)/c86.lib

# The compiler driver reuses fixed DOS temporary filenames.
.NOTPARALLEL:
.DELETE_ON_ERROR:

.PHONY: check clean
check: regress.cmd legacy.o status-check header-check link-check response-check inf-check close-check
	@output=$$("$(BIN)/cpm86" regress.cmd) && printf '%s\n' "$$output" && printf '%s\n' "$$output" | tr -d '\r' | grep -qx 'AZTEC PASS' && printf '%s\n' "$$output" | tr -d '\r' | grep -qx 'ATEXIT PASS'

regress.cmd: regress.o $(LIB) $(LIBDIR)/m.lib
	$(BIN)/aztec42_link -o $@ regress.o -lm -lc86

regress.o: regress.c $(wildcard ../../share/aztec42/include/*.h)
	$(BIN)/aztec42_cc +0 -o $@ regress.c

clean:
	rm -f dosreg.o dosreg.com dosreg.exe legacy.o regress.o regress.cmd status.o probe31.o probe32.o exit31.cmd exit32.cmd

status.o: status.c
	$(BIN)/aztec42_cc +0 -o $@ $<

probe31.o probe32.o: probe%.o: probe.asm
	$(BIN)/aztec42_as -dVERSION=0$*h -o $@ $<

exit31.cmd exit32.cmd: exit%.cmd: status.o probe%.o $(LIB)
	$(BIN)/aztec42_link -o $@ status.o probe$*.o -lc86

.PHONY: status-check
status-check: exit31.cmd exit32.cmd
	@for version in 31 32; do \
		output=$$("$(BIN)/cpm86" exit$$version.cmd) || exit 1; \
		marker=$$(printf '%s\n' "$$output" | tr -d '\r' | grep -E '^V[YN]*$$' || :); \
		case "$$version:$$marker" in 31:V|32:VY) ;; *) printf '%s\n' "$$output"; exit 1;; esac; \
	done
	@output=$$("$(BIN)/cpm86" exit32.cmd negative) && printf '%s\n' "$$output" | tr -d '\r' | grep -qx VF

legacy.o: legacy.c $(wildcard ../../share/aztec42/include/*.h)
	$(BIN)/aztec42_cc +0 -o $@ $<

.PHONY: dos-check
dos-check:
	@set -e; for model in '' lc ld l; do \
		$(BIN)/aztec42_cc -DDOS_TIME +"$$model" +0 -o dosreg.o regress.c; \
		$(BIN)/aztec42_link -o dosreg.exe dosreg.o -lm$$model -lc$$model; \
		output=$$($(BIN)/emu2 dosreg.exe); printf '%s\n' "$$output"; \
		printf '%s\n' "$$output" | tr -d '\r' | grep -qx 'AZTEC PASS' && printf '%s\n' "$$output" | tr -d '\r' | grep -qx 'ATEXIT PASS' || exit 1; \
	done
	@$(BIN)/aztec42_cc -DDOS_TIME +0 -o dosreg.o regress.c
	@$(BIN)/aztec42_link -o dosreg.com dosreg.o -lm -ld11
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

.PHONY: truncate-check
truncate-check:
	sh truncatecheck "$(BIN)"

DOS11_SOURCE ?= $(HOME)/Projects/DOS/pcdos11-hacking
.PHONY: truncate-pce-check
truncate-pce-check:
	sh truncatepcecheck "$(BIN)" "$(DOS11_SOURCE)"

.PHONY: close-check
close-check:
	sh closecheck "$(BIN)"
