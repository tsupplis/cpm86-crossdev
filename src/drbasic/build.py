#!/usr/bin/env python3
"""Append a startup module to the exact supplied Personal BASIC image."""
import hashlib
from pathlib import Path
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent
source = (ROOT / 'basic12.cmd').read_bytes()
assert hashlib.sha256(source).hexdigest() == 'b352a2e3e871406b5e972d4b77c2cef21520ba3a84fe2abc59208930bcebee9a', 'Unsupported BASIC image'
code_size = 48992
with tempfile.TemporaryDirectory(prefix='drbasic-') as folder:
    patch = Path(folder) / 'startup.bin'
    subprocess.run(['nasm', '-f', 'bin', '-Wall', '-Werror', '-w-reloc-abs-word', '-w-reloc-rel-word', str(ROOT / 'startup.asm'), '-o', str(patch)], check=True)
    extension = patch.read_bytes()
header = bytearray(source[:128])
code = bytearray(source[128:128+code_size])
assert code[0xc2:0xc5] == bytes.fromhex('8b5d5a')
assert code[0x24f:0x254] == bytes.fromhex('803e7f0100')
code[0xc2:0xc5] = b'\xe8' + struct.pack('<H', (code_size-0xc5)&65535)
code[0x24f:0x254] = b'\xe9' + struct.pack('<H', (code_size+3-0x252)&65535) + b'\x90\x90'
for offset, expected, entry in (
    (0x2f37, '8b9db200', 6),
    (0x2e37, 'e99401', 9),
    (0x2b40, '8b9d3402', 12),
    (0x2aec, '8d1e1a04', 15),
):
    old = bytes.fromhex(expected)
    assert code[offset:offset+len(old)] == old
    code[offset:offset+len(old)] = (b'\xe9' +
        struct.pack('<H', (code_size+entry-offset-3)&65535) +
        b'\x90'*(len(old)-3))
code += extension
code += bytes((-len(code))%16)
assert len(code) < 65536
struct.pack_into('<H', header, 1, len(code)//16)
struct.pack_into('<H', header, 5, len(code)//16)
rest = bytearray(source[128+code_size:])
assert rest.count(b' Version 1.2\n') == 1
rest = rest.replace(b' Version 1.2\n', b'Version 1.2N\n', 1)
output = header+code+rest
(ROOT / 'basic.cmd').write_bytes(output)
print('BASIC 1.2N: appended', len(code)-code_size, 'bytes')
