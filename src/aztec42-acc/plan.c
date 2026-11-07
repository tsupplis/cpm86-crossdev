/*
 * Planning: request_t -> plan_t.  See plan.h.
 *
 * build_plan validates the whole request first (installation, inputs,
 * search areas, profile compatibility), then emits one compiler or
 * assembler command per source, an optional linker command, and finally
 * moves long linker commands into a response file.  The filesystem is only
 * probed, never modified, so -### is always side-effect free.
 *
 * Every target-dependent spelling (flags, startup, libraries) comes from
 * profile.c; every host/guest path decision from path.c.
 */
#ifndef ACC_DOS
#define _XOPEN_SOURCE 700
#endif
#include "acc.h"
#include "request.h"
#include "plan.h"
#include "path.h"
#include "profile.h"
#include "raw.h"
#include <stdlib.h>
#include <string.h>
#ifndef ACC_DOS
#include <sys/stat.h>
#include <unistd.h>
#endif

/*
 * Longest command tail DOS passes to a child, in bytes, excluding the
 * terminating CR.  Aztec's fexecv silently truncates longer tails.
 */
#define DOS_TAIL_LIMIT 126

/* What a positional input is, from its suffix ("-lNAME" names a library). */
typedef enum {
    INPUT_UNKNOWN,
    INPUT_C,            /* .c: compiled by cc */
    INPUT_ASM,          /* .asm or .s: assembled by as */
    INPUT_OBJECT,       /* .o: linked as is */
    INPUT_LIBRARY,      /* .lib: linked as is */
    INPUT_LIBNAME,      /* -lNAME: resolved through the library areas */
    INPUT_LIST          /* .inp: a list of .o and .lib files to link */
} input_kind_t;

/* Planning state that does not outlive build_plan. */
typedef struct {
    path_map_t map;            /* host anchors of guest drives C: and D: */
    word_list_t includes;         /* host include areas: -I, INCLUDE, tool root */
    word_list_t libraries;        /* host library areas: -L, CLIB, tool root */
    word_list_t objects;          /* workspace objects produced for the linker */
    word_list_t inputs;     /* effective inputs: .inp files expanded in place */
    word_list_t sources;          /* guest paths of file inputs, for duplicates */
} plan_context_t;

/* Classify input PATH by its suffix (case-insensitive, as on DOS). */
static input_kind_t classify(const char *path)
{
    const char *dot;

    if (strncmp(path, "-l", 2) == 0)
        return INPUT_LIBNAME;
    dot = strrchr(path_basename(path), '.');
    if (dot == 0)
        return INPUT_UNKNOWN;
    if (path_equal(dot, ".c"))
        return INPUT_C;
    if (path_equal(dot, ".asm") || path_equal(dot, ".s"))
        return INPUT_ASM;
    if (path_equal(dot, ".o"))
        return INPUT_OBJECT;
    if (path_equal(dot, ".lib"))
        return INPUT_LIBRARY;
    if (path_equal(dot, ".inp"))
        return INPUT_LIST;
    return INPUT_UNKNOWN;
}

/* Non-zero if KIND is translated to an object before linking. */
static int is_source(input_kind_t kind)
{
    return kind == INPUT_C || kind == INPUT_ASM;
}

/* Non-zero if KIND names a file the user supplied (not -lNAME). */
static int is_file(input_kind_t kind)
{
    return kind != INPUT_UNKNOWN && kind != INPUT_LIBNAME;
}

/*
 * Append to PATHS the areas listed in environment variable NAME, using the
 * host list separator (';' on DOS, ':' on Unix).  An empty element means
 * the current directory.  An unset or empty variable adds nothing.
 */
static int env_paths(word_list_t *paths, const char *name)
{
    const char *value;
    const char *p;
    char buffer[ACC_PATH];
    size_t length;
    char separator;

#ifdef ACC_DOS
    separator = ';';
#else
    separator = ':';
#endif
    value = getenv(name);
    if (value == 0 || *value == '\0')
        return 1;
    do {
        p = strchr(value, separator);
        length = p == 0 ? strlen(value) : (size_t)(p - value);
        if (length >= sizeof(buffer))
            return fail("environment path is too long", name);
        memcpy(buffer, value, length);
        buffer[length] = '\0';
        if (!add_word(paths, length == 0 ? "." : buffer))
            return 0;
        if (p == 0)
            break;
        value = p + 1;
    } while (1);
    return 1;
}

/*
 * Build the ordered search list DEST: the user's areas (USER), then those
 * of environment VARIABLE, then SUBDIR of the tool root.  Every area must
 * exist and be reachable as a guest path, so problems surface before any
 * tool runs rather than as a confusing backend error.
 */
