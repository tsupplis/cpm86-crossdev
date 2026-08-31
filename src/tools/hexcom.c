/*
 * TPZASM: TDL ZASM / PSA PASM compatible assembler - hexcom.c
 * DRI HEXCOM 3.00 compatible utility
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT-0
 * scspell-id: 380e86a2-6335-11f1-92c6-246e96298730
 * Imported from https://github.com/johnsonjh/tpzasm/blob/master/src/hexcom.c
 * (see issue #24), replacing the previous simplified hexcom.c
 */

/******************************************************************************/

/*
 * hexcom.c - convert an Intel HEX file to a CP/M .COM image.
 *
 * A portable ANSI C89 reimplementation of Digital Research's HEXCOM 3.00,
 * output-compatible with the original (validated against orig/hexcom.com run
 * under the tnylpo CP/M emulator).
 *
 * Reads base.hex and writes base.com. Absolute memory image from the lowest
 * to the highest loaded address, padded up to a CP/M record boundary with
 * 0x00, and prints the same report and the same diagnostics as the original.
 */

/******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/******************************************************************************/

#define TPA 0x100        /* CP/M transient program area base address */
#define RECSZ 128        /* CP/M record (sector) size                */
#define ADDRSP 0x10000UL /* 64K address space                        */

/******************************************************************************/

static unsigned char *image;
static size_t imagesz;

/******************************************************************************/

static unsigned first_addr;       /* lowest address loaded                  */
static unsigned last_addr;        /* highest address loaded                 */
static unsigned long total_bytes; /* count of data bytes actually loaded    */
static int have_first;            /* set once the first data record is seen */

/******************************************************************************/

#ifdef FREE
# undef FREE
#endif

#ifndef __ORACLE_LINT__
# define FREE(p) \
 do {            \
   free((p));    \
   (p) = NULL;   \
 } while (0)
#else
# define FREE(p) free(p)
#endif

/******************************************************************************/

#ifdef HEXCOM_NORETURN
# undef HEXCOM_NORETURN
#endif

#if defined(__ORACLE_LINT__)
# define HEXCOM_NORETURN
#elif defined(__GNUC__) || defined(__clang__)
# define HEXCOM_NORETURN __attribute__ ((noreturn))
#elif defined(_MSC_VER) && (_MSC_VER >= 1200)
# define HEXCOM_NORETURN __declspec (noreturn)
#else
# define HEXCOM_NORETURN
#endif

/******************************************************************************/

static int
hexval (int c)
{
  if (c >= '0' && c <= '9')
    return c - '0';

  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;

  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;

  return -1;
}

/******************************************************************************/

/*
 * Read one byte (two hex digits) from f.
 * *ok is cleared on a bad digit/EOF.
 */

static int
rd_byte (FILE *f, int *ok)
{
  int c1, c2, hi, lo;

  c1 = fgetc (f);

  if (EOF == c1)
    {
      *ok = 0;

      return 0;
    } /* don't read past EOF */

  c2 = fgetc (f);
  hi = hexval (c1);
  lo = hexval (c2);

  if (hi < 0 || lo < 0)
    {
      *ok = 0;

      return 0;
    }

  *ok = 1;

  return (hi << 4) | lo;
}

/******************************************************************************/

/*
 * Dump a failing record's bytes the way the original does
 * a "<addr>: " header line, then the bytes 16 per line each
 * prefixed with their address.
 */

static void
dump_record (unsigned recaddr, const unsigned char *data, int n)
{
  int i;

  (void)printf ("%04X: \n", recaddr);

  for (i = 0; i < n; i++)
    {
      if (0 == i % 16)
        (void)printf ("%04X: ", (recaddr + (unsigned)i) & 0xFFFF);

      (void)printf ("%02X ", data [i]);

      if (0 == (i + 1) % 16)
        (void)printf ("\n");
    }

  if (0 != n % 16)
    (void)printf ("\n");

  (void)fflush (stdout);
  (void)fflush (stderr);
}

/******************************************************************************/

/*
 * Every "ERROR: " diagnostic goes through the original's common routine,
 * which prints  "ERROR: <msg>" CRLF "LOAD  ADDRESS <addr>"  with no
 * trailing newline (verified by disassembly at 0x0412 in orig/hexcom.com).
 */

static HEXCOM_NORETURN void
fatal_load (const char *msg, unsigned addr)
{
  (void)printf ("ERROR: %s\nLOAD  ADDRESS %04X", msg, addr);

  if (NULL != image)
    {
      /*LINTED E_CONSTANT_CONDITION*/
      FREE (image);
    }

  exit (1);
}

/******************************************************************************/

/*
 * Record-data error (invalid hex digit / bad checksum) with the byte dump.
 */

