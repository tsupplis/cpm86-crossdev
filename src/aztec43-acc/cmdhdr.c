/*
 * Header flags of a CP/M-86 command file.  See cmdhdr.h.
 */
#include "acc.h"
#include "cmdhdr.h"

/* Byte 7Fh of the 128-byte header holds the flags (not in CP/M-86 1.x). */
#define FLAGS_OFFSET 0x7FL

/* The first byte of a CMD file is the type of its first group: 1 to 9. */
int cmdhdr_set_flags(const char *path, int flags)
{
    FILE *stream;
    int first;
    int current;

    stream = fopen(path, "r+b");
    if (stream == 0)
        return fail("cannot open command file", path);
    first = fgetc(stream);
    if (first < 1 || first > 9 || fseek(stream, FLAGS_OFFSET, 0) != 0 ||
        (current = fgetc(stream)) == EOF) {
        fclose(stream);
        return fail("not a CP/M-86 command file", path);
    }
    if (fseek(stream, FLAGS_OFFSET, 0) != 0 ||
        fputc(current | flags, stream) == EOF) {
        fclose(stream);
        return fail("cannot set command file flags", path);
    }
    if (fclose(stream) != 0)
        return fail("cannot close command file", path);
    return 1;
}
