/*
 * Host path probes and host-to-guest (DOS) path translation.  See path.h.
 *
 * Besides exec.c, this is the only module with host-specific code: path
 * resolution uses realpath(3) on Unix and DOS system calls on DOS.  Guest
 * paths must be exact: anything DOS would alias, truncate, quote or treat
 * as a device is rejected instead of approximated.
 */
#ifndef ACC_DOS
#define _XOPEN_SOURCE 700
#endif
#include "acc.h"
#include "path.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef ACC_DOS
#include <dos.h>
#include <stat.h>
/* Aztec DOS-call helper: AH=FUNCTION, DS:SI=ARGUMENT, DL=EXTRA. */
extern int _sys(int function, char *argument, int extra);
#define SEP '\\'
#else
#include <sys/stat.h>
#include <unistd.h>
#define SEP '/'
#endif

/* Longest guest path DOS accepts for a file name, terminator excluded. */
#define DOS_PATH_LIMIT 63

/*
 * Check one guest path component of BASE name and EXT extension characters
 * starting at TEXT (DOT tells whether a '.' was present).  Rejects a name
 * or extension left empty around a dot, which DOS would alias, and the
 * reserved device names, which DOS opens as devices in any directory.
 * HOST is the user's spelling, for the diagnostic.
 */
static int dos_component(const char *text, size_t base, size_t ext, int dot,
                         const char *host)
{
    char name[9];
    size_t i;

    if (dot && (base == 0 || ext == 0))
        return fail("path requires unsupported DOS filename aliasing", host);
    for (i = 0; i < base; ++i)
        name[i] = (char)toupper((unsigned char)text[i]);
    name[base] = '\0';
    if (strcmp(name, "CON") == 0 || strcmp(name, "PRN") == 0 ||
        strcmp(name, "AUX") == 0 || strcmp(name, "NUL") == 0 ||
        (base == 4 && name[3] >= '1' && name[3] <= '9' &&
         (strncmp(name, "COM", 3) == 0 || strncmp(name, "LPT", 3) == 0)))
        return fail("path names a reserved DOS device", host);
    return 1;
}

/*
 * OUT = PARENT, a host separator unless PARENT already ends with one, then
 * NAME.  On DOS, forward slashes are normalised to backslashes.
 * OUT must hold ACC_PATH bytes.  Returns 0 if the result would not fit.
 */
int path_join(char *out, const char *parent, const char *name)
{
    size_t size;
#ifdef ACC_DOS
    char *p;
#endif

    size = strlen(parent);
    if (size + strlen(name) + 2 > ACC_PATH)
        return fail("path is too long", name);
    strcpy(out, parent);
    if (size != 0 && out[size - 1] != '/' && out[size - 1] != '\\') {
        out[size++] = SEP;
        out[size] = '\0';
    }
    strcat(out, name);
#ifdef ACC_DOS
    for (p = out; *p != '\0'; ++p) {
        if (*p == '/')
            *p = '\\';
    }
#endif
    return 1;
}

/* Non-zero if PATH names an existing directory; never prints a diagnostic
 * for a plain "no". */
int path_is_directory(const char *path)
{
    struct stat status;
#ifdef ACC_DOS
    char cwd[ACC_PATH];
    char normalized[ACC_PATH];

    if (!path_canonical(normalized, path, 0))
        return 0;
    path = normalized;
    /* DOS find-first (used by stat) cannot describe a drive's root. */
    if (strlen(path) == 3 && isalpha((unsigned char)path[0]) &&
        path[1] == ':' && (path[2] == '\\' || path[2] == '/'))
        return _sys(0x47, cwd, toupper((unsigned char)path[0]) - 'A' + 1) != -1;
#endif

    if (stat(path, &status) != 0)
        return 0;
#ifdef ACC_DOS
    return (status.st_attr & ST_DIRECT) != 0;
#else
    return S_ISDIR(status.st_mode);
#endif
}

/* Non-zero if PATH is an existing file (not a directory) that can be
 * opened for reading. */
int path_readable(const char *path)
{
    FILE *stream;

    if (path_is_directory(path))
        return 0;
    stream = fopen(path, "r");
    if (stream == 0)
        return 0;
    return fclose(stream) == 0;
}

/*
 * Store in OUT (ACC_PATH bytes) the absolute, normalised form of PATH.
 * With OUTPUT set, PATH may not exist yet, but its parent directory must.
 *
 * DOS: there is no realpath, so the drive (current drive by default) and
 * the drive's current directory are prefixed explicitly and "." and ".."
 * components are folded textually.
 * Unix: realpath resolves symbolic links.  A missing output is resolved
 * through its parent; a dangling link in its place is refused, because
 * publishing through it would write somewhere unexpected.
 */