static HEXCOM_NORETURN void
record_error (const char *msg, unsigned recaddr, unsigned erraddr,
              const unsigned char *data, int n)
{
  (void)printf ("FIRST ADDRESS %04X\n", first_addr);
  (void)printf ("%s\n", msg);
  (void)printf ("LOAD  ADDRESS %04X\n", recaddr);
  (void)printf ("ERROR ADDRESS %04X\n", erraddr);
  (void)printf ("BYTES READ    \n");

  dump_record (recaddr, data, n);

  /*LINTED E_CONSTANT_CONDITION*/
  FREE (image);

  exit (1);
}

/******************************************************************************/

static size_t xstrcpy (char *dst, const char *src, size_t dstsz)
{
  size_t n = 0;

  if (0 == dstsz)
    return 0;

  while (n + 1 < dstsz && '\0' != src [n])
    {
      dst [n] = src [n];
      n++;
    }

  dst [n] = '\0';

  return n;
}

/******************************************************************************/

static size_t xstrcat (char *dst, const char *src, size_t dstsz)
{
  size_t n = 0;

  while (n < dstsz && '\0' != dst [n])
    n++;

  if (n == dstsz)
    return n;

  {
    size_t m = 0;

    while (n + 1 < dstsz && '\0' != src [m])
      dst [n++] = src [m++];

    dst [n] = '\0';
  }

  return n;
}

/******************************************************************************/

