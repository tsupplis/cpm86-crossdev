/*
 * Header flags of a CP/M-86 command (.cmd) file.
 *
 * ln does not set the flag byte, so acc ORs the bits a program needs into
 * the linked product, in the private workspace, before publishing it.  The
 * bits are listed in the cpm86-cmdtools README ("Flags (offset 7Fh)").
 */
#ifndef CMDHDR_H
#define CMDHDR_H

/*
 * OR FLAGS into header byte 7Fh of the command file PATH, keeping the other
 * bits.  Fails (0, after a diagnostic) if PATH cannot be opened or does not
 * start like a command file.
 */
int cmdhdr_set_flags(const char *path, int flags);

#endif
