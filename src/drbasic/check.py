#!/usr/bin/env python3
"""Exercise the patched interpreter and verify the CMD transformation."""
from pathlib import Path
import os
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent
original = (ROOT / 'basic12.cmd').read_bytes()
patched = (ROOT / 'basic.cmd').read_bytes()
code_size = struct.unpack_from('<H', patched, 1)[0]*16
assert code_size == struct.unpack_from('<H', patched, 5)[0]*16
assert patched[9:128] == original[9:128]
assert patched[128+code_size+2368:] == original[128+48992+2368:]
old_data = original[128+48992:128+48992+2368]
assert patched[128+code_size:128+code_size+2368] == old_data.replace(b' Version 1.2\n', b'Version 1.2N\n')
for offset in range(48992):
    if offset not in range(0xc2, 0xc5) and offset not in range(0x24f, 0x254) and offset not in range(0x2f37, 0x2f3b) and offset not in range(0x2e37, 0x2e3a) and offset not in range(0x2b40, 0x2b44):
        assert patched[128+offset] == original[128+offset], hex(offset)

with tempfile.TemporaryDirectory(prefix='drbasic-check-') as directory:
    work = Path(directory)
    def source(name, text):
        (work / name).write_bytes(text.replace('\n', '\r\n').encode()+b'\x1a')
    source('hello.bas', '10 PRINT "EXECUTED"\n20 END\n')
    source('bad.bas', '10 PRINT "MUST NOT RUN"\n20 INVALID STATEMENT\n')
    source('ask.bas', '10 INPUT A$\n20 PRINT "ANSWER=";A$\n30 END\n')
    def run(args, keyboard='SYSTEM\r'):
        result = subprocess.run([os.environ.get('EMU', 'emu2'), str(ROOT/'basic.cmd'), *args],
                                input=keyboard.encode(), capture_output=True, cwd=work, timeout=10)
        assert result.returncode == 0, (args, result.stderr)
        output = result.stdout.decode('latin1')
        assert 'Version 1.2N' in output
        return output
    assert 'EXECUTED' in run([], 'PRINT "EXECUTED"\rSYSTEM\r')
    output = run(['hello.bas'])
    assert 'OLD "HELLO.BAS"' in output and 'EXECUTED' not in output
    assert 'EXECUTED' in run(['hello.bas'], 'RUN\rSYSTEM\r')
    for option in ('[R', '[R]', '[r', '[r]'):
        assert run(['hello.bas', option], '').count('EXECUTED') == 1
    assert 'EXECUTED' in run(['hello', '[R]'])
    assert 'EXECUTED' in run(['a:hello.bas', '[R]'])
    assert 'File not found' in run(['absent.bas'])
    assert 'File not found' in run(['absent.bas', '[R]'])
    output = run(['bad.bas', '[R]'])
    assert 'Program not run' in output and 'MUST NOT RUN' not in output
    assert 'Something is wrong at line 20' in run(['bad.bas'])
    assert 'ANSWER=REAL INPUT' in run(['ask.bas', '[R]'], 'REAL INPUT\rSYSTEM\r')
    source('fall.bas', '10 PRINT "FELL THROUGH"\n')
    assert 'FELL THROUGH' in run(['fall.bas', '[R]'], '')
    source('empty.bas', '')
    assert 'AFTER EMPTY' in run(['empty.bas', '[R]'], 'PRINT "AFTER EMPTY"\rSYSTEM\r')
    source('stop.bas', '10 STOP\n20 END\n')
    source('error.bas', '10 GOTO 999\n20 END\n')
    for name in ('stop.bas', 'error.bas', 'bad.bas', 'absent.bas'):
        output = run([name, '[R]'], 'PRINT "STILL INTERACTIVE"\rRUN "hello.bas"\rPRINT "AFTER RUN"\rSYSTEM\r')
        assert 'STILL INTERACTIVE' in output and 'EXECUTED' in output and 'AFTER RUN' in output, output
    assert 'ANSWER=REAL INPUT' in run(['ask.bas', '[R]'], 'REAL INPUT\r')
    source('write.bas', '10 OPEN "O",#1,"out.txt"\n20 PRINT #1,"SAVED"\n30 END\n')
    output = run(['write.bas', '[R]'], '')
    assert b'SAVED' in next(p for p in work.iterdir() if p.name.lower() == 'out.txt').read_bytes(), output
    source('early.bas', '10 END\n20 PRINT "UNREACHABLE"\n')
    assert 'UNREACHABLE' not in run(['early.bas', '[R]'], '')
    for args in (['hello.bas', '[X]'], ['hello.bas', '[R]extra'],
                 ['hello.bas', '[R]', 'extra'], ['[R]'],
                 ['too-long-filename.bas'], ['*.bas'], ['hello".bas']):
        output = run(args)
        assert 'Usage: BASIC' in output and 'EXECUTED' not in output, args
print('PASS: CMD layout, unchanged original code outside hooks, startup, load errors, options and interactive INPUT')