static int paths(plan_context_t *ctx, word_list_t *dest, const word_list_t *user,
                 const char *variable, const char *subdir)
{
    char buffer[ACC_PATH];
    char guest[ACC_PATH];
    int i;

    for (i = 0; i < user->count; ++i) {
        if (!add_word(dest, user->items[i]))
            return 0;
    }
    if (!env_paths(dest, variable) || !path_join(buffer, ctx->map.root, subdir) ||
        !add_word(dest, buffer))
        return 0;
    for (i = 0; i < dest->count; ++i) {
        if (!path_is_directory(dest->items[i]))
            return fail("search directory does not exist", dest->items[i]);
        if (!path_guest(&ctx->map, dest->items[i], guest))
            return 0;
    }
    return 1;
}

/*
 * Store in OUT (at least 16 bytes) the next workspace temporary name,
 * E:\uNNNN.SUFFIX.  SUFFIX has at most 3 characters (8.3 names).
 */
static int temp_name(plan_t *plan, const char *suffix, char *out)
{
    if (plan->temps == PLAN_TEMP_LIMIT)
        return fail("too many workspace temporaries", NULL);
    sprintf(out, "E:\\u%04d.%s", ++plan->temps, suffix);
    return 1;
}

/* Append a new command running guest program TOOL to PLAN; NULL on error. */
static command_t *command(plan_t *plan, const char *tool)
{
    command_t *result;

    result = calloc(1, sizeof(command_t));
    if (result == 0) {
        fail("out of memory", NULL);
        return NULL;
    }
    if (plan->last == 0)
        plan->first = result;
    else
        plan->last->next = result;
    plan->last = result;
    if (!add_word(&result->arguments, tool))
        return NULL;
    return result;
}

/* Append a command running backend NAME from the tool root's bin/. */
static command_t *tool_command(plan_t *plan, const plan_context_t *ctx, const char *name)
{
    char bindir[ACC_PATH];
    char path[ACC_PATH];
    char guest[ACC_PATH];

    if (!path_join(bindir, ctx->map.root, "bin") ||
        !path_join(path, bindir, name) || !path_guest(&ctx->map, path, guest))
        return NULL;
    return command(plan, guest);
}

/* Append PREFIX immediately followed by VALUE as one argument. */
static int option(word_list_t *args, const char *prefix, const char *value)
{
    char buffer[ACC_PATH];

    if (strlen(prefix) + strlen(value) >= sizeof(buffer))
        return fail("backend argument is too long", value);
    strcpy(buffer, prefix);
    strcat(buffer, value);
    return add_word(args, buffer);
}

/* Append PREFIX followed by the guest form of host path HOST. */
static int mapped(word_list_t *args, const plan_context_t *ctx,
                  const char *prefix, const char *host)
{
    char guest[ACC_PATH];

    return path_guest(&ctx->map, host, guest) && option(args, prefix, guest);
}

/*
 * Store in HOST the path of installation file NAME in the tool root's lib/
 * (startup objects, runtime and math libraries).  Fails if it is missing.
 */
static int stock(plan_context_t *ctx, const char *name, char *host)
{
    char libdir[ACC_PATH];

    if (!path_join(libdir, ctx->map.root, "lib") || !path_join(host, libdir, name))
        return 0;
    if (!path_readable(host))
        return fail("missing installation library or startup", host);
    return 1;
}

/*
 * Resolve -lNAME to the first NAME.lib in the library areas, in order, and
 * store its host path in HOST.  NAME must be a plain name: paths and
 * suffixes belong in positional inputs instead.
 */
static int user_library(plan_context_t *ctx, const char *name, char *host)
{
    char filename[ACC_PATH];
    int i;

    if (*name == '\0' || strpbrk(name, "/\\:.") != 0)
        return fail("invalid library name", name);
    if (strlen(name) + 5 >= sizeof(filename))
        return fail("library name is too long", name);
    strcpy(filename, name);
    strcat(filename, ".lib");
    for (i = 0; i < ctx->libraries.count; ++i) {
        if (!path_join(host, ctx->libraries.items[i], filename))
            return 0;
        if (path_readable(host))
            return 1;
    }
    return fail("library not found", name);
}

/*
 * Register HOST as a published output whose workspace product is PRODUCT.
 * Rejects directories, two outputs with the same guest name, and an output
 * that is (or, on Unix, hard-links to) one of the inputs: publishing would
 * destroy that input.
 */
static int output_path(plan_context_t *ctx, plan_t *plan, const char *host,
                       const char *product)
{
    char guest[ACC_PATH];
    char source[ACC_PATH];
    char resolved[ACC_PATH];
    int i;
#ifndef ACC_DOS
    struct stat destination;
    struct stat input;
#endif

    if (path_is_directory(host))
        return fail("output is a directory", host);
    if (!path_guest(&ctx->map, host, guest))
        return 0;
    for (i = 0; i < plan->outputs.count; ++i) {
        if (path_equal(plan->outputs.items[i], guest))
            return fail("output collision", host);
    }
    for (i = 0; i < ctx->inputs.count; ++i) {
        if (classify(ctx->inputs.items[i]) == INPUT_LIBNAME)
            continue;
        if (!path_guest(&ctx->map, ctx->inputs.items[i], source))
            return 0;
        if (path_equal(source, guest))
            return fail("output aliases an input", host);
#ifndef ACC_DOS
        if (stat(host, &destination) == 0 &&
            stat(ctx->inputs.items[i], &input) == 0 &&
            destination.st_dev == input.st_dev && destination.st_ino == input.st_ino)
            return fail("output aliases an input", host);
#endif
    }
    return path_canonical(resolved, host, 1) &&
        add_word(&plan->outputs, guest) && add_word(&plan->products, product) &&
        add_word(&plan->host_outputs, resolved);
}