int
main (int argc, char **argv)
{
  char base [256];
  char srcname [300];
  char dstname [300];
  char *dot;
  const char *hnp;
  FILE *src;
  FILE *out;
  unsigned char data [256] = { 0 };
  unsigned long span, records;
  unsigned long try_size;

  image = NULL;
  imagesz = 0;

  try_size = ADDRSP;

#ifndef __CPM__
# ifndef __VBCC__
  /* Flawfinder: ignore */ /* False positive CWE-807/CWE-20 */
  hnp = getenv ("HEXCOM_NO_PAD");
# else
  hnp = "1";
# endif
#endif

  for (;;)
    {
      if ((unsigned long)(size_t)try_size == try_size)
        {
          imagesz = (size_t)try_size;
          image = (unsigned char *)calloc (1, imagesz);

          if (NULL != image)
            break;
        }

      if (ADDRSP == try_size)
        try_size = 0xFFFFUL;
      else if (try_size > 0x1000UL)
        try_size -= 0x400UL;
      else
        break;
    }

  if (NULL == image)
    fatal_load ("OUT OF MEMORY", TPA);


  (void)printf ("HEXCOM\tVERS: 3.00\n");

  (void)fflush (stdout);
  (void)fflush (stderr);

  if (argc < 2 || strlen (argv [1]) >= sizeof (base))
    {
      (void)fprintf (stderr,
        "Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>\n"
        "\n"
        "Usage:\n"
        "  hexcom <basename>  (Reads basename.hex, writes basename.com)\n"
#ifndef __CPM__
# ifndef __VBCC__
        "\n"
        "Set 'HEXCOM_NO_PAD=1' in the environment to disable record padding.\n"
# endif
#endif
        "\n");

      /*LINTED E_CONSTANT_CONDITION*/
      FREE (image);

      return 1;
    }

  (void)strncpy (base, argv [1], sizeof (base) - 1);
  base [sizeof (base) - 1] = '\0';
  dot = strrchr (base, '.');

  if (NULL != dot && (0 == strcmp (dot, ".hex") || 0 == strcmp (dot, ".HEX")))
    *dot = '\0';

  (void)xstrcpy(srcname, base, sizeof (srcname));
  (void)xstrcat(srcname, ".hex", sizeof (srcname));

  (void)xstrcpy(dstname, base, sizeof (dstname));
  (void)xstrcat(dstname, ".com", sizeof (dstname));

  src = fopen (srcname, "rb");

  if (NULL == src)
    fatal_load ("CANNOT OPEN SOURCE FILE", TPA);

  /*
   * The original creates the output file before reading, so a malformed HEX
   * leaves an empty .com behind; opening it here reproduces that behavior!
   */

  out = fopen (dstname, "wb");

  if (NULL == out)
    fatal_load ("DIRECTORY FULL", TPA);

  for (;;)
    {
      unsigned addr;
      int c, ok, ll, tt, i, sum, cc;

      do
        c = fgetc (src);
      while (':' != c && EOF != c && 0x1A != c);

      if (':' != c)
        break; /* end of input: no further records */

      ll = rd_byte (src, &ok);

#ifdef IHD
# undef IHD
#endif

#define IHD "INVALID HEX DIGIT"

      if (!ok)
        record_error (IHD, 0, 0, data, 0);

      addr = (unsigned)rd_byte (src, &ok) << 8;

      if (!ok)
        record_error (IHD, 0, 0, data, 0);

      addr |= (unsigned)rd_byte (src, &ok);

      if (!ok)
        record_error (IHD, 0, 0, data, 0);

      tt = rd_byte (src, &ok);

      if (!ok)
        record_error (IHD, addr, addr, data, 0);

      if (0x01 == tt)
        break; /* end-of-file record */

      if (0x00 != tt)
        continue; /* ignore other record types */

      /*
       * A zero-length record (e.g. the ":0000000000" terminator some HEX
       * writers emit) loads nothing: it neither triggers the < 100 check
       * nor extends the image.
       */

      if (ll > 0 && addr < TPA)
        fatal_load ("LOAD ADDRESS LESS THAN 100", addr);

      if (ll > 0 && !have_first)
        {
          first_addr = addr;
          have_first = 1;
        }

      sum = ll + (int)((addr >> 8) & 0xFF) + (int)(addr & 0xFF) + tt;

      for (i = 0; i < ll; i++)
        {
          int b = rd_byte (src, &ok);

          if (!ok)
            record_error (IHD, addr, (addr + (unsigned)i) & 0xFFFF, data, i);

          data [i] = (unsigned char)b;
          sum += b;
        }

      cc = rd_byte (src, &ok);

      if (!ok)
        record_error (IHD, addr, (addr + (unsigned)ll) & 0xFFFF, data, ll);

      sum += cc;

      if (0 != (sum & 0xFF))
        record_error ("CHECKSUM ERROR ", addr,
                      (addr + (unsigned)ll) & 0xFFFF, data, ll);

      for (i = 0; i < ll; i++)
        {
          unsigned addr_masked = (addr + (unsigned)i) & 0xFFFF;

          if ((size_t)addr_masked >= imagesz)
            fatal_load ("LOAD ADDRESS TOO HIGH", addr_masked);

          image [addr_masked] = data [i];
        }

      if (ll > 0 && (addr + (unsigned)ll - 1) > last_addr)
        last_addr = (addr + (unsigned)ll - 1);

      total_bytes += (unsigned)ll;
    }

  if (ferror (src))
    {
      (void)fclose (src);
      fatal_load ("DISK READ ERROR",
                  (have_first ? first_addr : (unsigned)TPA));
    }

  (void)fclose (src);

  if (!have_first)
    {
      first_addr = 0;
      last_addr = 0;
    }

  span = ((last_addr >= first_addr) ? (last_addr - first_addr + 1) : 0);
  records = (span + RECSZ - 1) / RECSZ;

  if (NULL == hnp)
    {
      unsigned long pad_start = (unsigned long)first_addr +
                                (unsigned long)span;

      unsigned long pad_end = (unsigned long)first_addr +
                              (unsigned long)records * RECSZ;

      if (pad_end > (unsigned long)imagesz)
        pad_end = (unsigned long)imagesz;

      if (pad_start < pad_end)
        (void)memset (image + (size_t)pad_start, 0x1A,
                      (size_t)(pad_end - pad_start));
    }

  (void)printf ("FIRST ADDRESS %04X\n", first_addr);
  (void)printf ("LAST  ADDRESS %04X\n", last_addr);
  (void)printf ("BYTES READ    %04X\n", (unsigned)(total_bytes & 0xFFFFU));
  (void)printf ("RECORDS WRITTEN %02X\n\n", (unsigned)(records & 0xFFU));

  (void)fflush (stdout);
  (void)fflush (stderr);

  {
    unsigned long ul_write_size;

    ul_write_size = ((NULL == hnp) ? (unsigned long)records * RECSZ
                                   : (unsigned long)span);

    if (ul_write_size > 0)
      {
        size_t write_size;

        if (ul_write_size >
            ((unsigned long)imagesz - (unsigned long)first_addr))
          fatal_load ("IMAGE TOO LARGE", first_addr);

        write_size = (size_t)ul_write_size;

        if (fwrite (image + first_addr, 1, write_size, out) != write_size)
          {
            (void)fclose (out);
            fatal_load ("DISK WRITE ERROR", first_addr);
          }
      }
  }

  if (0 != fclose (out))
    fatal_load ("CANNOT CLOSE FILE", first_addr);

  /*LINTED E_CONSTANT_CONDITION*/
  FREE (image);

  return 0;
}

/******************************************************************************/

/*
 * Local Variables:
 * mode: c
 * indent-tabs-mode: nil
 * tab-width: 2
 * c-basic-offset: 2
 * fill-column: 80
 * eval: (setq-local display-fill-column-indicator-column 80)
 * eval: (display-fill-column-indicator-mode 1)
 * End:
 */

/******************************************************************************/
/* vim: set ft=c ts=2 sw=2 tw=0 ai expandtab cc=80 : */
/******************************************************************************/
