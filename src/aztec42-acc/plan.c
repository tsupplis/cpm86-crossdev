#ifndef ACC_DOS
#define _XOPEN_SOURCE 700
#endif
#include "acc.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef ACC_DOS
#include <dos.h>
#include <stat.h>
extern int _sys(int function, char *argument, int extra);
#define SEP '\\'
#else
#include <sys/stat.h>
#include <unistd.h>
#define SEP '/'
#endif

typedef struct {
    char root[ACC_PATH];
    char cwd[ACC_PATH];
    Words includes;
    Words libraries;
    Words objects;
    Words sources;
} Context;

static int canonical(char *out, const char *path, int output);

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

static int join_path(char *out, const char *parent, const char *name)
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

static int directory(const char *path)
{
    struct stat status;
#ifdef ACC_DOS
    char cwd[ACC_PATH];
    char normalized[ACC_PATH];

    if (!canonical(normalized, path, 0))
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

static int readable(const char *path)
{
    FILE *stream;

    if (directory(path))
        return 0;
    stream = fopen(path, "r");
    if (stream == NULL)
        return 0;
    return fclose(stream) == 0;
}

static int canonical(char *out, const char *path, int output)
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
        if (!join_path(combined, root, path + 1))
            return 0;
    } else {
        if (_sys(0x47, cwd, drive + 1) == -1)
            return fail("cannot determine DOS working directory", root);
        if (!join_path(out, root, cwd) || !join_path(combined, out, path))
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
        if (!directory(combined))
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
    if (resolved != NULL) {
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
    if (slash == NULL) {
        resolved = realpath(".", NULL);
    } else {
        *slash = '\0';
        resolved = realpath(parent[0] == '\0' ? "/" : parent, NULL);
    }
    if (resolved == NULL)
        return fail("output parent does not exist", path);
    ok = join_path(out, resolved, slash == NULL ? path : slash + 1);
    free(resolved);
    return ok;
#endif
}

#ifndef ACC_DOS
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

static int guest_path(const Context *ctx, const char *host, char *out)
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

    if (!canonical(resolved, host, 1))
        return 0;
#ifdef ACC_DOS
    (void)ctx;
    strcpy(out, resolved);
#else
    relative = within(resolved, ctx->root);
    if (relative != NULL && strlen(ctx->root) >= strlen(ctx->cwd)) {
        strcpy(out, "D:\\");
    } else {
        relative = within(resolved, ctx->cwd);
        if (relative != NULL) {
            strcpy(out, "C:\\");
        } else {
            relative = within(resolved, ctx->root);
            if (relative == NULL)
                return fail("path lies outside invocation directory and Aztec root", host);
            strcpy(out, "D:\\");
        }
    }
    if (strlen(relative) + 4 > ACC_PATH)
        return fail("DOS path is too long", host);
    if (strchr(relative, '\\') != NULL)
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
                strchr("\";,:*?<>|+=[]", *p) != NULL)
                return fail("path cannot be represented safely in DOS", host);
            if (dot)
                ++ext;
            else
                ++base;
            if (base > 8 || ext > 3)
                return fail("path requires unsupported DOS filename aliasing", host);
        }
    }
    if (strlen(out) > 63)
        return fail("path exceeds the supported DOS path limit (63 characters)", host);
    return dos_component(component, base, ext, dot, host);
}

static int equal_name(const char *a, const char *b)
{
    while (*a != '\0' && *b != '\0') {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
            return 0;
        ++a;
        ++b;
    }
    return *a == *b;
}

static const char *basename_of(const char *path)
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

static int classify(const char *path)
{
    const char *dot;

    if (strncmp(path, "-l", 2) == 0)
        return 4;
    dot = strrchr(basename_of(path), '.');
    if (dot == 0)
        return -1;
    if (equal_name(dot, ".c"))
        return 0;
    if (equal_name(dot, ".asm") || equal_name(dot, ".s"))
        return 1;
    if (equal_name(dot, ".o"))
        return 2;
    if (equal_name(dot, ".lib"))
        return 3;
    return -1;
}

static int env_paths(Words *paths, const char *name)
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

