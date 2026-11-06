/*
 * Command-line parsing: argv -> request_t.
 *
 * Options use one dash and are case-sensitive.  Short options with
 * operands (-o -I -D -L -l) accept attached or separated operands;
 * descriptive options require "=value"; anything else is an input.
 * Parsing checks syntax only and never touches the filesystem.
 */
#include "acc.h"
#include "request.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*
 * Replace the string owned by *SLOT with a copy of VALUE (last option
 * wins).  Returns 0 after a diagnostic; *SLOT is unchanged then.
 */
static int set_text(char **slot, const char *value)
{
    char *text;

    text = copy_text(value);
    if (text == 0)
        return 0;
    free(*slot);
    *slot = text;
    return 1;
}

/* Non-zero if the LENGTH characters at TEXT are 1 to 4 hex digits. */
static int hex_size(const char *text, size_t length)
{
    size_t i;

    if (length == 0 || length > 4)
        return 0;
    for (i = 0; i < length; ++i) {
        if (!isxdigit((unsigned char)text[i]))
            return 0;
    }
    return 1;
}

/*
 * Validate macro definition TEXT ("NAME" or "NAME=VALUE") and append it to
 * DEFINITIONS as "NAME=VALUE" ("NAME" alone means "NAME=1", as in cc).
 * Repeating an identical definition is accepted; redefining a name with a
 * different value is an error, because cc would silently keep one of them.
 */
static int define_macro(word_list_t *definitions, const char *text)
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

/*
 * Parse ARGV into R (see request.h).  Options are processed left to right;
 * repeated scalar options keep the last value, lists keep their order.
 * Checks done here need only the options themselves; everything that needs
 * the filesystem or the installation is left to build_plan.
 */
