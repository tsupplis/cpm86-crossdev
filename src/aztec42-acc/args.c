#include "acc.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int fail(const char *message, const char *detail)
{
    fprintf(stderr, "acc: %s", message);
    if (detail != 0)
        fprintf(stderr, ": %s", detail);
    fputc('\n', stderr);
    return 0;
}

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
    if (result == NULL) {
        fail("out of memory", NULL);
        return NULL;
    }
    strcpy(result, text);
    return result;
}

int add_word(Words *words, const char *text)
{
    char *value;

    if (words->count == ACC_LIMIT)
        return fail("too many arguments (limit 128 per list)", NULL);
    value = copy_text(text);
    if (value == NULL)
        return 0;
    words->items[words->count++] = value;
    return 1;
}

void free_words(Words *words)
{
    int i;

    for (i = 0; i < words->count; ++i)
        free(words->items[i]);
    words->count = 0;
}

static int set_text(char **slot, const char *value)
{
    char *text;

    text = copy_text(value);
    if (text == NULL)
        return 0;
    free(*slot);
    *slot = text;
    return 1;
}

static int selection(const char *value, const char *choices)
{
    const char *end;
    int index;
    size_t length;

    index = 0;
    while (*choices != '\0') {
        end = strchr(choices, '|');
        length = end == 0 ? strlen(choices) : (size_t)(end - choices);
        if (strlen(value) == length && strncmp(value, choices, length) == 0)
            return index;
        if (end == 0)
            break;
        choices = end + 1;
        ++index;
    }
    fail("invalid option value", value);
    return -1;
}

static int define_macro(Words *definitions, const char *text)
{
    char normalized[ACC_PATH];
    const char *end;
    const char *previous;
    size_t length;
    size_t i;
    int n;

    end = strchr(text, '=');
    length = end == 0 ? strlen(text) : (size_t)(end - text);
    if (length == 0 || !(isalpha((unsigned char)text[0]) || text[0] == '_'))
        return fail("invalid macro name", text);
    for (i = 1; i < length; ++i) {
        if (!(isalnum((unsigned char)text[i]) || text[i] == '_'))
            return fail("invalid macro name", text);
    }
    if (strlen(text) + (end == 0 ? 2 : 0) >= sizeof(normalized))
        return fail("macro definition is too long", text);
    strcpy(normalized, text);
    if (end == 0)
        strcat(normalized, "=1");
    for (n = 0; n < definitions->count; ++n) {
        previous = definitions->items[n];
        if (strncmp(previous, normalized, length) == 0 && previous[length] == '=') {
            if (strcmp(previous, normalized) != 0)
                return fail("conflicting macro definitions", text);
        }
    }
    return add_word(definitions, normalized);
}

int parse_request(Request *r, int argc, char **argv)
{
    const char *arg;
    const char *value;
    int i;
    int selected;
    char option;

    for (i = 1; i < argc; ++i) {
        arg = argv[i];
        if (*arg == '\0')
            return fail("empty argument", NULL);
        if (*arg == '@')
            return fail("response-file parsing is not implemented yet", arg);
        if (*arg != '-') {
            if (!add_word(&r->inputs, arg))
                return 0;
            continue;
        }
        if (strcmp(arg, "-c") == 0 || strcmp(arg, "-S") == 0) {
            selected = strcmp(arg, "-c") == 0 ? 1 : 2;
            if (r->stage != 0 && r->stage != selected)
                return fail("-S cannot be combined with -c", NULL);
            r->stage = selected;
        } else if (strcmp(arg, "-###") == 0) {
            r->dryrun = 1;
        } else if (strcmp(arg, "-v") == 0) {
            r->verbose = 1;
        } else if (strcmp(arg, "-save-temps") == 0) {
            r->save_temps = 1;
        } else if (strcmp(arg, "-w") == 0) {
            r->quiet = 1;
        } else if (strcmp(arg, "-nostartfiles") == 0) {
            r->nostart = 1;
        } else if (strcmp(arg, "-nostdlib") == 0) {
            r->nostdlib = 1;
        } else if (strcmp(arg, "-ansi") == 0) {
            r->language = 1;
        } else if (strcmp(arg, "-traditional") == 0) {
            r->language = 2;
        } else if (strcmp(arg, "-Osize") == 0) {
            r->optimize = 1;
        } else if (strcmp(arg, "-Ospeed") == 0) {
            r->optimize = 2;
        } else if (strncmp(arg, "-target=", 8) == 0) {
            selected = selection(arg + 8, "cpm86|dos1|dos2");
            if (selected < 0)
                return 0;
            r->target = selected;
        } else if (strncmp(arg, "-mcmodel=", 9) == 0) {
            selected = selection(arg + 9, "small|medium|compact|large");
            if (selected < 0)
                return 0;
            r->model = selected;
        } else if (strncmp(arg, "-march=", 7) == 0) {
            selected = selection(arg + 7, "i8086|i80186|i80286");
            if (selected < 0)
                return 0;
            r->cpu = selected;
        } else if (strncmp(arg, "-mfloat=", 8) == 0) {
            selected = selection(arg + 8, "soft|auto|8087|8087-fast");
            if (selected < 0)
                return 0;
            if (selected == 3)
                return fail("unsafe inline floating point is not qualified yet", arg);
            r->floating = selected;
        } else if (strncmp(arg, "-toolroot=", 10) == 0) {
            if (arg[10] == '\0')
                return fail("empty tool root", arg);
            if (!set_text(&r->root, arg + 10))
                return 0;
        } else if (strncmp(arg, "-emu=", 5) == 0) {
#ifdef ACC_DOS
            return fail("-emu is only available on Unix", arg);
#else
            if (arg[5] == '\0')
                return fail("empty emulator path", arg);
            if (!set_text(&r->emulator, arg + 5))
                return 0;
#endif
        } else if (arg[1] != '\0' && strchr("oIDLl", arg[1]) != NULL) {
            option = arg[1];
            value = arg + 2;
            if (*value == '\0') {
                if (i + 1 == argc || argv[i + 1][0] == '-' || argv[i + 1][0] == '\0')
                    return fail("missing option operand", arg);
                value = argv[++i];
            }
            if (option == 'o') {
                if (!set_text(&r->output, value))
                    return 0;
            } else if (option == 'I') {
                if (!add_word(&r->includes, value))
                    return 0;
            } else if (option == 'L') {
                if (!add_word(&r->libraries, value))
                    return 0;
            } else if (option == 'D') {
                if (!define_macro(&r->definitions, value))
                    return 0;
            } else {
                char library[ACC_PATH];
                if (strlen(value) + 2 >= sizeof(library))
                    return fail("library name is too long", value);
                strcpy(library, "-l");
                strcat(library, value);
                if (!add_word(&r->inputs, library))
                    return 0;
            }
        } else {
            return fail("unsupported or unknown option", arg);
        }
    }
    if (r->inputs.count == 0)
        return fail("no input", NULL);
    if (r->target != 2 && r->model != 0)
        return fail("non-small model requires -target=dos2", NULL);
    return 1;
}

void free_request(Request *r)
{
    free(r->output);
    free(r->root);
    free(r->emulator);
    free_words(&r->includes);
    free_words(&r->definitions);
    free_words(&r->libraries);
    free_words(&r->inputs);
}