/*
 * Store in OUT the side file that ln writes beside OUTPUT: the same name
 * with SUFFIX (".rsm" for ln -r, ".dbg" for ln -g).
 */
static int sibling_output(char *out, const char *output, const char *suffix)
{
    const char *base;
    const char *dot;

    base = path_basename(output);
    dot = strrchr(output, '.');
    if (dot == 0 || dot < base)
        return fail("output requires a filename suffix", output);
    if ((size_t)(dot - output) + 5 >= ACC_PATH)
        return fail("output path is too long", output);
    memcpy(out, output, (size_t)(dot - output));
    strcpy(out + (dot - output), suffix);
    return 1;
}

/* Fail unless installation file NAME (relative to the tool root) exists. */
static int required_tool(const plan_context_t *ctx, const char *name)
{
    char path[ACC_PATH];

    if (!path_join(path, ctx->map.root, name))
        return 0;
    if (!path_readable(path))
        return fail("required backend tool is missing", path);
    return 1;
}

/*
 * Store in OUT the tool root that contains this acc, when acc runs from
 * <root>/bin and <root>/bin/cc.exe exists.  Unix only: DOS 2 does not pass
 * a program its own path.  Returns 0 when there is no such root.
 */
static int discover_root(const request_t *r, char *out)
{
#ifdef ACC_DOS
    (void)r;
    (void)out;
    return 0;
#else
    char tool[ACC_PATH];
    char *slash;
    int level;

    if (r->program == 0 || !path_program(out, r->program))
        return 0;
    for (level = 0; level < 2; ++level) {
        slash = strrchr(out, '/');
        if (slash == 0 || slash == out)
            return 0;
        *slash = '\0';
    }
    return strlen(out) + 12 < sizeof(tool) && path_join(tool, out, "bin") &&
        path_join(tool, tool, "cc.exe") && path_readable(tool);
#endif
}

/*
 * Append to CTX->inputs the entries of link list LIST: one .o or .lib path
 * per line, relative to the invocation directory.  LF or CRLF line ends
 * and blank lines are accepted.  Options, comments, blanks inside a path,
 * sources and nested lists are errors, so a list can never change how the
 * link is configured.
 */
static int read_list(plan_context_t *ctx, const char *list)
{
    char line[ACC_PATH];
    FILE *stream;
    const char *p;
    size_t length;
    input_kind_t kind;
    int ok;

    if (!path_readable(list) || (stream = fopen(list, "r")) == 0)
        return fail("cannot read input", list);
    ok = 1;
    while (ok && fgets(line, sizeof(line), stream) != 0) {
        length = strlen(line);
        if (length == sizeof(line) - 1 && line[length - 1] != '\n') {
            ok = fail("line too long in link list", list);
            break;
        }
        while (length != 0 && (line[length - 1] == '\n' || line[length - 1] == '\r'))
            line[--length] = '\0';
        if (length == 0)
            continue;
        for (p = line; *p != '\0'; ++p) {
            if ((unsigned char)*p <= 32 || (unsigned char)*p == 127)
                ok = fail("blank or control character in link list entry", line);
        }
        kind = classify(line);
        if (ok && kind != INPUT_OBJECT && kind != INPUT_LIBRARY)
            ok = fail("link list entries must be .o or .lib files", line);
        if (ok)
            ok = add_word(&ctx->inputs, line);
    }
    fclose(stream);
    return ok;
}

/*
 * Build CTX->inputs from the request's inputs, replacing the .inp file by
 * its entries (order preserved, so -lNAME stays where the user put it).
 * A list is a link-only alternative to naming files on the command line:
 * at most one, and never mixed with other file inputs.
 */
static int expand_inputs(const request_t *r, plan_context_t *ctx)
{
    const char *item;
    input_kind_t kind;
    int lists;
    int others;
    int i;

    lists = 0;
    others = 0;
    for (i = 0; i < r->inputs.count; ++i) {
        item = r->inputs.items[i];
        kind = classify(item);
        if (kind == INPUT_LIST) {
            if (r->stage != STAGE_LINK)
                return fail("input is incompatible with selected stage", item);
            if (++lists > 1)
                return fail("only one .inp file is allowed", item);
        } else if (kind != INPUT_LIBNAME) {
            ++others;
        }
    }
    if (lists != 0 && others != 0)
        return fail("a .inp file cannot be combined with other file inputs", NULL);
    for (i = 0; i < r->inputs.count; ++i) {
        item = r->inputs.items[i];
        if (classify(item) == INPUT_LIST ? !read_list(ctx, item) :
            !add_word(&ctx->inputs, item))
            return 0;
    }
    return 1;
}

