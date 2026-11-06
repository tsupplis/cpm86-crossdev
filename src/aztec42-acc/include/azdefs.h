/*
 * azdefs.h - identification macros for Aztec C 4.3 (cpm86-crossdev).
 *
 * Opt-in: #include <azdefs.h>.  Everything here is invariant for this
 * toolchain and is defined only when __AZTEC__ is: acc defines it
 * (as 1) on every compilation.  Other facts that depend on the invocation
 * are defined by acc too: __CPM86__ / __MSDOS__ (-target) and __i186__ /
 * __i286__ (-march).  The compiler itself defines __STDC__, __FILE__,
 * __LINE__, __DATE__, __TIME__, __FUNC__ and MPU8086.
 */
#ifndef AZDEFS_H
#define AZDEFS_H

#ifdef __AZTEC__

#define __AZTEC_C__ 4
#define __AZTEC_C_MINOR__ 3
#define __AZTEC_C_PATCHLEVEL__ 0
/* A long: 40300 does not fit the 16-bit int of this target. */
#define __AZTEC_C_VERSION__ \
    ((__AZTEC_C__ * 10000L) + (__AZTEC_C_MINOR__ * 100L) + __AZTEC_C_PATCHLEVEL__)
#define __VERSION__ "Aztec C 4.3.0 (https://github.com/tsupplis/cpm86-crossdev)"

/* MPU8086 is the compiler's own processor-family symbol. */
#ifdef MPU8086
#define __i86__ 1
#endif

#define __CHAR_BIT__ 8
#define __STDC_NO_ATOMICS__ 1
#define __STDC_NO_THREADS__ 1

#define __ORDER_LITTLE_ENDIAN__ 1234
#define __ORDER_BIG_ENDIAN__ 4321
#define __ORDER_PDP_ENDIAN__ 3412
#define __BYTE_ORDER__ __ORDER_LITTLE_ENDIAN__

#endif /* __AZTEC__ */

#endif