int path_canonical(char *out, const char *path, int output)
{
#ifdef ACC_DOS
    char combined[ACC_PATH];
    char cwd[ACC_PATH];
    char root[4];
    char *read;
    char *write;
    char *start;
    int drive;
    size_t length;

    if (strlen(path) >= ACC_PATH)
        return fail("path is too long", path);
    drive = bdos(0x19, 0, 0);
    if (isalpha((unsigned char)path[0]) && path[1] == ':') {
        drive = toupper((unsigned char)path[0]) - 'A';
        path += 2;
    }
    root[0] = (char)('A' + drive);
    root[1] = ':';
    root[2] = '\\';
    root[3] = '\0';
    if (*path == '\\' || *path == '/') {
        if (!path_join(combined, root, path + 1))
            return 0;
    } else {
        if (_sys(0x47, cwd, drive + 1) == -1)
            return fail("cannot determine DOS working directory", root);
        if (!path_join(out, root, cwd) || !path_join(combined, out, path))
            return 0;
    }
    strcpy(out, root);
    write = out + 3;
    read = combined + 3;
    while (*read != '\0') {
        start = read;
        while (*read != '\0' && *read != '\\')
            ++read;
        length = (size_t)(read - start);
        if (length == 2 && start[0] == '.' && start[1] == '.') {
            while (write > out + 3 && write[-1] != '\\')
                --write;
            if (write > out + 3)
                --write;
        } else if (length != 0 && !(length == 1 && start[0] == '.')) {
            if (write > out + 3)
                *write++ = '\\';
            memcpy(write, start, length);
            write += length;
        }
        if (*read != '\0')
            ++read;
    }
    *write = '\0';
    if (output && write > out + 3) {
        strcpy(combined, out);
        read = strrchr(combined, '\\');
        if (read == combined + 2)
            read[1] = '\0';
        else
            *read = '\0';
        if (!path_is_directory(combined))
            return fail("path parent does not exist", out);
    }
    return 1;
#else
    char parent[ACC_PATH];
    char *resolved;
    char *slash;
    struct stat status;
    int ok;

    resolved = realpath(path, NULL);
    if (resolved != 0) {
        if (strlen(resolved) >= ACC_PATH) {
            free(resolved);
            return fail("resolved path is too long", path);
        }
        strcpy(out, resolved);
        free(resolved);
        return 1;
    }
    if (!output)
        return fail("cannot resolve path", path);
    if (lstat(path, &status) == 0)
        return fail("cannot resolve existing output", path);
    if (strlen(path) >= sizeof(parent))
        return fail("path is too long", path);
    strcpy(parent, path);
    slash = strrchr(parent, '/');
    if (slash == 0) {
        resolved = realpath(".", NULL);
    } else {
        *slash = '\0';
        resolved = realpath(parent[0] == '\0' ? "/" : parent, NULL);
    }
    if (resolved == 0)
        return fail("output parent does not exist", path);
    ok = path_join(out, resolved, slash == 0 ? path : slash + 1);
    free(resolved);
    return ok;
#endif
}

#ifndef ACC_DOS
/*
 * If PATH equals ROOT or lies below it, return the part of PATH after ROOT
 * (without the separating slash); otherwise return 0.  Both are canonical.
 */
static const char *within(const char *path, const char *root)
{
    size_t size;

    size = strlen(root);
    if (strncmp(path, root, size) != 0)
        return NULL;
    if (size == 1 && root[0] == '/')
        return path + 1;
    if (path[size] == '\0')
        return path + size;
    if (path[size] == '/')
        return path + size + 1;
    return NULL;
}
#endif

#ifndef ACC_DOS
/*
 * Like path_canonical for a path that is going to be named to the emulated
 * tools, except that a symbolic link in the last component is kept: the
 * tools open C:\NAME and the host follows the link itself.  The directory
 * part is still fully resolved, so a link to a directory cannot lead out of
 * the invocation directory or the tool root.
 */
static int canonical_guest(char *out, const char *host)
{
    char parent[ACC_PATH];
    struct stat status;
    char *resolved;
    char *slash;
    int ok;

    if (lstat(host, &status) != 0 || !S_ISLNK(status.st_mode))
        return path_canonical(out, host, 1);
    if (strlen(host) >= sizeof(parent))
        return fail("path is too long", host);
    strcpy(parent, host);
    slash = strrchr(parent, '/');
    if (slash == 0) {
        resolved = realpath(".", NULL);
    } else {
        *slash = '\0';
        resolved = realpath(parent[0] == '\0' ? "/" : parent, NULL);
    }
    if (resolved == 0)
        return fail("cannot resolve path", host);
    ok = strlen(resolved) < ACC_PATH &&
        path_join(out, resolved, slash == 0 ? host : slash + 1);
    free(resolved);
    return ok ? 1 : fail("resolved path is too long", host);
}
#endif