/*
 * Check everything the plan depends on before emitting any command: the
 * tool root, search areas, legacy settings, every input, the number of
 * compilation outputs, target macros, and every backend tool and startup
 * the plan will need.  Fills CTX for the planning functions.
 */
static int validate(const request_t *r, plan_context_t *ctx)
{
    char guest[ACC_PATH];
    char discovered[ACC_PATH];
    const char *root;
    const char *macro;
    int i;
    int j;
    input_kind_t kind;
    int count;

    root = r->root;
    if (root == 0 || *root == '\0')
        root = getenv("ACCROOT");
    if ((root == 0 || *root == '\0') && discover_root(r, discovered))
        root = discovered;
    if (root == 0 || *root == '\0')
        return fail("no Aztec tool root: use -toolroot=dir or ACCROOT", NULL);
    if (!path_is_directory(root) || !path_canonical(ctx->map.root, root, 0))
        return fail("invalid Aztec installation root", root);
#ifndef ACC_DOS
    if (getcwd(ctx->map.cwd, sizeof(ctx->map.cwd)) == 0)
        return fail("cannot determine working directory", NULL);
#else
    if (!path_canonical(ctx->map.cwd, ".", 0))
        return 0;
#endif
    if (!paths(ctx, &ctx->includes, &r->includes, "INCLUDE", "include") ||
        !paths(ctx, &ctx->libraries, &r->libraries, "CLIB", "lib"))
        return 0;
    /* cc searches at most 8 areas; the current directory is always one. */
    if (ctx->includes.count > 7)
        return fail("too many include areas for conservative Aztec limit (7 plus current directory)", NULL);
    if (getenv("AZTEC_STARTUP") != 0 || getenv("AZTEC_NOSTARTUP") != 0)
        return fail("legacy AZTEC_STARTUP/AZTEC_NOSTARTUP settings are not supported", NULL);
    if (!expand_inputs(r, ctx))
        return 0;
    count = 0;
    for (i = 0; i < ctx->inputs.count; ++i) {
        kind = classify(ctx->inputs.items[i]);
        if (kind == INPUT_UNKNOWN)
            return fail("unsupported input suffix", ctx->inputs.items[i]);
        if (r->stage != STAGE_LINK &&
            (!is_source(kind) || (r->stage == STAGE_ASSEMBLY && kind != INPUT_C)))
            return fail("input is incompatible with selected stage", ctx->inputs.items[i]);
        if (is_file(kind)) {
            if (!path_readable(ctx->inputs.items[i]))
                return fail("cannot read input", ctx->inputs.items[i]);
            if (!path_guest(&ctx->map, ctx->inputs.items[i], guest))
                return 0;
            if (kind != INPUT_LIBRARY) {
                for (j = 0; j < ctx->sources.count; ++j) {
                    if (path_equal(ctx->sources.items[j], guest))
                        return fail("duplicate input", ctx->inputs.items[i]);
                }
                if (!add_word(&ctx->sources, guest))
                    return 0;
            }
        }
        if (kind != INPUT_LIBRARY && kind != INPUT_LIBNAME)
            ++count;
    }
    if (count == 0)
        return fail("at least one source or object input is required", NULL);
    /* A raw option for a tool that does not run is a mistake, not a no-op. */
    for (i = 0, j = 0; i < ctx->inputs.count; ++i)
        j |= classify(ctx->inputs.items[i]) == INPUT_C ? 1 :
            classify(ctx->inputs.items[i]) == INPUT_ASM ? 2 : 0;
    if (r->cc_options.count != 0 && !(j & 1))
        return fail("-Wc given, but no C source is compiled", NULL);
    if (r->as_options.count != 0 && !(j & 2))
        return fail("-Wa given, but no .asm source is assembled", NULL);
    if (r->ln_options.count != 0 && r->stage != STAGE_LINK)
        return fail("-Wl given, but nothing is linked", NULL);
    if (r->squeeze && j == 0)
        return fail("-squeeze given, but no source is compiled", NULL);
    if (r->stage != STAGE_LINK && r->output != 0 && count != 1)
        return fail("one -o cannot name multiple compilation outputs", NULL);
    /*
     * With -target, acc defines that system's macro (__CPM86__=1,
     * __MSDOS__=1 for dos1, __MSDOS__=2 for dos2): a user definition of
     * either OS macro must agree exactly.  Without -target, both are
     * user's.
     */
    for (i = 0; r->target_given && i < r->definitions.count; ++i) {
        macro = r->definitions.items[i];
        if ((strncmp(macro, "__CPM86__=", 10) == 0 ||
             strncmp(macro, "__MSDOS__=", 10) == 0) &&
            strcmp(macro, profile_target_macro(r->target) + 2) != 0)
            return fail("conflicting target macro", macro);
    }
    /* acc defines __AZTEC__ as 1: a user definition must agree. */
    for (i = 0; i < r->definitions.count; ++i) {
        macro = r->definitions.items[i];
        if (strncmp(macro, "__AZTEC__=", 10) == 0 &&
            strcmp(macro + 10, "1") != 0)
            return fail("conflicting compiler macro", macro);
    }
    /* acc defines the CPU tier macros: a user definition must agree. */
    for (i = 0; i < r->definitions.count; ++i) {
        macro = r->definitions.items[i];
        if ((strncmp(macro, "__i186__=", 9) == 0 &&
             (r->cpu == CPU_I8086 || strcmp(macro + 9, "1") != 0)) ||
            (strncmp(macro, "__i286__=", 9) == 0 &&
             (r->cpu != CPU_I80286 || strcmp(macro + 9, "1") != 0)))
            return fail("conflicting CPU macro", macro);
    }
    /* Verify every required tool before printing any command. */
    for (i = 0; i < ctx->inputs.count; ++i) {
        kind = classify(ctx->inputs.items[i]);
        if (!is_source(kind))
            continue;
        if (!required_tool(ctx, kind == INPUT_C ? "bin/cc.exe" : "bin/as.exe"))
            return 0;
        if (kind == INPUT_C) {
            /* cc runs cgen, then as unless -S stops it after cgen. */
            if (!required_tool(ctx, "bin/cgen.exe"))
                return 0;
            if (r->stage != STAGE_ASSEMBLY && !required_tool(ctx, "bin/as.exe"))
                return 0;
        }
    }
    if (r->squeeze && !required_tool(ctx, "bin/sqz.exe"))
        return 0;
    if (r->stage == STAGE_LINK && !required_tool(ctx, "bin/ln.exe"))
        return 0;
    if (r->overlay_root != 0 && !required_tool(ctx, "lib/ovld.o"))
        return 0;
    if (r->overlay_table != 0 && !required_tool(ctx, "lib/ovbgn.o"))
        return 0;
    return 1;
}

