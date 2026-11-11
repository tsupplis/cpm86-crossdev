/*
 * Header flags of a CP/M-86 command file.  See cmdhdr.h.
 */
#include "acc.h"
#include "cmdhdr.h"

/* Aztec's fopen has no "b" modes and its stdio is raw: "r+" updates in place. */
#ifdef ACC_DOS
#define UPDATE_MODE "r+"
#else
#define UPDATE_MODE "r+b"
#endif

/* Byte 7Fh of the 128-byte header holds the flags (not in CP/M-86 1.x). */
#define FLAGS_OFFSET 0x7FL

/* The first byte of a CMD file is the type of its first group: 1 to 9. */
int cmdhdr_set_flags(const char *path, int flags)
{
    FILE *stream;
    int first;
    int current;

    stream = fopen(path, UPDATE_MODE);
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

/* Offsets of the data and udata sizes in the header of an .rsm file. */
#define RSM_DATA_OFFSET 0x0CL
#define RSM_UDATA_OFFSET 0x0EL

int cmdhdr_align_rsm(const char *path)
{
    FILE *stream;
    unsigned int data;
    unsigned int udata;
    int b[4];
    int i;

    stream = fopen(path, UPDATE_MODE);
    if (stream == 0)
        return fail("cannot open symbol table", path);
    if (fseek(stream, RSM_DATA_OFFSET, 0) != 0) {
        fclose(stream);
        return fail("not an overlay symbol table", path);
    }
    for (i = 0; i < 4; ++i) {
        b[i] = fgetc(stream);
        if (b[i] == EOF) {
            fclose(stream);
            return fail("not an overlay symbol table", path);
        }
    }
    data = (unsigned int)b[0] | ((unsigned int)b[1] << 8);
    udata = (unsigned int)b[2] | ((unsigned int)b[3] << 8);
    /* The sum is odd exactly when the low bits differ. */
    if (((data ^ udata) & 1) != 0) {
        if (udata == 0xFFFFU) {
            fclose(stream);
            return fail("overlay data origin cannot be aligned", path);
        }
        ++udata;
        if (fseek(stream, RSM_UDATA_OFFSET, 0) != 0 ||
            fputc((int)(udata & 0xFF), stream) == EOF ||
            fputc((int)((udata >> 8) & 0xFF), stream) == EOF) {
            fclose(stream);
            return fail("cannot align the overlay symbol table", path);
        }
    }
    if (fclose(stream) != 0)
        return fail("cannot close symbol table", path);
    return 1;
}
