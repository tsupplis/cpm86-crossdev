# Aztec 4.2 runtime

There is one runtime patch: **runtime.patch**. It transforms the unpacked
original sources directly into the current runtime. No tarball overlay,
upgrade patch, or numbered patch chain is used.

The installation sequence is:

    Fetch originals -> unpack -> apply runtime.patch -> build -> install libraries

The baseline is AztecC86.zip plus DOS11.ARC from az8634b.zip. The fetchers
stage these archives; `unpack` extracts and normalizes the source line endings.
`cross_aztec42_build` then invokes `apply_patches`, builds every memory model,
installs the libraries and refreshes the startup objects.

`apply_patches` checks the whole patch before writing. Its single marker,
`share/aztec42/patched-runtime`, contains the identity of runtime.patch.
A matching marker makes subsequent calls a no-op. A source tree already
containing the full patch (including the former v9 installation) is adopted
without applying anything again. Old numbered markers are removed. A new
patch identity clears `built` so the libraries are rebuilt. An incompatible
or partially patched tree is rejected without changing its sources.

## Install and rebuild

For a fresh installation, run `./fetch_tools` from the repository root.
For an existing compatible installation, run:

    sh src/fetch/cross_aztec42_build --rebuild
    make -C src/aztecreg clean check dos-check

The build fetcher normally skips an installation marked `built`; `--rebuild`
forces a rebuild. A failed build leaves that marker absent, so the next run
retries. You may edit sources under share/aztec42/src and headers under
share/aztec42/include, then use the same rebuild command. If protected,
give the files you intend to edit owner write permission first.

For an old partial installation that is rejected, preserve the whole directory
before a fresh install (choose a backup name that does not already exist):

    mv share/aztec42 share/aztec42.backup
    ./fetch_tools

Keep any local source edits in that backup for review and reapplication.
The installer does not overwrite an incompatible source tree automatically.

## Regenerate the patch

Keep a pristine copy after fetching and unpacking the original distributions,
before applying any runtime patch. Generate the sole patch directly against it:

    src/aztec42-patch/build_patches "$pristine" "$edited" src/aztec42-patch/runtime.patch

Always use the original distribution as the baseline, never the previous
patched revision. The generator preserves vintage line endings. Verify the
result by applying it to a fresh copy, comparing sources, rebuilding, and
running the regression suite.

The wrappers in bin/ and fetchers in src/fetch are maintained directly in
this repository; they are not embedded in runtime.patch. The compiler's
batch-mode executable fix is also a separate fetcher step.

## Runtime behavior and regression coverage

The string comparisons fold ASCII A through Z. The bounded formatter uses the
Aztec format engine and its 16-bit int count. Link the math library before the
C library for floating-point formats or calculations. INFINITY and INFINITE
use an IEEE binary64 value; the software runtime also preserves infinity
and NaN during binary32 promotion. These extensions do not add IEEE exception handling.

Run `make -C src/aztecreg dos-check` for DOS 1.x and the four DOS 2.x memory models.
The CP/M status probe supplies version 3.1 or 3.2 at the BDOS entry point
and records whether shutdown calls function 108 with the value returned
by main. It works with an emulator that does not expose that value to
the host shell.

For BDOS versions above 3.1, exit passes the 16-bit status to function
108. The one exception is -1, which maps to the application failure code
0xff00 because 0xffff asks BDOS to read the previous status.


The integer conversions share one range-checked parser. The old misspelled
`stroul` declaration is a macro alias for `strtoul`. `strtod` accepts C89
decimal syntax and uses the existing Aztec `atof` arithmetic after limiting
the input to 18 significant digits and a bounded decimal exponent. Its
rounding precision follows that engine. Subnormal results from the software
math runtime return zero with ERANGE.

The printf and vprintf families share the original format engine. Registering
an exit callback pulls in the 32-slot atexit table. Programs using no callbacks
only pay for a null hook in their existing startup module.

DOS 1.1 reuses the DOS 2.x time, clock, localtime, gmtime, ctime and asctime
objects. Aztec time_t retains its packed DOS date/time representation, with
two-second resolution. clock() retains Aztec's hundredths-of-a-second
wall-clock convention, including midnight rollover. Existing callers retain those Aztec representations. CP/M-86 has no time/clock
implementation. setlocale, strcoll, strxfrm and strftime remain unimplemented
and have no public declarations. 

The runtime supplies isinf and isnan as double-based C99-name extensions.
They share a binary64 classifier independent of the math engine. Floating
expressions still follow the vintage compiler's promotion and arithmetic rules.

Software binary64 fmod uses integer significand division.
The integer algorithm handles subnormals and signed zero, but Aztec software
floating-point loads/returns flush subnormals. Signed zero transport is
preserved. These
existing ABI limits also apply to fmod. Zero divisors and infinite
numerators produce a quiet NaN and EDOM; NaNs propagate. The existing 8087
implementation is unchanged. It also fixes realloc(NULL), preservation of the
original allocation on failure, free(NULL), allocation-size overflow, the full
unsigned-byte ctype range, and ungetc EOF/buffer handling. fgetpos, fsetpos,
rewind, labs, div and ldiv reuse existing runtime operations. Realloc shrinking
keeps the existing block, avoiding an unnecessary allocation and copy.

setvbuf remains deferred: the legacy output paths do not yet implement its
full line-buffering contract.

The build orders every rebuilt C and math archive with Aztec's ord utility
before invoking the librarian. This is required by the single-pass linker:
printf must precede its vfprintf provider, atoi/atol their strtol provider,
and file-position wrappers their underlying stdio providers. The combined
regression previously hid these dependencies by directly referencing both.
`make -C src/aztecreg link-check` now links each of 31 entry-point cases alone
across all six targets, including all three math backends for the math cases.
The normal `check` target includes these isolated links. ord reports the
legacy cycles. The archive recipe places croot first so its dependencies
are visible before scanning the ordered members; the linker wrapper supplies
the matching startup object explicitly before the archive. The isolated tests
also cover an empty main, DOS time/clock, and DOS 2 profiling entry points.

The linker wrappers inspect `-f` response files as well as direct arguments
when selecting startup objects. `make -C src/aztecreg response-check` links
and executes all six targets with the library specified only inside LF and
CRLF response files. Caller-provided AZTEC_STARTUP/AZTEC_NOSTARTUP overrides
still take precedence. This wrapper correction requires no runtime rebuild.

[Infinity support](INF.md) and [DOS file resizing](TRUNCATE.md) are included
in the single runtime patch. Tests link installed libraries directly:

    make -C src/aztecreg clean check dos-check truncate-check expr-inf-check
    make -C src/aztecreg truncate-pce-check

The PCE test requires the separate DOS 1.1 image checkout. The emu2 correction
used by truncate-check is a temporary regression fixture, not an installation
step or a second Aztec patch.
