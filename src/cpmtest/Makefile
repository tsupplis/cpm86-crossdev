
BINDIR  = ../../bin
RASM86  = $(BINDIR)/pcdev_rasm86
LINK86  = $(BINDIR)/pcdev_linkcmd
EMU     ?= $(BINDIR)/emu2
CMDINFO = $(BINDIR)/cmdinfo

TESTS = \
	tst00001.cmd \
	tst00201.cmd \
	tst00901.cmd \
	tst00b01.cmd \
	tst00c01.cmd \
	tst00d01.cmd \
	tst00f01.cmd \
	tst01001.cmd \
	tst01101.cmd \
	tst01201.cmd \
	tst01301.cmd \
	tst01401.cmd \
	tst01501.cmd \
	tst01502.cmd \
	tst01601.cmd \
	tst01701.cmd \
	tst01801.cmd \
	tst01901.cmd \
	tst01a01.cmd \
	tst01b01.cmd \
	tst01d01.cmd \
	tst01e01.cmd \
	tst01f01.cmd \
	tst02101.cmd \
	tst02201.cmd \
	tst02301.cmd \
	tst02401.cmd \
	tst03301.cmd \
	tst02f01.cmd \
	tst02001.cmd \
	tst03201.cmd \
	tst03501.cmd \
	tst03701.cmd \
	tst03901.cmd \
	tst03a01.cmd \
	tst09801.cmd

# sub02f02 is a P_CHAIN helper — built but not listed in TESTS
HELPERS = sub02f02.cmd

all: $(TESTS) $(HELPERS)

# tst02f01 needs its chain target present before it can run
tst02f01.cmd: sub02f02.cmd

%.obj: %.a86
	unix2dos $<
	$(RASM86) $< $$ pz sz

%.cmd: %.obj
	$(LINK86) $* '[$$sz]'
	$(CMDINFO) $@

test: $(TESTS)
	@passed=0; failed=0; \
	for cmd in $(TESTS); do \
		out=$$($(EMU) ./$$cmd 2>&1); \
		rc=$$?; \
		printf '%s' "$$out"; \
		if [ $$rc -ne 0 ] || printf '%s\n' "$$out" | grep -q '^ERR:'; then \
			echo "--- Fail: $$cmd "; \
			failed=$$((failed+1)); \
		else \
			echo "--- Pass: $$cmd "; \
			passed=$$((passed+1)); \
		fi; \
	done; \
	echo "--- Results: $$passed/$$((passed+failed)) tests passed"; \
	[ $$failed -eq 0 ]

clean:
	$(RM) *.obj *.cmd *.lst *.sym *.tmp *.out *.prt *.prn
