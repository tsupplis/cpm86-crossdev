/*
 * acc - portable Aztec C 4.2 compiler driver: definitions shared by every
 * module (size limits, word lists, diagnostics).
 *
 * Pipeline: parse_request (args.c, request.h) turns argv into a request_t;
 * build_plan (plan.c, plan.h) validates it into a plan_t of backend commands;
 * print_plan (plan.c) shows it for -### and -v; execute_plan (exec.c) runs
 * it.  Target-dependent facts live in profile.c, host paths in path.c.
 *
 * Portability: every source is C89 and must build warning-free both with a
 * modern Unix compiler and with Aztec C 4.2 for DOS 2.x (16-bit int and
 * size_t, 64 KB data).  ACC_DOS selects the DOS build.  Null-pointer tests
 * compare with 0: Aztec's NULL is ((void *)0) and warns on const pointers.
 */
#ifndef ACC_H
#define ACC_H

#include <stdio.h>

/* Maximum number of entries in one word_list_t. */
#define ACC_LIMIT 128

/* Maximum length of a path or argument, terminator included. */
#define ACC_PATH 1024

/* An ordered list of heap-allocated strings, owned by the list. */
typedef struct {
    char *items[ACC_LIMIT];
    int count;
} word_list_t;

/*
 * Print "acc: MESSAGE" or "acc: MESSAGE: DETAIL" on stderr.
 * Always returns 0 so callers can "return fail(...)".
 */
int fail(const char *message, const char *detail);

/* Return a heap copy of TEXT, or NULL after a diagnostic. */
char *copy_text(const char *text);

/* Append a copy of TEXT to WORDS; 0 after a diagnostic. */
int add_word(word_list_t *words, const char *text);

/* Free every string owned by WORDS and leave the list empty. */
void free_words(word_list_t *words);

#endif
