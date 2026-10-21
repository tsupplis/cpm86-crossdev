#!/usr/bin/env python3
"""Regression fixtures for PL/I reserved storage and OMF relocation threads."""
from pathlib import Path
import subprocess
import tempfile

TOOL = Path(__file__).resolve().parents[2] / 'bin/drpli_objfix'


def record(kind, payload):
    header = bytes([kind]) + (len(payload) + 1).to_bytes(2, 'little')
    return header + payload + bytes([-(sum(header) + sum(payload)) & 255])


def records(blob):
    pos = 0
    result = []
    while pos + 3 <= len(blob) and blob[pos] != 0x1a:
        kind = blob[pos]
        length = int.from_bytes(blob[pos + 1:pos + 3], 'little')
        chunk = blob[pos:pos + 3 + length]
        assert len(chunk) == length + 3 and sum(chunk) & 255 == 0
        result.append((kind, chunk[3:-1]))
        pos += len(chunk)
        if kind == 0x8a:
            break
    return result


def check():
    with tempfile.TemporaryDirectory(prefix='objfix-') as td:
        path = Path(td) / 'sample.obj'
        for declared, end, expected in ((0, 8, 8), (64, 8, 64),
                                         (4, 8, 8), (64, None, 64)):
            source = record(0x98, b'\x48' + declared.to_bytes(2, 'little') + b'\x01\x01\x01')
            if end is not None:
                source += record(0xa0, b'\x01\x00\x00' + b'x' * end)
            # Frame thread 1 uses group 2; target thread 1 uses segment 1.
            # Definitions must remain active across FIXUPP records.
            source += record(0x9c, b'\x45\x02\x01\x01')
            source += record(0x9c, b'\xc4\x00\x9d')
            source += record(0x8a, b'\x00')
            path.write_bytes(source)
            subprocess.run([str(TOOL), str(path)], check=True, capture_output=True)
            fixed = path.read_bytes()
            output = records(fixed)
            segdef = next(p for k, p in output if k == 0x98)
            assert int.from_bytes(segdef[1:3], 'little') == expected
            assert [p for k, p in output if k == 0x9c] == [b'\xc4\x00\x14\x02\x01']
            assert len(fixed) % 128 == 0
            subprocess.run([str(TOOL), str(path)], check=True, capture_output=True)
            assert path.read_bytes() == fixed, 'conversion must be idempotent'
    print('PASS: reserved segment sizes, zero-size placeholders, thread direction, checksums and idempotence')


if __name__ == '__main__':
    check()
