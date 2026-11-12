.PHONY: all check clean
all: basic.cmd
basic.cmd: startup.asm build.py basic12.cmd
	python3 build.py
check: basic.cmd
	python3 check.py
clean:
	rm -f basic.cmd
