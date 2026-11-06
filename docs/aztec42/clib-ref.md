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
| Conformance | `exit` returns `void`; `strtol`/`strtoul`/`strtod`/`stricmp` prototypes corrected; `const` added to `stdio` prototypes | all |
| Conformance | Header guards on every header; `assert.h` re-evaluates `NDEBUG` on each inclusion | all |
| New functions | `atexit`, `bsearch`, `div`, `ldiv`, `labs`, `memmove`, `strcspn`, `strspn`, `strpbrk`, `strtok`, `strerror`, `strtoul`, `remove`, `fgetpos`, `fsetpos`, `rewind` | all |
| New functions | `vprintf`, `vfprintf`, `vsprintf` | all |
| New functions | `mktime`, `difftime` | DOS |
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
| Headers | New `stdint.h` (fixed-width subset), `strings.h`, `unistd.h`; prototypes for `cpm.h`, `dos.h`, `screen.h`, `stat.h`; `os()` in `fcntl.h` | all |
| Screen | `scr_getch`, `scr_getche`, `scr_ungetch`, `scr_line`, `scr_col` | all |
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
| `<stdio.h>` | P | `setvbuf` missing; `snprintf`, `vsnprintf` (E) |
| `<stdlib.h>` | P | `abs` and the multibyte functions missing; `getenv`, `system` DOS only |
| `<string.h>` | P | `strcoll`, `strxfrm` missing (declarations removed) |
| `<time.h>` | P | DOS only; `strftime` missing; packed DOS `time_t` (section 3) |

### Functions by header

| Header | Functions | Status | Notes |
| --- | --- | --- | --- |
| `stdio.h` | `fopen`, `freopen`, `fclose`, `fflush`, `setbuf`, `tmpfile`, `tmpnam`, `remove`+, `rename` | Y | `fflush` is a macro over `flsh_`; `remove` calls `unlink` |
| | `printf`, `fprintf`, `sprintf`, `vprintf`+, `vfprintf`+, `vsprintf`+ | Y | One shared formatter; infinity prints as stars, NaN as question marks |
| | `scanf`, `fscanf`, `sscanf` | Y | |
| | `fgetc`, `fputc`, `getc`, `putc`, `getchar`, `putchar`, `fgets`, `fputs`, `gets`, `puts`, `ungetc` | Y | `fgetc` and `fputc` are macros for `getc` and `putc`; `ungetc` fixed (+) |
| | `fread`, `fwrite`, `fseek`, `ftell`, `rewind`+, `fgetpos`+, `fsetpos`+ | Y | `fpos_t` is `long` |
| | `feof`, `ferror`, `clearerr`, `perror` | Y | Macros except `perror` |
| | `setvbuf` | N | Deferred: legacy output paths lack line-buffer semantics |
| `stdlib.h` | `atoi`, `atol`, `atof`, `strtol`+, `strtoul`+, `strtod`+ | Y | `strtod` uses the Aztec `atof` engine: 18 significant digits; subnormal results return 0 with `ERANGE` |
| | `rand`, `srand`, `qsort`, `bsearch`+, `div`+, `ldiv`+, `labs`+ | Y | `RAND_MAX` is 32767 |
| | `malloc`, `calloc`, `realloc`, `free` | Y | Edge cases fixed (+) |
| | `abort`, `exit`, `atexit`+ | Y | 32 `atexit` slots, run in reverse order |
| | `getenv`, `system` | P | DOS only |
| | `abs` | N | Declared, but no implementation is linked |
| | `mblen`, `mbtowc`, `wctomb`, `mbstowcs`, `wcstombs` | N | |
| `string.h` | `memcpy`, `memmove`+, `memchr`, `memcmp`, `memset` | Y | `memmove` handles overlapping far pointers |
| | `strcpy`, `strncpy`, `strcat`, `strncat`, `strcmp`, `strncmp`, `strlen`, `strchr`, `strrchr`, `strstr` | Y | |
| | `strcspn`+, `strspn`+, `strpbrk`+, `strtok`+, `strerror`+ | Y | |
| | `strcoll`, `strxfrm` | N | |
| `math.h` | `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `atan2`, `sinh`, `cosh`, `tanh`, `exp`, `log`, `log10`, `pow`, `sqrt`, `ceil`, `floor`, `fabs`, `frexp`, `ldexp`, `modf` | Y | `log(0)` is `-INFINITY` with `ERANGE`; negative is NaN with `EDOM` (+) |
| | `fmod`+ | Y | Zero divisor or infinite numerator: quiet NaN, `EDOM` |
| `time.h` | `time`, `clock`, `localtime`, `gmtime`, `ctime`, `asctime` | P | DOS only |
| | `mktime`+, `difftime`+ | P | DOS only; 1980-2107, no time zones (section 3) |
| | `strftime` | N | |
| `setjmp.h`, `signal.h` | `setjmp`, `longjmp`, `signal`, `raise` | Y | |
| `locale.h` | `setlocale`, `localeconv` | N | |

External names are significant to 8 characters (`vsnprintf` links as `vsnprint`).

## 3. Time library (DOS)

| Item | Behaviour |
| --- | --- |
| `time_t` | Packed DOS date and time (not a Unix timestamp), 2-second resolution |
| `clock()` | Hundredths of a second, wall clock, with midnight rollover; `CLOCKS_PER_SEC` is `CLK_TCK` |
| `mktime` | Normalises all fields; ignores input `tm_wday` and `tm_yday`; returns `(time_t)-1` outside 1980-01-01 to 2107-12-31; odd seconds round down |
| `difftime` | Signed seconds between valid packed times; lives in the math libraries (`-lm -lc`) |
| `localtime` | Gregorian weekday and day of year, correct after February 2100; `tm_isdst` is 0 |
| DOS 1.1 | Shares the DOS 2 time objects |
| CP/M-86 | No time functions |

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

## 5. Known gaps

| Gap | Effect | Workaround |
| --- | --- | --- |
| `abs` declared, not linked | `Undefined symbol: abs_` | `labs`, or `#define abs(x) ((x) < 0 ? -(x) : (x))` |
| `scr_col()` returns the line, not the column | Wrong value | `scr_loc(&line, &col)` |
| `setvbuf`, `strftime`, `strcoll`, `strxfrm`, locale, multibyte | Not available | none |
| 16-bit `int` | `4*10000` overflows | use `long` |
| `ftruncate`, `truncate` | DOS only; declared in `<dos.h>` | none on CP/M |

## 6. Regression coverage

| Target (`make -C src/aztecreg ...`) | Covers |
| --- | --- |
| `check dos-check` | CP/M-86, DOS 1.1 and the four DOS 2 models; isolated link of 31 entry points on all six targets |
| `time-check` | DOS time and clock with software and interrupt 8087 math; link check of inline 8087 |
| `inf-check`, `expr-inf-check` | Infinity and NaN; the `expr` suite (11,298 checks) |
| `truncate-check`, `truncate-pce-check` | Resize on DOS 1.1 and 2, under emu2 and PCE |
| `response-check` | Library selection from LF and CRLF response files |
| `close-check` | CP/M close failure propagation |
