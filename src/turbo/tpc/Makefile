ROOT := ../../..
DOS200_INPUT ?= ../cdos-2.0/turbo.com
DOS200_OUTPUT ?= dospc200.com
DOS302_INPUT ?= ../dos-3.0/turbo.com
DOS302_OUTPUT ?= dospc302.com
DMS301_INPUT ?= ../dosms-3.0/turbo.com
DMS301_OUTPUT ?= dmspc301.com
C86301_INPUT ?= ../cpm86-3.0/turbo.cmd
C86301_OUTPUT ?= c86pc301.cmd
NASM ?= nasm
EMU ?= $(abspath $(ROOT)/bin/emu2)
TEST_OUTPUT := $(CURDIR)/tests

.DELETE_ON_ERROR:
.PHONY: all dos dms c86 check check-dos check-dms check-c86 check-cmd check-images check-com check-chain clean

all: dos dms c86
dos: dos200 dos302
dms: dms301
c86: c86301
check: check-dos check-dms check-c86
check-dos: check-dos200 check-dos302
check-dms: check-dms301
check-c86: check-c86301
check-cmd: check-c86301-cli
check-images: check-dos200-images check-dos302-images check-dms301-images check-c86301-images
check-com: check-dos200-com check-dos302-com check-dms301-com check-c86301-com
check-chain: check-dos200-chain check-dos302-chain check-dms301-chain check-c86301-chain

# Arguments: variable prefix, target, source stem, output directory, extension, input SHA-256.
define compiler_rules
.PHONY: $(2) check-$(2) check-$(2)-images check-$(2)-cli check-$(2)-com check-$(2)-chain
$(2): $$($(1)_OUTPUT)

$$($(1)_OUTPUT): $(3).asm $$($(1)_INPUT)
	@test "$$$$(shasum -a 256 "$$($(1)_INPUT)" | cut -d ' ' -f 1)" = $(6) || \
	    { echo "Unsupported $(2) input: expected the supplied original image" >&2; exit 1; }
	$$(NASM) -f bin -Wall -Werror -w-reloc-abs-word -w-reloc-rel-word \
	    -DINPUT='"$$(abspath $$($(1)_INPUT))"' -o "$$@" $(3).asm

check-$(2): check-$(2)-images check-$(2)-cli check-$(2)-com check-$(2)-chain

check-$(2)-images: $(2)
	sh $$(SHFLAGS) check-image.sh $(2) "$$(abspath $$($(1)_INPUT))" "$$(abspath $$($(1)_OUTPUT))" "$$(TEST_OUTPUT)/$(4)/image"
	sh $$(SHFLAGS) check-image-tests.sh "$$(CURDIR)/check-image.sh" $(2) "$$(abspath $$($(1)_INPUT))" "$$(abspath $$($(1)_OUTPUT))" "$$(TEST_OUTPUT)/$(4)/image-negative"

check-$(2)-com: $(2)
	sh $$(SHFLAGS) check-com.sh "$$(EMU)" "$$(abspath $$($(1)_OUTPUT))" $(5) "$$(CURDIR)/tests" "$$(TEST_OUTPUT)/$(4)/$(5)"

check-$(2)-chain: $(2)
	sh $$(SHFLAGS) check-chain.sh "$$(EMU)" "$$(abspath $$($(1)_OUTPUT))" $(5) "$$(CURDIR)/tests" "$$(TEST_OUTPUT)/$(4)/chain"
endef

$(eval $(call compiler_rules,DOS200,dos200,dospc200,dos/200,com,fad63d86ab63aa3267667167b59bb50b7f7381da249794025c2d5d06cc551473))
$(eval $(call compiler_rules,DOS302,dos302,dospc302,dos/302,com,abaf813c5432fff3d0578e5553078d97599b613fd015610b035f7aa80ff09c56))
$(eval $(call compiler_rules,DMS301,dms301,dmspc301,dms/301,com,0126e7223c4dd7c3895e6d5892790280ef7b44bfa839d51a24d8c258a3754474))
$(eval $(call compiler_rules,C86301,c86301,c86pc301,c86/301,cmd,ae897e3fdb4491fed057b88bff8b247c9fab42908b169fa7366eba5ae0cce529))

check-dos200-cli: dos200
	sh $(SHFLAGS) check.sh "$(EMU)" "$(abspath $(DOS200_OUTPUT))" "$(TEST_OUTPUT)/dos/200/cli" dos200

check-dos302-cli: dos302
	sh $(SHFLAGS) check.sh "$(EMU)" "$(abspath $(DOS302_OUTPUT))" "$(TEST_OUTPUT)/dos/302/cli" dos302

check-dms301-cli: dms301
	sh $(SHFLAGS) check.sh "$(EMU)" "$(abspath $(DMS301_OUTPUT))" "$(TEST_OUTPUT)/dms/301/cli" dms301

check-c86301-cli: c86301
	sh $(SHFLAGS) check-cmd.sh "$(EMU)" "$(abspath $(C86301_OUTPUT))" "$(abspath $(C86301_INPUT))" "$(TEST_OUTPUT)/c86/301/cli"

clean:
	rm -f "$(DOS200_OUTPUT)" "$(DOS302_OUTPUT)" "$(DMS301_OUTPUT)" "$(C86301_OUTPUT)"