/*
 * Translate host path HOST into the guest path the backend tools will see,
 * in OUT (ACC_PATH bytes).
 *
 * Unix: a path inside the tool root becomes D:\..., inside the invocation
 * directory C:\...; when one contains the other the deeper anchor wins.
 * Paths elsewhere cannot be reached by the emulated tools and are refused;
 * a symbolic link in the last component is not followed (see canonical_guest).
 * DOS: the canonical host path is already the guest path.
 *
 * The result is then checked component by component against what DOS can
 * represent exactly: 8.3 names, printable characters other than DOS
 * separators and wildcards, no reserved devices, at most 63 characters.
 * Returns 0 after a diagnostic naming HOST.
 */
int path_guest(const path_map_t *map, const char *host, char *out)
{
    char resolved[ACC_PATH];
#ifndef ACC_DOS
    const char *relative;
#endif
    char *p;
    char *component;
    size_t base;
    size_t ext;
    int dot;

#ifdef ACC_DOS
    if (!path_canonical(resolved, host, 1))
        return 0;
    (void)map;
    strcpy(out, resolved);
#else
    if (!canonical_guest(resolved, host))
        return 0;
    relative = within(resolved, map->root);
    if (relative != 0 && strlen(map->root) >= strlen(map->cwd)) {
        strcpy(out, "D:\\");
    } else {
        relative = within(resolved, map->cwd);
        if (relative != 0) {
            strcpy(out, "C:\\");
        } else {
            relative = within(resolved, map->root);
            if (relative == 0)
                return fail("path lies outside invocation directory and Aztec root", host);
            strcpy(out, "D:\\");
        }
    }
    if (strlen(relative) + 4 > ACC_PATH)
        return fail("DOS path is too long", host);
    if (strchr(relative, '\\') != 0)
        return fail("path cannot be represented safely in DOS", host);
    strcat(out, relative);
#endif
    base = 0;
    ext = 0;
    dot = 0;
    component = out;
    /* No implicit long-name aliases or DOS quoting guesses. */
    for (p = out; *p != '\0'; ++p) {
        if (*p == '/')
            *p = '\\';
        if (*p == ':' && p == out + 1) {
            base = 0;
            component = p + 1;
            continue;
        }
        if (*p == '\\') {
            if (!dos_component(component, base, ext, dot, host))
                return 0;
            base = ext = 0;
            dot = 0;
            component = p + 1;
        } else if (*p == '.') {
            if (dot)
                return fail("path requires unsupported DOS filename aliasing", host);
            dot = 1;
        } else {
            if ((unsigned char)*p < 33 || (unsigned char)*p > 126 ||
                strchr("\";,:*?<>|+=[]", *p) != 0)
                return fail("path cannot be represented safely in DOS", host);
            if (dot)
                ++ext;
            else
                ++base;
            if (base > 8 || ext > 3)
                return fail("path requires unsupported DOS filename aliasing", host);
        }
    }
    if (strlen(out) > DOS_PATH_LIMIT)
        return fail("path exceeds the supported DOS path limit (63 characters)", host);
    return dos_component(component, base, ext, dot, host);
}

/* Non-zero if A and B are the same file name under DOS case folding. */
int path_equal(const char *a, const char *b)
{
    while (*a != '\0' && *b != '\0') {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
            return 0;
        ++a;
        ++b;
    }
    return *a == *b;
}

/*
 * Return the last component of PATH: the text after the final '/', '\\'
 * or drive colon, so both host and guest spellings work.  Points into PATH.
 */
const char *path_basename(const char *path)
{
    const char *start;

    start = path;
    while (*path != '\0') {
        if (*path == '/' || *path == '\\' || *path == ':')
            start = path + 1;
        ++path;
    }
    return start;
}


#ifndef ACC_DOS
/* Store the canonical path of PATH in OUT if it is an executable file. */
static int executable(char *out, const char *path)
{
    struct stat status;
    char *resolved;

    if (stat(path, &status) != 0 || !S_ISREG(status.st_mode) ||
        access(path, X_OK) != 0)
        return 0;
    resolved = realpath(path, NULL);
    if (resolved == 0 || strlen(resolved) >= ACC_PATH) {
        free(resolved);
        return 0;
    }
    strcpy(out, resolved);
    free(resolved);
    return 1;
}

/* Resolve NAME like a shell; an empty PATH element is ".". */
int path_program(char *out, const char *name)
{
    const char *list;
    const char *end;
    char directory[ACC_PATH];
    char candidate[ACC_PATH];
    size_t length;

    if (strchr(name, '/') != 0)
        return executable(out, name);
    list = getenv("PATH");
    if (list == 0)
        list = "/usr/bin:/bin";
    for (;;) {
        end = strchr(list, ':');
        length = end == 0 ? strlen(list) : (size_t)(end - list);
        if (length < sizeof(directory)) {
            memcpy(directory, list, length);
            directory[length] = '\0';
            if (strlen(directory) + strlen(name) + 2 <= sizeof(candidate)) {
                strcpy(candidate, length == 0 ? "." : directory);
                strcat(candidate, "/");
                strcat(candidate, name);
                if (executable(out, candidate))
                    return 1;
            }
        }
        if (end == 0)
            return 0;
        list = end + 1;
    }
}
#endif
