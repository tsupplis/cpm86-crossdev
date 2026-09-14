
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
	ex7genu.cmd \
	ex7gens.cmd \
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

# ── Example 1: single code segment ──────────────────────────────────────────────

ex1sing.cmd: ex1sing.obj
	$(LINK86) 'ex1sing=ex1sing [cod[ori[0]],dat[ori[0],max[0]],$$sz]'

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

ex7genu.h86: ex7genu.a86
	unix2dos $<
	$(ASM86) $<

# DATA[M37] = 55 paragraphs = 880 bytes: covers org 100h + data + rb 200h tail
ex7genu.cmd: ex7genu.h86
	$(GENCMD) $< 'DATA[M37]'
	$(CMDINFO) $@

# To see the under-sized header (no Mn), run manually:
#   cpm86_gencmd ex7genu.h86  →  DATA MN=368  (rev_buf tail missing)
# vs the correct build above:  DATA MN=880  (full rb 200h covered)

ex7gens.h86: ex7gens.a86
	unix2dos $<
	$(ASM86) $<

# STACK[M8] = 8 paragraphs = 128 bytes (STKSIZE=80h, all rw — zero bytes in hex)
# DATA[M37]  = same as ex7genu (rev_buf tail)
ex7gens.cmd: ex7gens.h86
	$(GENCMD) $< 'DATA[M37] STACK[M8]'
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
