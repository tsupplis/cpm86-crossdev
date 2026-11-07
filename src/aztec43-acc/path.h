/*
 * Host paths and their DOS guest equivalents.
 *
 * Backend tools run in a DOS environment (emu2 on Unix, native on DOS).
 * On Unix, the invocation directory is guest drive C:, the Aztec tool root
 * is D: and the private workspace is E:.  On DOS, host paths already are
 * guest paths.  Paths that DOS cannot represent exactly (long names,
 * aliasing, reserved devices, unsafe characters, more than 63 characters)
 * are rejected rather than approximated.
 */
#ifndef PATH_H
#define PATH_H

#include "acc.h"

/* Host locations that anchor the guest drives. */
typedef struct {
    char root[ACC_PATH];    /* canonical Aztec tool root (D:) */
    char cwd[ACC_PATH];     /* canonical invocation directory (C:) */
} path_map_t;

/* OUT = PARENT + separator + NAME; returns 0 after a diagnostic. */
int path_join(char *out, const char *parent, const char *name);

/* Non-zero if PATH is an existing directory. */
int path_is_directory(const char *path);

/* Non-zero if PATH is an existing, readable regular file. */
int path_readable(const char *path);

/*
 * Make PATH absolute and normalised in OUT.  With OUTPUT set, PATH may not
 * exist yet but its parent directory must.  Returns 0 after a diagnostic.
 */
int path_canonical(char *out, const char *path, int output);

/* Translate host path HOST to its guest path in OUT. */
int path_guest(const path_map_t *map, const char *host, char *out);

/* Case-insensitive file name comparison (DOS semantics). */
int path_equal(const char *a, const char *b);

/*
 * Unix only: resolve program NAME like a shell (directly when it contains
 * '/', else through PATH) to the absolute path of an executable regular
 * file, in OUT (ACC_PATH bytes).  Returns 0, without a diagnostic, when
 * there is none.
 */
int path_program(char *out, const char *name);

/* Last component of PATH (after '/', '\\' or a drive colon). */
const char *path_basename(const char *path);

#endif