/* Non-zero if TEXT is a non-empty run of decimal digits. */
static int decimal_value(const char *text)
{
    if (*text == '\0')
        return 0;
    for (; *text != '\0'; ++text) {
        if (*text < '0' || *text > '9')
            return 0;
    }
    return 1;
}

/* Append FLAG to ARGS unless it is NULL (the setting needs no flag). */
static int add_flag(word_list_t *args, const char *flag)
{
    return flag == 0 || add_word(args, flag);
}

/*
 * Plan "sqz PRODUCT" right after the command that created PRODUCT from
 * INPUT.  sqz rewrites the object in place, so the product keeps its name
 * and the publish and link steps need no change.
 */
static int squeeze_product(plan_t *plan, const plan_context_t *ctx,
                           const char *product, const char *input)
{
    command_t *cmd;

    cmd = tool_command(plan, ctx, "sqz.exe");
    if (cmd == 0)
        return 0;
    cmd->product = copy_text(product);
    cmd->source = copy_text(input);
    return cmd->product != 0 && cmd->source != 0 &&
        add_word(&cmd->arguments, product);
}

/*
 * Plan the compiler or assembler command for source INPUT.  The product
 * gets the next workspace temporary name.
 * In link mode it stays in the workspace for the linker; with -c or -S it
 * is published as -o, or as INPUT with its suffix replaced.
 */
