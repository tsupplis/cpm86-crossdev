
BINDIR  = ../../bin
RASM86  = $(BINDIR)/pcdev_rasm86
LINK86  = $(BINDIR)/pcdev_linkcmd
LIB86   = $(BINDIR)/pcdev_lib86
ASM86   = $(BINDIR)/cpm86_asm86
GENCMD  = $(BINDIR)/cpm86_gencmd
EMU     ?= $(BINDIR)/emu2
CMDINFO = $(BINDIR)/cmdinfo

EXAMPLES = \
	ex1sing.cmd \
	ex2dual.cmd \
	ex3trip.cmd \
	ex4stak.cmd \
	ex5main.cmd \
	ex6lib.cmd \
	ex7genc.cmd \
	ex7gend.cmd \
	ex8macr.cmd \
	exhelp.cmd

all: $(EXAMPLES)

# ── Pattern rules ──────────────────────────────────────────────────────────────

%.obj: %.a86
	unix2dos $<
	$(RASM86) $< $$ pz sz

%.cmd: %.obj
	$(LINK86) $* '[$$sz]'
	$(CMDINFO) $@

# ── Example 5: multi-module ────────────────────────────────────────────────────

ex5main.cmd: ex5main.obj ex5util.obj
	$(LINK86) 'ex5main=ex5main,ex5util [$$sz]'

# ── Example 6: library ────────────────────────────────────────────────────────

mathlib.l86: add16.obj mul16.obj
	$(LIB86) 'mathlib=add16,mul16'

ex6lib.cmd: ex6lib.obj mathlib.l86
	$(LINK86) 'ex6lib[$$sz]=ex6lib,mathlib.l86[search]'

# ── Example 7: ASM86 / GENCMD path ────────────────────────────────────────────

ex7genc.h86: ex7genc.a86
	unix2dos $<
	$(ASM86) $<

ex7genc.cmd: ex7genc.h86
	$(GENCMD) $< 8080

ex7gend.h86: ex7gend.a86
	unix2dos $<
	$(ASM86) $<

ex7gend.cmd: ex7gend.h86
	$(GENCMD) $<
	$(CMDINFO) $@

# ── Example 9: helper library ─────────────────────────────────────────────────

helplib.l86: hlp_io.obj hlp_str.obj hlp_num.obj hlp_mem.obj hlp_cmd.obj hlp_file.obj
	$(LIB86) 'helplib=hlp_io,hlp_str,hlp_num,hlp_mem,hlp_cmd,hlp_file'

# exhelp.a86 is self-contained (no extrn) -- link standalone via pattern rule
exhelp.cmd: exhelp.obj
	$(LINK86) 'exhelp [$$sz]'
	$(CMDINFO) $@

# ── Utility ───────────────────────────────────────────────────────────────────

run: $(EXAMPLES)
	@for cmd in $(EXAMPLES); do \
		echo "--- $$cmd"; \
		$(EMU) ./$$cmd; \
	done

clean:
	$(RM) *.obj *.cmd *.lst *.sym *.h86 *.l86 *.map
