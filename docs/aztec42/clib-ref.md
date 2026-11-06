# Aztec C86 4.2 C library: changes and ANSI C89 conformance

Baseline: Aztec C86 4.10d runtime as shipped in `AztecC86.zip`.
Delta: [`runtime.patch`](../../src/aztec42-patch/runtime.patch)
(single patch, applied by `src/fetch/cross_aztec42`). Procedure and regression
targets: [patch README](../../src/aztec42-patch/README.md),
[infinity](../../src/aztec42-patch/INF.md), [truncate](../../src/aztec42-patch/TRUNCATE.md).
Compiler driver: [`acc` manual](acc.md).

Targets: **DOS 2** (`c`, `cl`, `clc`, `cld`), **DOS 1.1** (`d11`), **CP/M-86** (`c86`).
Math libraries `m`, `m87`, `m87s` link **before** the C library (`-lm -lc`).

## 1. Summary of enhancements

| Area | Enhancement | Targets |
| --- | --- | --- |
| Conformance | `exit(main(...))`: the return value of `main` is the exit status | all |
| Conformance | `INT_MIN`, `SHRT_MIN`, `LONG_MIN` were unsigned hex constants; now signed expressions | all |
| Conformance | `localtime`, `gmtime`, `ctime`, `asctime` take `const` pointers | all |
| Conformance | `exit` returns `void`; `strtol`/`strtoul`/`strtod`/`stricmp` prototypes corrected; `const` added to `stdio` prototypes | all |
| Conformance | Header guards on every header; `assert.h` re-evaluates `NDEBUG` on each inclusion | all |
| New functions | `atexit`, `bsearch`, `div`, `ldiv`, `labs`, `memmove`, `strcspn`, `strspn`, `strpbrk`, `strtok`, `strerror`, `strtoul`, `remove`, `fgetpos`, `fsetpos`, `rewind` | all |
| New functions | `vprintf`, `vfprintf`, `vsprintf`, `abs` | all |
| New functions | `mktime`, `difftime` | all |
| New functions | `time`, `clock`, `localtime`, `gmtime`, `ctime`, `asctime` for CP/M-86, Concurrent CP/M-86 and Concurrent DOS (assembly clock interface over the DOS runtime's calendar code) | CP/M |
| New functions | `strftime` (C locale, no OS dependency) | all |
| New functions | `fmod`, `strtod` (C89 decimal syntax) in the math libraries | all |
| Robustness | One range-checked parser behind `strtol`, `strtoul`, `atoi`, `atol`; `ERANGE` on overflow | all |
| Robustness | `realloc(NULL, n)`, `realloc(p, 0)`, `free(NULL)`, size-overflow checks in `malloc`/`calloc`, `realloc` shrinks in place and keeps the original block on failure | all |
| Robustness | `ctype` table covers the full unsigned byte range and `EOF`; `qsort` with fewer than 2 elements; `ungetc` EOF and buffer handling | all |
| Robustness | `localtime` weekday and day of year (Gregorian, after 2100); `tm_isdst` and `tm_hsec` zeroed | DOS |
| I/O | CP/M `close` reports a failed directory update (`-1`, `EIO`) | CP/M |
| I/O | CP/M `lseek` reworked: true end of file from the last-record byte count | CP/M |
| I/O | DOS 1.1 `access` added; `unlink` sets `errno`; `fcb` split into `dos_fcb` and `cpm_fcb` | DOS 1.1 |
| I/O | `ftruncate`, `truncate` (byte-accurate resize) | DOS |
| I/O | `ENOSPC` (23) and `EIO` (24) with `strerror` text | all |
| Floating point | Software binary32/binary64 carry infinity, NaN and signed zero; special operands in `+ - * /`; `log`/`log10` special values | all |
| Floating point | `INFINITY`, `INFINITE`, `isinf`, `isnan` | all |
| Process | CP/M 3.1+ exit passes the status to BDOS 108 (`-1` maps to `0xff00`) | CP/M |
| Extensions | `snprintf`, `vsnprintf`, `strdup`, `strnicmp`, `strcmpi`, `strcasecmp`/`strncasecmp` macros; `stricmp`, `strlwr`, `strrev` on CP/M | all |
| Headers | New `stdint.h` (fixed-width subset), `strings.h`, `unistd.h`, `regs.h` (`sysint`, `farcall`, `segread`, `int86`), `x86.h` (`peek`/`poke`, ports, `movblock`, far pointers); `movmem`, `setmem`, `swapmem` in `memory.h`; prototypes for `cpm.h`, `dos.h`, `screen.h`, `stat.h`; `os()` in `fcntl.h` | all |
| Screen | `scr_getch`, `scr_getche`, `scr_ungetch`, `scr_line`, `scr_col`; `scr_col` returns the column | all |
| Build | Archives ordered with `ord` for single-pass linking; startup objects extracted by the fetcher | all |

## 2. ANSI C89 library conformance

Legend: **Y** conforms; **P** partial (see notes); **N** missing; **E** Aztec extension.
`+` marks a function or fix added by the 4.2 patch.

| Header | Status | Notes |
| --- | --- | --- |
| `<assert.h>` | Y | `NDEBUG` re-evaluated per inclusion (+) |
| `<ctype.h>` | Y | Macros over a 257-entry table: full byte range and `EOF` (+); `tolower`, `toupper` are functions |
| `<errno.h>` | P | `ENOSPC` (23) and `EIO` (24) are Aztec numbers (E) |
| `<float.h>` | Y | |
| `<limits.h>` | Y | Signed minima fixed (+) |
| `<locale.h>` | N | `setlocale` and `localeconv` not implemented; declaration removed (+) |
| `<math.h>` | P | All C89 functions present; `fmod` (+). Not IEEE-754 complete (section 4) |
| `<setjmp.h>` | Y | |
| `<signal.h>` | Y | |
| `<stdarg.h>` | Y | |
| `<stddef.h>` | Y | `NULL` is `((void *)0)` |
| `<stdio.h>` | P | `setvbuf` missing. Also provides `snprintf`, `vsnprintf` (E: C99, not in C89) |
| `<stdlib.h>` | P | Multibyte functions missing; `getenv`, `system` DOS only |
| `<string.h>` | P | `strcoll`, `strxfrm` missing (declarations removed) |
| `<time.h>` | Y | All C89 functions on every target. Packed DOS `time_t`, 1980 to 2107, no time zones: `gmtime` is `localtime` (section 3) |

### Functions by header

| Header | Functions | Status | Notes |
| --- | --- | --- | --- |
| `stdio.h` | `fopen`, `freopen`, `fclose`, `fflush`, `setbuf`, `tmpfile`, `tmpnam`, `remove`+, `rename` | Y | `fflush` is a macro over `flsh_`; `remove` calls `unlink` |
| | `printf`, `fprintf`, `sprintf`, `vprintf`+, `vfprintf`+, `vsprintf`+ | Y | One shared formatter; infinity prints as stars, NaN as question marks |
| | `snprintf`+, `vsnprintf`+ | E | C99, not part of C89; bounded output, `int` count (16-bit) |
| | `scanf`, `fscanf`, `sscanf` | Y | |
| | `fgetc`, `fputc`, `getc`, `putc`, `getchar`, `putchar`, `fgets`, `fputs`, `gets`, `puts`, `ungetc` | Y | `fgetc` and `fputc` are macros for `getc` and `putc`; `ungetc` fixed (+) |
| | `fread`, `fwrite`, `fseek`, `ftell`, `rewind`+, `fgetpos`+, `fsetpos`+ | Y | `fpos_t` is `long` |
| | `feof`, `ferror`, `clearerr`, `perror` | Y | Macros except `perror` |
| | `setvbuf` | N | Deferred: legacy output paths lack line-buffer semantics |
| `stdlib.h` | `atoi`, `atol`, `atof`, `strtol`+, `strtoul`+, `strtod`+ | Y | `strtod` uses the Aztec `atof` engine: 18 significant digits; subnormal results return 0 with `ERANGE` |
| | `rand`, `srand`, `qsort`, `bsearch`+, `abs`+, `div`+, `ldiv`+, `labs`+ | Y | `RAND_MAX` is 32767 |
| | `malloc`, `calloc`, `realloc`, `free` | Y | Edge cases fixed (+) |
| | `abort`, `exit`, `atexit`+ | Y | 32 `atexit` slots, run in reverse order |
| | `getenv`, `system` | P | DOS only |
| | `mblen`, `mbtowc`, `wctomb`, `mbstowcs`, `wcstombs` | N | |
| `string.h` | `memcpy`, `memmove`+, `memchr`, `memcmp`, `memset` | Y | `memmove` handles overlapping far pointers |
| | `strcpy`, `strncpy`, `strcat`, `strncat`, `strcmp`, `strncmp`, `strlen`, `strchr`, `strrchr`, `strstr` | Y | |
| | `strcspn`+, `strspn`+, `strpbrk`+, `strtok`+, `strerror`+ | Y | |
| | `strcoll`, `strxfrm` | N | |
| `math.h` | `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `atan2`, `sinh`, `cosh`, `tanh`, `exp`, `log`, `log10`, `pow`, `sqrt`, `ceil`, `floor`, `fabs`, `frexp`, `ldexp`, `modf` | Y | `log(0)` is `-INFINITY` with `ERANGE`; negative is NaN with `EDOM` (+) |
| | `fmod`+ | Y | Zero divisor or infinite numerator: quiet NaN, `EDOM` |
| `time.h` | `time`, `clock`, `localtime`, `gmtime`, `ctime`, `asctime` | Y | All targets; `const` prototypes (+); `clock` has 1-second resolution on CP/M (section 3) |
| | `mktime`+, `difftime`+ | Y | All targets; `(time_t)-1` outside 1980 to 2107 (section 3) |
| | `strftime`+ | Y | All targets; C locale (section 3) |
| `setjmp.h`, `signal.h` | `setjmp`, `longjmp`, `signal`, `raise` | Y | |
| `locale.h` | `setlocale`, `localeconv` | N | |

External names are significant to 8 characters (`vsnprintf` links as `vsnprint`).

## 3. Time library

| Item | Behaviour |
| --- | --- |
| `time_t` | Packed DOS date and time (not a Unix timestamp), 2-second resolution |
| `clock` | Hundredths of a second, wall clock, with midnight rollover; `CLOCKS_PER_SEC` is `CLK_TCK` |
| `mktime` | Normalises all fields; ignores input `tm_wday` and `tm_yday`; returns `(time_t)-1` outside 1980-01-01 to 2107-12-31; odd seconds round down |
| `difftime` | Signed seconds between valid packed times; lives in the math libraries (`-lm -lc`) |
| `localtime` | Gregorian weekday and day of year, correct after February 2100; `tm_isdst` is 0 |
| `strftime` | All targets, including CP/M-86. C locale: `%a %A %b %B %c %d %H %I %j %m %M %p %S %U %w %W %x %X %y %Y %Z %%`, plus `%e` (space-padded day). `%Z` is empty (no time zone). `%c` is `%a %b %e %H:%M:%S %Y`. Returns 0 if the result does not fit, with an empty string. An unknown specifier is copied as written |
| DOS 1.1 | Shares the DOS 2 time objects |
| CP/M-86, Concurrent CP/M-86, Concurrent DOS | `time` and `clock` are assembly (`cpm86/time.asm`); `localtime`, `gmtime`, `ctime`, `asctime`, `mktime` are the DOS objects, so `time_t` is the same packed format |

| BDOS version (function 12) | Source | Notes |
| --- | --- | --- |
| Above 22h: MP/M II and later, CP/M-86 v4, Concurrent CP/M, Concurrent DOS | BDOS 105 (`T_GET`) | Day count (1 is 1978-01-01), BCD hour and minute; BCD seconds returned in `AL` |
| 22h: CP/M-86 1.1 | System data area, BDOS 31h, offset 20h | The string `MM/DD/YY<sep>HH:MM:SS`; `YY` is 2000 to 2099 |
| Below 22h | none | `time` and `clock` return -1 |

`time` returns -1 for dates before 1980 or after 2107, or a bad clock value. `clock` is the time of day in hundredths of a second from a one-second clock, with the DOS runtime's rollover. 

## 4. Floating point limits

| Feature | Status |
| --- | --- |
| Infinity, NaN, signed zero through load, store, negate, `+ - * /` | Y |
| `isinf`, `isnan`, `INFINITY` (binary64) | Y (E) |
| Relational operators on NaN | N: Aztec `$dcmp` returns an ordered integer; use `isnan` |
| Gradual underflow, exceptions, special values in every math function | N |
| Compiler-folded constant expressions | N |
| Standard text for infinity and NaN in `printf` | N |
| 8087 hardware backend | Unchanged; assembles in all models, not executed under emu2 |

## 5. System interface (Aztec extensions)

Targets: DOS 2, DOS 1.1 and CP/M-86 unless noted. `+` marks headers added by the 4.2 patch.

| Verbs | Header | Notes |
| --- | --- | --- |
| `sysint(n, &in, &out)`, `int86` | `<regs.h>`+ | Software interrupt `n`; registers loaded from `in`, stored in `out` (same block allowed); returns the flags (bit 0 is carry). Put the segment for `DS:DX` or `ES:DI` arguments in the block (`segread`) |
| `farcall(ip, cs, &in, &out)` | `<regs.h>`+ | Far call with the same register blocks; returns the flags |
| `segread(&sregs)` | `<regs.h>`+ | Stores `CS`, `SS`, `DS`, `ES` in that order |
| `union REGS`, `struct WORDREGS`, `BYTEREGS`, `SREGS` | `<regs.h>`+ | Same names as the Aztec 3.4 `regs.h`; word registers `ax bx cx dx si di ds es`, byte registers `al ah bl bh cl ch dl dh` |
| `peekb`, `peekw`, `pokeb`, `pokew` (`peek`, `poke` are the byte forms) | `<x86.h>`+ | Absolute address as `(offset, segment)`; the same layout as a far pointer, so large-model code can pass its two halves |
| `inportb`, `inportw`, `outportb`, `outportw` (`inport`, `outport` are the byte forms) | `<x86.h>`+ | Plain `in` and `out` |
| `movblock(from_off, from_seg, to_off, to_seg, len)` | `<x86.h>`+ | Far copy, forward only (not overlap-safe). `len` 0 copies 64 KB. `len` 4 copies with interrupts disabled, so an interrupt vector is replaced atomically |
| `ptrtoabs`, `abstoptr`, `_ptradd`, `_ptrdiff` | `<x86.h>`+ | `seg:off` to and from a 20-bit absolute address (`abstoptr` returns an offset of 0 to 15); far-pointer arithmetic beyond 64 KB. Use `far void *` |
| `movmem(src, dst, len)` | `<memory.h>`+ | Near copy, overlap-safe |
| `setmem(buf, len, byte)`, `swapmem(a, b, len)`, `memccpy` | `<memory.h>`+ | Model-sized pointers |
| `bdos(func, dx, cx)` | `<dos.h>`; `<cpm.h>` as `bdos(func, dx)` | DOS 2 and DOS 1.1; returns AL |
| `dos(func, bx, cx, dx, di, si)`, `dosx`, `bdosx` | `<dos.h>` | DOS 2 only; `dos` returns AX, or -1 with `errno` set when carry is set; `dosx` and `bdosx` take a far `DS:DX` |
| `signal`, `raise`, `abort` | `<signal.h>`, `<stdlib.h>` | See below |

No `intr`, `intdos` or `int86x`: use `sysint` and `dos`.

```c
#include <regs.h>

union REGS r;
r.h.ah = 0x30;                 /* DOS: get version */
int86(0x21, &r, &r);           /* r.h.al = major, r.h.ah = minor */
```

Signals:

| Rule | Detail |
| --- | --- |
| Signals | `SIGINT SIGTERM SIGABRT SIGFPE SIGILL SIGSEGV`; `SIG_DFL`, `SIG_IGN`, `SIG_ERR`; an invalid number returns `SIG_ERR` with `EINVAL` |
| Ctrl-C and Ctrl-Break | Delivered as `SIGINT` on DOS 2 only. On DOS 1.1 and CP/M-86 `signal` and `raise` work, but only an explicit `raise` delivers a signal |
| Other signals | Raised only by `raise`. The manual: "the only one that currently has any effect is SIGINT" |
| Handler | May return, call `exit` or `longjmp`. The Ctrl-C hook resets it to `SIG_DFL` before the call; `raise` does not |
| `SIG_DFL` | `raise` calls `_exit(255)`: the normal exit code does not run, so `stdio` buffers are lost |
| `abort` | `raise(SIGABRT)`, then `_exit(255)`: a handler runs, but `stdio` is never flushed |
| `exec`, `system` | The child starts with `SIG_DFL`; the parent handlers return afterwards |

Ctrl-C delivery needs a real console and was not tested. `raise`, `sysint`, `bdos`, `dos`, the peek, poke and block verbs, and far-pointer arithmetic were run under emu2 (DOS 2, all four memory models).

## 6. Known gaps

| Gap | Effect | Workaround |
| --- | --- | --- |
| `setvbuf`, `strcoll`, `strxfrm`, locale, multibyte | Not available | none |
| CP/M `clock` | Resolution is one second | none |
| `ftruncate`, `truncate` | DOS only; declared in `<dos.h>` | none on CP/M-86 |

## 7. Regression coverage

| Target (`make -C src/aztecreg ...`) | Covers |
| --- | --- |
| `check dos-check` | CP/M-86, DOS 1.1 and the four DOS 2 models; isolated link of 31 entry points on all six targets |
| `time-check` | DOS time and clock with software and interrupt 8087 math; link check of inline 8087 |
| PCE, CP/M-86 1.1 (BDOS 22h) | `time`, `clock`, `ctime`, `localtime`, `gmtime`, `mktime`, `difftime`, `strftime` against the system-data clock: all checks pass. emu2 (BDOS 31h, `T_GET`) passes the same program |
| `cpmtime-check` | The CP/M clock conversions, run on DOS from the `c86` library: every day from 1978 to 2107 decoded from a BDOS stamp and packed, compared with `mktime`; the 1.1 date string; invalid stamps and dates |
| `inf-check`, `expr-inf-check` | Infinity and NaN; the `expr` suite (11,298 checks) |
| `truncate-check`, `truncate-pce-check` | Resize on DOS 1.1 and 2, under emu2 and PCE |
| `response-check` | Library selection from LF and CRLF response files |
| `close-check` | CP/M close failure propagation |