static int paths(Context *ctx, Words *dest, const Words *user,
                 const char *variable, const char *subdir)
{
    char buffer[ACC_PATH];
    char guest[ACC_PATH];
    int i;

    for (i = 0; i < user->count; ++i) {
        if (!add_word(dest, user->items[i]))
            return 0;
    }
    if (!env_paths(dest, variable) || !join_path(buffer, ctx->root, subdir) ||
        !add_word(dest, buffer))
        return 0;
    for (i = 0; i < dest->count; ++i) {
        if (!directory(dest->items[i]))
            return fail("search directory does not exist", dest->items[i]);
        if (!guest_path(ctx, dest->items[i], guest))
            return 0;
    }
    return 1;
}

static Command *command(Plan *plan, const char *tool)
{
    Command *result;

    result = calloc(1, sizeof(Command));
    if (result == NULL) {
        fail("out of memory", NULL);
        return NULL;
    }
    if (plan->last == NULL)
        plan->first = result;
    else
        plan->last->next = result;
    plan->last = result;
    if (!add_word(&result->arguments, tool))
        return NULL;
    return result;
}

static Command *tool_command(Plan *plan, const Context *ctx, const char *name)
{
    char bindir[ACC_PATH];
    char path[ACC_PATH];
    char guest[ACC_PATH];

    if (!join_path(bindir, ctx->root, "bin") ||
        !join_path(path, bindir, name) || !guest_path(ctx, path, guest))
        return NULL;
    return command(plan, guest);
}

static int option(Words *args, const char *prefix, const char *value)
{
    char buffer[ACC_PATH];

    if (strlen(prefix) + strlen(value) >= sizeof(buffer))
        return fail("backend argument is too long", value);
    strcpy(buffer, prefix);
    strcat(buffer, value);
    return add_word(args, buffer);
}

static int mapped(Words *args, const Context *ctx,
                  const char *prefix, const char *host)
{
    char guest[ACC_PATH];

    return guest_path(ctx, host, guest) && option(args, prefix, guest);
}

static int stock(Context *ctx, const char *name, char *host)
{
    char libdir[ACC_PATH];

    if (!join_path(libdir, ctx->root, "lib") || !join_path(host, libdir, name))
        return 0;
    if (!readable(host))
        return fail("missing installation library or startup", host);
    return 1;
}

static int user_library(Context *ctx, const char *name, char *host)
{
    char filename[ACC_PATH];
    int i;

    if (*name == '\0' || strpbrk(name, "/\\:.") != NULL)
        return fail("invalid library name", name);
    if (strlen(name) + 5 >= sizeof(filename))
        return fail("library name is too long", name);
    strcpy(filename, name);
    strcat(filename, ".lib");
    for (i = 0; i < ctx->libraries.count; ++i) {
        if (!join_path(host, ctx->libraries.items[i], filename))
            return 0;
        if (readable(host))
            return 1;
    }
    return fail("library not found", name);
}

static int output_path(Context *ctx, Plan *plan, const char *host,
                       const char *product, const Request *r)
{
    char guest[ACC_PATH];
    char source[ACC_PATH];
    char resolved[ACC_PATH];
    int i;
#ifndef ACC_DOS
    struct stat destination;
    struct stat input;
#endif

    if (directory(host))
        return fail("output is a directory", host);
    if (!guest_path(ctx, host, guest))
        return 0;
    for (i = 0; i < plan->outputs.count; ++i) {
        if (equal_name(plan->outputs.items[i], guest))
            return fail("output collision", host);
    }
    for (i = 0; i < r->inputs.count; ++i) {
        if (classify(r->inputs.items[i]) == 4)
            continue;
        if (!guest_path(ctx, r->inputs.items[i], source))
            return 0;
        if (equal_name(source, guest))
            return fail("output aliases an input", host);
#ifndef ACC_DOS
        if (stat(host, &destination) == 0 &&
            stat(r->inputs.items[i], &input) == 0 &&
            destination.st_dev == input.st_dev && destination.st_ino == input.st_ino)
            return fail("output aliases an input", host);
#endif
    }
    return canonical(resolved, host, 1) &&
        add_word(&plan->outputs, guest) && add_word(&plan->products, product) &&
        add_word(&plan->host_outputs, resolved);
}