static int compile_input(const request_t *r, plan_context_t *ctx, plan_t *plan,
                         const char *input, input_kind_t kind)
{
    raw_context_t raw;
    const word_list_t *raw_options;
    const char *const *macros;
    command_t *cmd;
    word_list_t *args;
    char product[32];
    char output[ACC_PATH];
    char *dot;
    int i;

    if (!temp_name(plan, r->stage == STAGE_ASSEMBLY ? "asm" : "o", product))
        return 0;
    cmd = tool_command(plan, ctx, kind == INPUT_C ? "cc.exe" : "as.exe");
    if (cmd == 0)
        return 0;
    cmd->product = copy_text(product);
    cmd->source = copy_text(input);
    if (cmd->product == 0 || cmd->source == 0)
        return 0;
    args = &cmd->arguments;
    if (kind == INPUT_C) {
        /* -b: never wait for a key after errors; -n: no debugger data. */
        if (!add_word(args, profile_cpu_flag(r->cpu)) ||
            !add_word(args, "-b") || (!r->debug && !add_word(args, "-n")) ||
            !add_flag(args, profile_model_flag(r->model)) ||
            !add_flag(args, profile_float_flag(r->floating)) ||
            !add_flag(args, profile_optimize_flag(r->optimize)) ||
            (r->quiet && !add_word(args, "-s")) ||
            (r->stage == STAGE_ASSEMBLY && !add_word(args, "-A")))
            return 0;
    } else if (!add_flag(args, profile_assembler_cpu_flag(r->cpu)) ||
               !add_flag(args, profile_assembler_float_flag(r->cpu, r->floating))) {
        return 0;
    }
    raw.assembly_stage = r->stage == STAGE_ASSEMBLY;
    raw.large_data = r->model == MODEL_COMPACT || r->model == MODEL_LARGE;
    raw.dos_link = 0;
    raw.dos2_exe = 0;
    raw_options = kind == INPUT_C ? &r->cc_options : &r->as_options;
    for (i = 0; i < raw_options->count; ++i) {
        if (!raw_append(args, kind == INPUT_C ? RAW_COMPILER : RAW_ASSEMBLER,
                        raw_options->items[i], &raw))
            return 0;
    }
    if (!add_word(args, profile_identity_macro()))
        return 0;
    /* Compilation is OS-neutral unless -target is given. */
    if (r->target_given && !add_word(args, profile_target_macro(r->target)))
        return 0;
    for (macros = profile_cpu_macros(r->cpu); *macros != 0; ++macros) {
        if (!add_word(args, *macros))
            return 0;
    }
    for (i = 0; i < r->definitions.count; ++i) {
        /* as -Dsym=const: the value must be a decimal constant. */
        if (kind == INPUT_ASM && !decimal_value(strchr(r->definitions.items[i], '=') + 1))
            return fail("assembler -D values must be decimal constants",
                        r->definitions.items[i]);
        if (!option(args, "-D", r->definitions.items[i]))
            return 0;
    }
    for (i = 0; i < ctx->includes.count; ++i) {
        if (!mapped(args, ctx, "-I", ctx->includes.items[i]))
            return 0;
    }
    if (!add_word(args, "-o") || !add_word(args, product) ||
        !mapped(args, ctx, "", input))
        return 0;
    if (r->squeeze && !squeeze_product(plan, ctx, product, input))
        return 0;
    if (r->stage == STAGE_LINK)
        return add_word(&ctx->objects, product);
    if (r->output != 0) {
        strcpy(output, r->output);
    } else {
        strcpy(output, path_basename(input));
        dot = strrchr(output, '.');
        *dot = '\0';
        strcat(output, r->stage == STAGE_ASSEMBLY ? ".asm" : ".o");
    }
    return output_path(ctx, plan, output, product);
}

/*
 * Resolve the link target into PLAN.  The -o suffix selects the system:
 * .cmd is CP/M-86; .exe and .com are DOS 2, or DOS 1 with -target=dos1.
 * An explicit -target must agree with the suffix.  Without -o, -target
 * decides, else CP/M-86 (a.cmd).  An overlay (.ovr) links no startup or
 * runtime, so its target is only known when -target is given.
 */
static int resolve_target(const request_t *r, plan_t *plan)
{
    const char *suffix;
    int dos;

    plan->target = r->target_given ? r->target : TARGET_CPM86;
    plan->target_known = 1;
    if (r->overlay_table != 0) {
        plan->target_known = r->target_given;
        return 1;
    }
    if (r->output == 0)
        return 1;
    suffix = strrchr(path_basename(r->output), '.');
    if (suffix == 0)
        return fail("output needs a .cmd, .exe or .com suffix", r->output);
    if (path_equal(suffix, ".cmd"))
        dos = 0;
    else if (path_equal(suffix, ".exe") || path_equal(suffix, ".com"))
        dos = 1;
    else
        return fail("output needs a .cmd, .exe or .com suffix", r->output);
    if (r->target_given && dos != (r->target != TARGET_CPM86))
        return fail("output suffix conflicts with target", r->output);
    if (!r->target_given)
        plan->target = dos ? TARGET_DOS2 : TARGET_CPM86;
    return 1;
}

/*
 * Plan the linker command.  Argument order matters to the single-pass
 * Aztec linker:
 *   -o product [+c CODE +d DATA -r]   output and overlay root options
 *   startup                           unless -nostartfiles/-nostdlib/overlay
 *   inputs, in command-line order     with -lNAME resolved to a path
 *   ovld.o | ROOT.rsm ovbgn.o         overlay loader or overlay start
 *   C runtime                         last, unless -nostdlib or overlay
 * The output suffix must match the target: .cmd for CP/M-86, .exe or .com
 * for DOS, .ovr for an overlay.
 */