int parse_request(request_t *r, int argc, char **argv)
{
    const char *arg;
    const char *value;
    const char *comma;
    int i;
    int selected;
    stage_t stage;
    char option;

    if (argc > 0 && !set_text(&r->program, argv[0]))
        return 0;
    for (i = 1; i < argc; ++i) {
        arg = argv[i];
        if (*arg == '\0')
            return fail("empty argument", NULL);
        if (*arg == '@')
            return fail("response files are not supported: list link inputs in a .inp file", arg);
        if (*arg != '-') {
            if (!add_word(&r->inputs, arg))
                return 0;
            continue;
        }
        if (strcmp(arg, "-c") == 0 || strcmp(arg, "-S") == 0) {
            stage = strcmp(arg, "-c") == 0 ? STAGE_OBJECT : STAGE_ASSEMBLY;
            if (r->stage != STAGE_LINK && r->stage != stage)
                return fail("-S cannot be combined with -c", NULL);
            r->stage = stage;
        } else if (strcmp(arg, "-###") == 0) {
            r->dryrun = 1;
        } else if (strcmp(arg, "-v") == 0) {
            r->verbose = 1;
        } else if (strcmp(arg, "-save-temps") == 0) {
            r->save_temps = 1;
        } else if (strcmp(arg, "-w") == 0) {
            r->quiet = 1;
        } else if (strcmp(arg, "-squeeze") == 0) {
            r->squeeze = 1;
        } else if (strcmp(arg, "-nostartfiles") == 0) {
            r->nostart = 1;
        } else if (strcmp(arg, "-nostdlib") == 0) {
            r->nostdlib = 1;
        } else if (strcmp(arg, "-g") == 0) {
            r->debug = 1;
        } else if (strncmp(arg, "-Wc,", 4) == 0 ||
                   strncmp(arg, "-Wa,", 4) == 0 ||
                   strncmp(arg, "-Wl,", 4) == 0) {
            /* One backend option per -W; commas are not separators. */
            if (arg[4] == '\0')
                return fail("empty backend option", arg);
            if (!add_word(arg[2] == 'c' ? &r->cc_options :
                          arg[2] == 'a' ? &r->as_options : &r->ln_options,
                          arg + 4))
                return 0;
        } else if (strcmp(arg, "-ansi") == 0) {
            /* cc -ansi disables near/far, which Aztec's own headers use. */
            return fail("strict ANSI mode is not supported: it disables "
                        "near and far", arg);
        } else if (strcmp(arg, "-traditional") == 0) {
            /* cc -3 (v3.40b K&R rules) is deliberately not exposed. */
            return fail("K&R mode is not supported: acc compiles ANSI C", arg);
        } else if (strcmp(arg, "-Osize") == 0) {
            r->optimize = OPTIMIZE_SIZE;
        } else if (strcmp(arg, "-Ospeed") == 0) {
            r->optimize = OPTIMIZE_SPEED;
        } else if (strncmp(arg, "-target=", 8) == 0) {
            selected = profile_choice(arg + 8, target_names, TARGET_COUNT);
            if (selected < 0)
                return 0;
            r->target = (target_t)selected;
            r->target_given = 1;
        } else if (strncmp(arg, "-mcmodel=", 9) == 0) {
            selected = profile_choice(arg + 9, model_names, MODEL_COUNT);
            if (selected < 0)
                return 0;
            r->model = (model_t)selected;
        } else if (strncmp(arg, "-march=", 7) == 0) {
            selected = profile_choice(arg + 7, cpu_names, CPU_COUNT);
            if (selected < 0)
                return 0;
            r->cpu = (cpu_t)selected;
        } else if (strncmp(arg, "-mfloat=", 8) == 0) {
            selected = profile_choice(arg + 8, float_names, FLOAT_COUNT);
            if (selected < 0)
                return 0;
            r->floating = (float_backend_t)selected;
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
        } else if (strncmp(arg, "-overlay-root=", 14) == 0) {
            value = arg + 14;
            comma = strchr(value, ',');
            /* ln +C/+D sizes: hexadecimal bytes, at most one segment. */
            if (r->overlay_root != 0 || comma == 0 ||
                !hex_size(value, (size_t)(comma - value)) ||
                !hex_size(comma + 1, strlen(comma + 1)))
                return fail("expected -overlay-root=CODE,DATA in hex bytes (1-4 digits)", arg);
            if (!set_text(&r->overlay_root, value))
                return 0;
        } else if (strncmp(arg, "-overlay=", 9) == 0) {
            if (r->overlay_table != 0 || arg[9] == '\0')
                return fail("expected one root .rsm path for -overlay", arg);
            if (!set_text(&r->overlay_table, arg + 9))
                return 0;
        } else if (arg[1] != '\0' && strchr("oIDLl", arg[1]) != 0) {
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
    if (r->squeeze && r->stage == STAGE_ASSEMBLY)
        return fail("-squeeze needs object files: it cannot be combined with -S", NULL);
    /* Without -target the model is checked at link time, from -o. */
    if (r->target_given && r->target != TARGET_DOS2 && r->model != MODEL_SMALL)
        return fail("non-small model requires -target=dos2", NULL);
    if ((r->overlay_root != 0 || r->overlay_table != 0) &&
        (r->stage != STAGE_LINK ||
         (r->overlay_root != 0 && r->overlay_table != 0)))
        return fail("overlays need one link mode: -overlay-root or -overlay", NULL);
    /* Overlay programs must use small code and small data (Tech Info 2.1). */
    if ((r->overlay_root != 0 || r->overlay_table != 0) && r->model != MODEL_SMALL)
        return fail("overlays require the small model", NULL);
    if ((r->overlay_root != 0 || r->overlay_table != 0) && r->output == 0)
        return fail("overlay links require an explicit -o output", NULL);
    return 1;
}

/* Release every string and list owned by R. */
void free_request(request_t *r)
{
    free(r->output);
    free(r->program);
    free(r->root);
    free(r->emulator);
    free(r->overlay_root);
    free(r->overlay_table);
    free_words(&r->includes);
    free_words(&r->definitions);
    free_words(&r->libraries);
    free_words(&r->inputs);
    free_words(&r->cc_options);
    free_words(&r->as_options);
    free_words(&r->ln_options);
}