static int overlay_table_output(char *out, const char *output)
{
    const char *base;
    char *suffix;

    base = basename_of(output);
    suffix = strrchr(output, '.');
    if (suffix == NULL || suffix < base)
        return fail("overlay root output requires a filename suffix", output);
    if ((size_t)(suffix - output) + 5 >= ACC_PATH)
        return fail("overlay root output path is too long", output);
    memcpy(out, output, (size_t)(suffix - output));
    strcpy(out + (suffix - output), ".rsm");
    return 1;
}

static int required_tool(const Context *ctx, const char *name)
{
    char path[ACC_PATH];

    if (!join_path(path, ctx->root, name))
        return 0;
    if (!readable(path))
        return fail("required backend tool is missing", path);
    return 1;
}

static int validate(const Request *r, Context *ctx)
{
    char guest[ACC_PATH];
    const char *root;
    const char *macro;
    int i;
    int j;
    int type;
    int count;

    root = r->root;
    if (root == 0 || *root == '\0')
        root = getenv("ACCROOT");
    if (root == 0 || *root == '\0')
        return fail("set -toolroot or ACCROOT; executable-relative discovery is not implemented yet", NULL);
    if (!directory(root) || !canonical(ctx->root, root, 0))
        return fail("invalid Aztec installation root", root);
#ifndef ACC_DOS
    if (getcwd(ctx->cwd, sizeof(ctx->cwd)) == NULL)
        return fail("cannot determine working directory", NULL);
#else
    if (!canonical(ctx->cwd, ".", 0))
        return 0;
#endif
    if (!paths(ctx, &ctx->includes, &r->includes, "INCLUDE", "include") ||
        !paths(ctx, &ctx->libraries, &r->libraries, "CLIB", "lib"))
        return 0;
    if (ctx->includes.count > 7)
        return fail("too many include areas for conservative Aztec limit (7 plus current directory)", NULL);
    if (getenv("AZTEC_STARTUP") != NULL || getenv("AZTEC_NOSTARTUP") != NULL)
        return fail("legacy AZTEC_STARTUP/AZTEC_NOSTARTUP settings are not supported", NULL);
    count = 0;
    for (i = 0; i < r->inputs.count; ++i) {
        type = classify(r->inputs.items[i]);
        if (type < 0)
            return fail("unsupported input suffix", r->inputs.items[i]);
        if (r->stage != 0 && (type > 1 || (r->stage == 2 && type != 0)))
            return fail("input is incompatible with selected stage", r->inputs.items[i]);
        if (type != 4) {
            if (!readable(r->inputs.items[i]))
                return fail("cannot read input", r->inputs.items[i]);
            if (!guest_path(ctx, r->inputs.items[i], guest))
                return 0;
            if (type < 3) {
                for (j = 0; j < ctx->sources.count; ++j) {
                    if (equal_name(ctx->sources.items[j], guest))
                        return fail("duplicate input", r->inputs.items[i]);
                }
                if (!add_word(&ctx->sources, guest))
                    return 0;
            }
        }
        if (type < 3)
            ++count;
        if (type == 1 && (r->cpu == 2 || r->floating == 2))
            return fail("explicit assembly CPU/coprocessor mapping is not qualified yet", NULL);
    }
    if (count == 0)
        return fail("at least one source or object input is required", NULL);
    if (r->stage != 0 && r->output != NULL && count != 1)
        return fail("one -o cannot name multiple compilation outputs", NULL);
    for (i = 0; i < r->definitions.count; ++i) {
        macro = r->definitions.items[i];
        if (strncmp(macro, "__CPM86__=", 10) == 0) {
            if (r->target != 0 || strcmp(macro + 10, "1") != 0)
                return fail("conflicting target macro", macro);
        }
        if (strncmp(macro, "__DOS__=", 8) == 0) {
            if (r->target == 0 || strcmp(macro + 8, "1") != 0)
                return fail("conflicting target macro", macro);
        }
    }
    /* Verify every required tool before printing any command. */
    for (i = 0; i < r->inputs.count; ++i) {
        type = classify(r->inputs.items[i]);
        if (type > 1)
            continue;
        if (!required_tool(ctx, type == 0 ? "bin/cc.exe" : "bin/as.exe"))
            return 0;
        if (type == 0) {
            if (!required_tool(ctx, "bin/cgen.exe"))
                return 0;
            if (r->stage != 2 && !required_tool(ctx, "bin/as.exe"))
                return 0;
        }
    }
    if (r->stage == 0 && !required_tool(ctx, "bin/ln.exe"))
        return 0;
    if (r->overlay_root != NULL && !required_tool(ctx, "lib/ovld.o"))
        return 0;
    if (r->overlay_table != NULL && !required_tool(ctx, "lib/ovbgn.o"))
        return 0;
    return 1;
}