static int link_inputs(const request_t *r, plan_context_t *ctx, plan_t *plan)
{
    command_t *cmd;
    char path[ACC_PATH];
    const char *output;
    const char *suffix;
    const char *runtime;
    const char *startup;
    const char *name;
    const char *comma;
    char overlay_output[ACC_PATH];
    char overlay_guest[ACC_PATH];
    char overlay_code[5];       /* validated: at most 4 hex digits */
    char overlay_data[5];
    int overlay_root;
    int overlay;
    int i;
    input_kind_t kind;
    int object;
    char product[16];
    char table[16];
    raw_context_t raw;

    output = r->output == 0 ? profile_default_executable(plan->target) : r->output;
    suffix = strrchr(path_basename(output), '.');
    overlay_root = r->overlay_root != 0;
    overlay = r->overlay_table != 0;
    /* A DOS .com program cannot use overlays (Tech Info 2.1). */
    if (overlay_root && path_equal(suffix, ".com"))
        return fail("a .com program cannot use overlays", output);
    if (overlay && !path_equal(suffix, ".ovr"))
        return fail("an overlay output needs the .ovr suffix", output);
    /* Only DOS 2 .exe programs can use large models (Tech Info 1.1.1). */
    if (r->model != MODEL_SMALL &&
        (plan->target != TARGET_DOS2 || !path_equal(suffix, ".exe")))
        return fail("non-small model requires a dos2 .exe output", output);
    /* The product keeps the output suffix: ln selects the format from it. */
    if (!temp_name(plan, suffix + 1, product) ||
        !output_path(ctx, plan, output, product))
        return 0;
    if (r->debug) {
        /* ln -g writes the sdb information beside its output, as .dbg. */
        strcpy(table, product);
        strcpy(strrchr(table, '.'), ".dbg");
        if (!sibling_output(overlay_output, output, ".dbg") ||
            !output_path(ctx, plan, overlay_output, table))
            return 0;
    }
    if (overlay_root) {
        /* ln -r writes the symbol table beside its output, as .rsm. */
        strcpy(table, product);
        strcpy(strrchr(table, '.'), ".rsm");
        if (!sibling_output(overlay_output, output, ".rsm") ||
            !output_path(ctx, plan, overlay_output, table))
            return 0;
        /* args.c validated CODE,DATA: two hex values of 1 to 4 digits. */
        comma = strchr(r->overlay_root, ',');
        memcpy(overlay_code, r->overlay_root, (size_t)(comma - r->overlay_root));
        overlay_code[comma - r->overlay_root] = '\0';
        strcpy(overlay_data, comma + 1);
    }
    cmd = tool_command(plan, ctx, "ln.exe");
    if (cmd == 0 || !add_word(&cmd->arguments, "-o") ||
        !add_word(&cmd->arguments, product))
        return 0;
    cmd->product = copy_text(product);
    cmd->source = copy_text(output);
    if (cmd->product == 0 || cmd->source == 0)
        return 0;
    if (overlay_root &&
        (!add_word(&cmd->arguments, "+c") ||
         !add_word(&cmd->arguments, overlay_code) ||
         !add_word(&cmd->arguments, "+d") ||
         !add_word(&cmd->arguments, overlay_data)))
        return 0;
    if (overlay_root && !add_word(&cmd->arguments, "-r"))
        return 0;
    raw.assembly_stage = 0;
    raw.large_data = r->model == MODEL_COMPACT || r->model == MODEL_LARGE;
    raw.dos_link = plan->target != TARGET_CPM86;
    raw.dos2_exe = plan->target == TARGET_DOS2 && path_equal(suffix, ".exe");
    for (i = 0; i < r->ln_options.count; ++i) {
        if (!raw_append(&cmd->arguments, RAW_LINKER, r->ln_options.items[i], &raw))
            return 0;
    }
    /* -g applies to every file listed after it: startup, inputs, libraries. */
    if (r->debug && !add_word(&cmd->arguments, "-g"))
        return 0;
    runtime = profile_runtime(plan->target, r->model);
    startup = profile_startup(plan->target, r->model);
    if (!overlay_root && !overlay && !r->nostart && !r->nostdlib) {
        if (!stock(ctx, startup, path) || !mapped(&cmd->arguments, ctx, "", path))
            return 0;
    }
    object = 0;
    for (i = 0; i < ctx->inputs.count; ++i) {
        name = ctx->inputs.items[i];
        kind = classify(name);
        if (is_source(kind)) {
            if (!add_word(&cmd->arguments, ctx->objects.items[object++]))
                return 0;
        } else if (kind == INPUT_LIBNAME) {
            name += 2;
            /* -lm selects the math family matching -mfloat and -mcmodel. */
            if (strcmp(name, "m") == 0) {
                if (!stock(ctx, profile_math(r->floating, r->model), path))
                    return 0;
            } else {
                /* The runtime is implied by the profile and linked last. */
                if (profile_is_runtime(name))
                    return fail("select the target/model instead of an explicit stock C runtime", name);
                if (!user_library(ctx, name, path))
                    return 0;
            }
            if (!mapped(&cmd->arguments, ctx, "", path))
                return 0;
        } else if (!mapped(&cmd->arguments, ctx, "", name)) {
            return 0;
        }
    }
    if (overlay_root &&
        (!stock(ctx, "ovld.o", path) || !mapped(&cmd->arguments, ctx, "", path)))
        return 0;
    if (overlay) {
        suffix = strrchr(path_basename(r->overlay_table), '.');
        if (suffix == 0 || !path_equal(suffix, ".rsm") ||
            !path_readable(r->overlay_table) ||
            !path_guest(&ctx->map, r->overlay_table, overlay_guest))
            return fail("overlay root input must end in .rsm", r->overlay_table);
        if (!mapped(&cmd->arguments, ctx, "", r->overlay_table) ||
            !stock(ctx, "ovbgn.o", path) ||
            !mapped(&cmd->arguments, ctx, "", path))
            return 0;
    }
    if (!overlay && !r->nostdlib && (!stock(ctx, runtime, path) ||
                        !mapped(&cmd->arguments, ctx, "", path)))
        return 0;
    return 1;
}

