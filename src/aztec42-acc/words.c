/*
 * Word lists and diagnostics shared by every module.
 *
 * A word_list_t owns heap copies of its strings; free_words releases them.
 * Every function that can fail prints its own diagnostic through fail() and
 * returns 0 (or NULL), so callers only propagate the failure.
 */
#include "acc.h"
#include <stdlib.h>
#include <string.h>

/*
 * Print "acc: MESSAGE" or "acc: MESSAGE: DETAIL" on stderr.
 * DETAIL may be 0.  Always returns 0 so callers can "return fail(...)".
 */
int fail(const char *message, const char *detail)
{
    fprintf(stderr, "acc: %s", message);
    if (detail != 0)
        fprintf(stderr, ": %s", detail);
    fputc('\n', stderr);
    return 0;
}

/*
 * Return a heap copy of TEXT, owned by the caller.
 * Fails (NULL) when TEXT does not fit ACC_PATH or memory is exhausted;
 * the length bound keeps every later fixed-size buffer copy safe.
 */
char *copy_text(const char *text)
{
    char *result;
    size_t size;

    size = strlen(text);
    if (size >= ACC_PATH) {
        fail("argument is too long", text);
        return NULL;
    }
    result = malloc(size + 1);
    if (result == 0) {
        fail("out of memory", NULL);
        return NULL;
    }
    strcpy(result, text);
    return result;
}

/*
 * Append a copy of TEXT to WORDS.
 * Returns 1, or 0 when the list is full or the copy fails.
 */
int add_word(word_list_t *words, const char *text)
{
    char *value;

    if (words->count == ACC_LIMIT)
        return fail("too many arguments (limit 128 per list)", NULL);
    value = copy_text(text);
    if (value == 0)
        return 0;
    words->items[words->count++] = value;
    return 1;
}

/* Free every string owned by WORDS and leave the list empty and reusable. */
void free_words(word_list_t *words)
{
    int i;

    for (i = 0; i < words->count; ++i)
        free(words->items[i]);
    words->count = 0;
}