static int compile_input(const Request *r, Context *ctx, Plan *plan,
                         const char *input, int index, int type)
{
    static const char *models[] = { "", "+lc", "+ld", "+l" };
    Command *cmd;
    Words *args;
    char product[32];
    char output[ACC_PATH];
    char *dot;
    int i;

    sprintf(product, "E:\\u%04d.%s", index, r->stage == 2 ? "asm" : "o");
    cmd = tool_command(plan, ctx, type == 0 ? "cc.exe" : "as.exe");
    if (cmd == NULL)
        return 0;
    cmd->product = copy_text(product);
    cmd->source = copy_text(input);
    if (cmd->product == NULL || cmd->source == NULL)
        return 0;
    args = &cmd->arguments;
    if (type == 0) {
        if (!add_word(args, r->cpu == 0 ? "+0" : (r->cpu == 1 ? "+1" : "+2")) ||
            !add_word(args, "-b") || !add_word(args, "-n"))
            return 0;
        if (r->model != 0 && !add_word(args, models[r->model]))
            return 0;
        if (r->floating == 2 && !add_word(args, "+e"))
            return 0;
        if (r->language != 0 && !add_word(args, r->language == 1 ? "-ansi" : "-3"))
            return 0;
        if (r->optimize != 0 && !add_word(args, r->optimize == 1 ? "+c" : "+f"))
            return 0;
        if (r->quiet && !add_word(args, "-s"))
            return 0;
        if (r->stage == 2 && !add_word(args, "-A"))
            return 0;
    } else if (r->cpu == 1 && !add_word(args, "-186")) {
        return 0;
    }
    if (!add_word(args, r->target == 0 ? "-D__CPM86__=1" : "-D__DOS__=1"))
        return 0;
    for (i = 0; i < r->definitions.count; ++i) {
        if (type == 1)
            return fail("explicit assembly macro validation is not implemented yet", NULL);
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
    if (r->stage == 0)
        return add_word(&ctx->objects, product);
    if (r->output != NULL) {
        strcpy(output, r->output);
    } else {
        strcpy(output, basename_of(input));
        dot = strrchr(output, '.');
        *dot = '\0';
        strcat(output, r->stage == 2 ? ".asm" : ".o");
    }
    return output_path(ctx, plan, output, product, r);
}

static int link_inputs(const Request *r, Context *ctx, Plan *plan)
{
    static const char *runtimes[] = { "c.lib", "clc.lib", "cld.lib", "cl.lib" };
    static const char *startups[] = { "sbegin.o", "beginlc.o", "beginld.o", "lbegin.o" };
    static const char *maths[3][4] = {
        { "m.lib", "mlc.lib", "mld.lib", "ml.lib" },
        { "m87s.lib", "m87slc.lib", "m87sld.lib", "m87sl.lib" },
        { "m87.lib", "m87lc.lib", "m87ld.lib", "m87l.lib" }
    };
    Command *cmd;
    char path[ACC_PATH];
    const char *output;
    const char *suffix;
    const char *runtime;
    const char *startup;
    const char *name;
    const char *comma;
    char overlay_output[ACC_PATH];
    char overlay_guest[ACC_PATH];
    char overlay_code[ACC_PATH];
    char overlay_data[ACC_PATH];
    int overlay_root;
    int overlay;
    int i;
    int type;
    int object;

    output = r->output == NULL ? (r->target == 0 ? "a.cmd" : "a.exe") : r->output;
    suffix = strrchr(basename_of(output), '.');
    overlay_root = r->overlay_root != NULL;
    overlay = r->overlay_table != NULL;
    if (suffix == 0 ||
        (overlay_root && !equal_name(suffix, ".cmd")) ||
        (overlay && !equal_name(suffix, ".ovr")) ||
        (!overlay_root && !overlay && r->target == 0 && !equal_name(suffix, ".cmd")) ||
        (!overlay_root && !overlay && r->target != 0 &&
         !equal_name(suffix, ".exe") && !equal_name(suffix, ".com")))
        return fail("output suffix conflicts with target", output);
    if (!overlay_root && !overlay && equal_name(suffix, ".com") && r->model != 0)
        return fail("COM output requires small model", output);
    if (!output_path(ctx, plan, output,
                     equal_name(suffix, ".cmd") ? "E:\\result.cmd" :
                     (equal_name(suffix, ".ovr") ? "E:\\result.ovr" :
                      (equal_name(suffix, ".com") ? "E:\\result.com" : "E:\\result.exe")),
                     r))
        return 0;
    if (overlay_root) {
        if (!overlay_table_output(overlay_output, output) ||
            !output_path(ctx, plan, overlay_output, "E:\\result.rsm", r))
            return 0;
        comma = strchr(r->overlay_root, ',');
        if (comma == NULL)
            return fail("invalid overlay root sizes", r->overlay_root);
        if ((size_t)(comma - r->overlay_root) >= sizeof(overlay_code) ||
            strlen(comma + 1) >= sizeof(overlay_data))
            return fail("overlay root size is too long", r->overlay_root);
        memcpy(overlay_code, r->overlay_root, (size_t)(comma - r->overlay_root));
        overlay_code[comma - r->overlay_root] = '\0';
        strcpy(overlay_data, comma + 1);
    }
    cmd = tool_command(plan, ctx, "ln.exe");
    if (cmd == NULL || !add_word(&cmd->arguments, "-o") ||
        !add_word(&cmd->arguments, plan->products.items[0]))
        return 0;
    cmd->product = copy_text(plan->products.items[0]);
    cmd->source = copy_text(output);
    if (cmd->product == NULL || cmd->source == NULL)
        return 0;
    if (overlay_root &&
        (!add_word(&cmd->arguments, "+c") ||
         !add_word(&cmd->arguments, overlay_code) ||
         !add_word(&cmd->arguments, "+d") ||
         !add_word(&cmd->arguments, overlay_data)))
        return 0;
    if (overlay_root && !add_word(&cmd->arguments, "-r"))
        return 0;
    runtime = r->target == 0 ? "c86.lib" : (r->target == 1 ? "d11.lib" : runtimes[r->model]);
    startup = r->target == 0 ? "begin86.o" : (r->target == 1 ? "begin11.o" : startups[r->model]);
    if (!overlay_root && !overlay && !r->nostart && !r->nostdlib) {
        if (!stock(ctx, startup, path) || !mapped(&cmd->arguments, ctx, "", path))
            return 0;
    }
    object = 0;
    for (i = 0; i < r->inputs.count; ++i) {
        name = r->inputs.items[i];
        type = classify(name);
        if (type < 2) {
            if (!add_word(&cmd->arguments, ctx->objects.items[object++]))
                return 0;
        } else if (type == 4) {
            name += 2;
            if (strcmp(name, "m") == 0) {
                if (!stock(ctx, maths[r->floating][r->model], path))
                    return 0;
            } else {
                if (equal_name(name, "c") || equal_name(name, "c86") ||
                    equal_name(name, "d11") || equal_name(name, "cl") ||
                    equal_name(name, "clc") || equal_name(name, "cld"))
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
        suffix = strrchr(basename_of(r->overlay_table), '.');
        if (suffix == NULL || !equal_name(suffix, ".rsm") ||
            !readable(r->overlay_table) ||
            !guest_path(ctx, r->overlay_table, overlay_guest))
            return fail("overlay root input must end in .rsm", r->overlay_table);
        if (equal_name(overlay_guest, plan->outputs.items[0]))
            return fail("overlay output aliases its root symbol table", r->overlay_table);
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

static int serialize_commands(Plan *plan)
{
    Command *cmd;
    const unsigned char *p;
    size_t length;
    int i;

    for (cmd = plan->first; cmd != NULL; cmd = cmd->next) {
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
        if (length <= 126)
            continue;
        if (!equal_name(basename_of(cmd->arguments.items[0]), "ln.exe"))
            return fail("compiler/assembler command exceeds the verified DOS command-tail limit",
                        cmd->source);
        for (i = 0; i < cmd->arguments.count; ++i) {
            if (!add_word(&cmd->response, cmd->arguments.items[i]))
                return 0;
        }
        free_words(&cmd->arguments);
        if (!add_word(&cmd->arguments, cmd->response.items[0]) ||
            !add_word(&cmd->arguments, "-f") ||
            !add_word(&cmd->arguments, "E:\\link.rsp"))
            return 0;
    }
    return 1;
}

int build_plan(const Request *r, Plan *plan)
{
    Context *ctx;
    int i;
    int ok;
    int type;
    char guest[ACC_PATH];

    ctx = calloc(1, sizeof(Context));
    if (ctx == NULL)
        return fail("out of memory", NULL);
    ok = validate(r, ctx);
    if (ok) {
        strcpy(plan->root, ctx->root);
        strcpy(plan->cwd, ctx->cwd);
        for (i = 0; ok && i < ctx->libraries.count; ++i)
            ok = guest_path(ctx, ctx->libraries.items[i], guest) &&
                add_word(&plan->library_paths, guest);
    }
    for (i = 0; ok && i < r->inputs.count; ++i) {
        type = classify(r->inputs.items[i]);
        if (type < 2)
            ok = compile_input(r, ctx, plan, r->inputs.items[i], i + 1, type);
    }
    if (ok && r->stage == 0)
        ok = link_inputs(r, ctx, plan);
    if (ok)
        ok = serialize_commands(plan);
    free_words(&ctx->includes);
    free_words(&ctx->libraries);
    free_words(&ctx->objects);
    free_words(&ctx->sources);
    free(ctx);
    return ok;
}

static void quoted(const char *text)
{
    fputc('"', stderr);
    while (*text != '\0') {
        if (*text == '"' || *text == '\\')
            fputc('\\', stderr);
        if ((unsigned char)*text < 32)
            fprintf(stderr, "\\x%02x", (unsigned int)(unsigned char)*text);
        else
            fputc(*text, stderr);
        ++text;
    }
    fputc('"', stderr);
}

void print_plan(const Request *r, const Plan *plan)
{
    static const char *targets[] = { "cpm86", "dos1", "dos2" };
    static const char *models[] = { "small", "medium", "compact", "large" };
    const Command *cmd;
    int i;

    fprintf(stderr, "acc: plan target=%s model=%s cpu=i%d86\n",
            targets[r->target], models[r->model], r->cpu == 0 ? 80 : (r->cpu == 1 ? 801 : 802));
    fputs(r->dryrun ?
          "acc: E: is a symbolic private workspace; no files are created\n" :
          "acc: E: maps the invocation's private workspace\n", stderr);
    for (cmd = plan->first; cmd != 0; cmd = cmd->next) {
        fputs("tool:", stderr);
        for (i = 0; i < cmd->arguments.count; ++i) {
            fputc(' ', stderr);
            quoted(cmd->arguments.items[i]);
        }
        fputc('\n', stderr);
        if (cmd->response.count != 0) {
            fputs("response: \"E:\\\\link.rsp\"", stderr);
            for (i = 1; i < cmd->response.count; ++i) {
                fputc(' ', stderr);
                quoted(cmd->response.items[i]);
            }
            fputc('\n', stderr);
        }
    }
    for (i = 0; i < plan->outputs.count; ++i) {
        fputs("publish: ", stderr);
        quoted(plan->products.items[i]);
        fputs(" -> ", stderr);
        quoted(plan->outputs.items[i]);
        fputc('\n', stderr);
    }
}

void free_plan(Plan *plan)
{
    Command *cmd;
    Command *next;

    for (cmd = plan->first; cmd != NULL; cmd = next) {
        next = cmd->next;
        free_words(&cmd->arguments);
        free_words(&cmd->response);
        free(cmd->product);
        free(cmd->source);
        free(cmd);
    }
    free_words(&plan->outputs);
    free_words(&plan->products);
    free_words(&plan->host_outputs);
    free_words(&plan->library_paths);
}
