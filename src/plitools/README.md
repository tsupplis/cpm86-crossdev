# pli_fixobj

Fixes PL/I-86 OBJ files for compatibility with LINK86.

The PL/I-86 compiler V1.0 produces OBJ files with two issues that
LINK86 v1.4 cannot handle:

1. `SEGDEF` records carry a segment length of 0 as a placeholder.
   `pli_fixobj` scans `LEDATA` records to compute the actual segment
   sizes and patches the `SEGDEF` records accordingly.
2. `FIXUPP` records use thread subrecords (`F=1`/`T=1` references),
   which LINK86 v1.4 does not support. `pli_fixobj` expands all
   thread-based fixup references to explicit (`F=0`/`T=0`) form and
   drops the now-unneeded thread subrecords.

The output is padded to a 128-byte boundary, as required by the
CP/M-86 file system.

## Usage

```
pli_fixobj <input.obj> [output.obj]
```

If `output.obj` is omitted, the input file is patched in place.

## Origin

Ported from the Python implementation in
[ccpm-86/ccpm](https://gitlab.com/ccpm-86/ccpm).

## License

GPL 3. See [LICENSE.md](LICENSE.md).