/*
 * Check that every command can be passed to DOS unchanged.  Arguments with
 * blanks or control characters are refused: DOS has no reliable quoting.
 * Every linker command is moved into a response file (the next workspace
 * temporary) and run as "ln -f FILE", so link size never depends on the
 * DOS command tail.  cc and as have no response files: a compiler command
 * longer than the tail is an error.
 */
static int serialize_commands(plan_t *plan)
{
    char name[16];
    char message[128];
    command_t *cmd;
    const unsigned char *p;
    size_t length;
    int i;

    for (cmd = plan->first; cmd != 0; cmd = cmd->next) {
        length = 0;
        for (i = 1; i < cmd->arguments.count; ++i) {
            p = (const unsigned char *)cmd->arguments.items[i];
            for (; *p != '\0'; ++p) {
                if (*p <= 32 || *p == 127)
                    return fail("backend argument cannot be represented without unverified DOS quoting",
                                cmd->arguments.items[i]);
            }
            length += strlen(cmd->arguments.items[i]) + 1;
        }
        if (!path_equal(path_basename(cmd->arguments.items[0]), "ln.exe")) {
            if (length > DOS_TAIL_LIMIT) {
                sprintf(message, "compiler command line too long (%u > %d bytes; "
                        "reduce -I/-D or shorten paths)", (unsigned)length,
                        DOS_TAIL_LIMIT);
                return fail(message, cmd->source);
            }
            continue;
        }
        for (i = 0; i < cmd->arguments.count; ++i) {
            if (!add_word(&cmd->response, cmd->arguments.items[i]))
                return 0;
        }
        free_words(&cmd->arguments);
        if (!add_word(&cmd->arguments, cmd->response.items[0]) ||
            !add_word(&cmd->arguments, "-f") ||
            !temp_name(plan, "rsp", name) ||
            (cmd->response_file = copy_text(name)) == 0 ||
            !add_word(&cmd->arguments, name))
            return 0;
    }
    return 1;
}

/*
 * Validate R, then plan: one command per source in input order, the linker
 * in link mode, and command serialisation.  The plan_context_t is heap-allocated
 * because its fixed path buffers would not fit a 16-bit DOS stack.
 */
int build_plan(const request_t *r, plan_t *plan)
{
    plan_context_t *ctx;
    int i;
    int ok;
    input_kind_t kind;
    char guest[ACC_PATH];

    ctx = calloc(1, sizeof(plan_context_t));
    if (ctx == 0)
        return fail("out of memory", NULL);
    ok = validate(r, ctx);
    plan->target = r->target;
    plan->target_known = r->target_given;
    if (ok && r->stage == STAGE_LINK)
        ok = resolve_target(r, plan);
    if (ok) {
        strcpy(plan->root, ctx->map.root);
        strcpy(plan->cwd, ctx->map.cwd);
        for (i = 0; ok && i < ctx->libraries.count; ++i)
            ok = path_guest(&ctx->map, ctx->libraries.items[i], guest) &&
                add_word(&plan->library_paths, guest);
    }
    for (i = 0; ok && i < ctx->inputs.count; ++i) {
        kind = classify(ctx->inputs.items[i]);
        if (is_source(kind))
            ok = compile_input(r, ctx, plan, ctx->inputs.items[i], kind);
    }
    if (ok && r->stage == STAGE_LINK)
        ok = link_inputs(r, ctx, plan);
    if (ok)
        ok = serialize_commands(plan);
    free_words(&ctx->includes);
    free_words(&ctx->libraries);
    free_words(&ctx->objects);
    free_words(&ctx->inputs);
    free_words(&ctx->sources);
    free(ctx);
    return ok;
}

/* Release every command, list and string owned by PLAN. */
void free_plan(plan_t *plan)
{
    command_t *cmd;
    command_t *next;

    for (cmd = plan->first; cmd != 0; cmd = next) {
        next = cmd->next;
        free_words(&cmd->arguments);
        free_words(&cmd->response);
        free(cmd->response_file);
        free(cmd->product);
        free(cmd->source);
        free(cmd);
    }
    free_words(&plan->outputs);
    free_words(&plan->products);
    free_words(&plan->host_outputs);
    free_words(&plan->library_paths);
}
