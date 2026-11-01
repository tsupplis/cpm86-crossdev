# DOS file resizing

Both functions are included in the single `runtime.patch` and normal DOS
library builds. No incremental overlay is required.

| API | DOS 2.x small / lc / ld / l | DOS 1.1 | CP/M-86 |
| --- | --- | --- | --- |
| `ftruncate(int fd, long length)` | Yes | Yes | No |
| `truncate(const char *path, long length)` | Yes | Yes | No |

Include `<dos.h>` for these DOS-only declarations. The common `<unistd.h>`
does not advertise them to CP/M applications. Both return zero on success,
minus one with `errno` on failure. No duplicate resize implementation:
`truncate` opens an existing file and calls `ftruncate`.

Shrinking preserves the prefix; growth explicitly writes zero bytes using a
shared 128-byte buffer. `ftruncate` restores the original descriptor position,
even when it lies past the new EOF. Invalid descriptors, negative lengths,
read-only descriptors and character devices fail. Missing paths are not
created. A failed extension can leave a partially extended file. Flush any
buffered `FILE` stream before resizing its descriptor.

DOS 2.x uses handle write (INT 21h/40h) with a zero count for shrinking.
DOS 1.1 uses random FCB block write (INT 21h/28h) with zero records and the
runtime's existing one-byte record size. DOS 1.1 FCB failures provide less
precise error information. `ENOSPC` is added as Aztec error 23, with its
`strerror` text. CP/M-86 is excluded: record-based truncation is not a
transparent substitute for this byte-length contract.

## Regression

From the repository root, run:

```
make -C src/aztecreg truncate-check
```

This links the installed runtime libraries and tests all five DOS targets. It also builds a temporary emulator with `emu2-truncate.patch`:
the current emulator treats INT 21h/28h with CX=0 as a successful no-op.
The companion patch implements that missing operation. Neither installed
emulator nor cached emulator sources are modified. Run from a terminal;
the emulator may require a controlling TTY for DOS 1.1 console setup.

To use an emulator that already implements this operation:

```
TRUNCATE_EMULATOR=/absolute/path/to/emu2 make -C src/aztecreg truncate-check
```

Tests cover shrink, growth beyond 64 KB, zero-filled new bytes, prefix and
position preservation, unchanged size, zero length, reopening, invalid and
closed descriptors, negative sizes, read-only descriptors, devices, and
missing paths. All five targets passed under the corrected emulator; all
four DOS 2.x targets also passed under the installed emulator. The DOS 1.1 kernel from `pcdos11-hacking/pcdos_base.img` also passed
under PCE (8088, 128 KB RAM): `TRUNCATE PASS: 0 failures`. DOS 2.x
kernels and injected disk-full errors have not been tested.

## DOS 1.1 kernel regression under PCE

```
make -C src/aztecreg truncate-pce-check
```

Requires PCE, mtools, Python 3, and the user's `~/Projects/DOS/pcdos11-hacking`
checkout. Override `DOS11_SOURCE` to use another checkout location and `PCE`
to select an emulator executable. This target copies `pcdos_base.img`, uses
its DOS kernel and the checkout's ROM/configuration, and boots a temporary
AUTOEXEC that runs the test and PCEEXIT. The test writes `RESULT.TXT` through
DOS; the host reads it after shutdown and requires zero failures. PCE's guest
abort hook returns status 1, so the report—not emulator exit status—determines
success. Original images, configuration and installed libraries are untouched.

## Rebuilding

Use the normal rebuild procedure in [README.md](README.md). The DOS member
lists include both APIs in `c`, `clc`, `cld`, `cl`, and `d11`; the CP/M archive
excludes them. The emulator correction is a regression fixture only, stored
in `src/aztecreg/emu2-truncate.patch`, and is not an Aztec runtime patch.
