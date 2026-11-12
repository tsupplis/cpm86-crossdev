/*
 * Native DOS execution of a plan_t (the Aztec C 4.2 build of acc).
 *
 * The plan's symbolic E: drive becomes a private workspace directory.
 * Every backend tool is started with fexecv/wait and receives its own
 * environment block (PATH, INCLUDE, CLIB, CCTEMP) built from the parent's.
 * Products are verified after each tool, then published with a staged
 * copy and backup/restore: DOS rename cannot replace an existing file.
 * The workspace is removed unless -save-temps is given.
 */
#include "acc.h"
#include "request.h"
#include "plan.h"
#include "publish.h"
#include "cmdhdr.h"
#include <dos.h>
#include <fcntl.h>
#include <signal.h>
#include <stat.h>
#include <stdlib.h>
#include <string.h>

/* Longest DOS command tail: DOS and emu2 silently drop what follows. */


/* Size of a DOS path buffer: 63 characters plus the terminator. */
#define ACC_DOS_PATH 64

/* Aztec C runtime symbols without a declaring header. */
extern unsigned _PSP;               /* segment of this program's PSP */
extern int fexecv(const char *, char **);
extern int wait(void);
extern int mkdir(const char *);
extern char *mktemp(char *);
/* Next file matching a wildcard pattern (DOS find first/next); 0 at end. */
extern char *scdir(char *);

/* Set by the SIGINT (Ctrl-C/Ctrl-Break) handler; stops further commands. */
static int interrupted;

/* Print an I/O diagnostic naming PATH; always returns 0. */
static int dos_error(const char *message, const char *path)
{
    fprintf(stderr, "acc: %s: %s\n", message, path);
    return 0;
}

/* OUT = PARENT\NAME, within the DOS path limit; 0 after a diagnostic. */
static int dos_join(char *out, const char *parent, const char *name)
{
    size_t size;

    size = strlen(parent);
    if (size + strlen(name) + 2 > ACC_DOS_PATH)
        return fail("DOS path is too long", name);
    strcpy(out, parent);
    if (size != 0 && out[size - 1] != '\\' && out[size - 1] != '/')
        strcat(out, "\\");
    strcat(out, name);
    return 1;
}

/* Far pointer to SEGMENT:OFFSET (the PSP and environment blocks). */
static char far *dos_far(unsigned segment, unsigned offset)
{
    unsigned long address;

    address = ((unsigned long)segment << 16) | offset;
    return (char far *)address;
}

/* Non-zero if environment ENTRY ("NAME=value") defines NAME, ignoring case. */
static int env_name_equal(const char far *entry, const char *name)
{
    size_t i;
    unsigned char a;
    unsigned char b;

    for (i = 0; name[i] != '\0'; ++i) {
        a = (unsigned char)entry[i];
        b = (unsigned char)name[i];
        if (a >= 'a' && a <= 'z')
            a = (unsigned char)(a - 'a' + 'A');
        if (b >= 'a' && b <= 'z')
            b = (unsigned char)(b - 'a' + 'A');
        if (a != b)
            return 0;
    }
    return entry[i] == '=';
}

/* Append VALUE and its terminator at *TARGET, advancing *TARGET. */
static int env_copy(char far **target, const char *value)
{
    while (*value != '\0')
        *(*target)++ = *value++;
    *(*target)++ = '\0';
    return 1;
}

/*
 * Allocate a child environment block in *REPLACEMENT: a copy of the block
 * at segment ORIGINAL without PATH, CLIB, INCLUDE and CCTEMP, followed by
 * PATH=<root>\bin, an empty INCLUDE (all include areas are explicit -I
 * arguments), CLIB=<library areas> (each ending in '\', as ln requires)
 * and CCTEMP=<workspace>.  The block must be released with DOS call 0x49.
 */
static int dos_environment(unsigned original, unsigned *replacement,
                           const plan_t *plan, const char *work)
{
    char far *source;
    char far *target;
    char far *entry;
    char path[ACC_DOS_PATH];
    char clib[ACC_PATH];
    char temp[ACC_DOS_PATH];
    char variable[ACC_DOS_PATH + 8];
    unsigned long bytes;
    unsigned paragraphs;
    int segment;
    int i;

    if (!dos_join(path, plan->root, "bin"))
        return 0;
    if (strlen(path) + 6 >= sizeof(variable))
        return fail("DOS tool path is too long for environment", path);
    strcpy(variable, "PATH=");
    strcat(variable, path);
    strcpy(clib, "CLIB=");
    for (i = 0; i < plan->library_paths.count; ++i) {
        if (strlen(clib) + strlen(plan->library_paths.items[i]) + 3 >= sizeof(clib))
            return fail("DOS library path is too long for environment", NULL);
        if (i != 0)
            strcat(clib, ";");
        strcat(clib, plan->library_paths.items[i]);
        if (clib[strlen(clib) - 1] != '\\')
            strcat(clib, "\\");
    }
    if (strlen(work) + 8 >= sizeof(temp))
        return fail("DOS temporary path is too long for environment", work);
    strcpy(temp, "CCTEMP=");
    strcat(temp, work);
    if (temp[strlen(temp) - 1] != '\\')
        strcat(temp, "\\");

    source = dos_far(original, 0);
    bytes = 0;
    while (bytes < 8190 && (source[bytes] != '\0' || source[bytes + 1] != '\0'))
        ++bytes;
    if (bytes >= 8190)
        return fail("inherited DOS environment is too large", NULL);
    ++bytes;
    bytes += strlen(variable) + 1 + strlen(clib) + 1 +
        strlen("INCLUDE=") + 1 + strlen(temp) + 1 + 2;
    paragraphs = (unsigned)((bytes + 15) / 16);
    segment = dos(0x48, paragraphs, 0, 0, 0, 0);
    if (segment == -1)
        return fail("cannot allocate child environment block", NULL);
    *replacement = (unsigned)segment;
    target = dos_far(*replacement, 0);
    entry = source;
    while (*entry != '\0') {
        if (!env_name_equal(entry, "PATH") &&
            !env_name_equal(entry, "CLIB") &&
            !env_name_equal(entry, "INCLUDE") &&
            !env_name_equal(entry, "CCTEMP")) {
            while (*entry != '\0')
                *target++ = *entry++;
            *target++ = '\0';
        } else {
            while (*entry != '\0')
                ++entry;
            ++entry;
        }
    }
    env_copy(&target, variable);
    env_copy(&target, "INCLUDE=");
    env_copy(&target, clib);
    env_copy(&target, temp);
    *target++ = '\0';
    *target = '\0';
    return 1;
}

/*
 * Create a unique private workspace directory in CCTEMP, TEMP or the
 * invocation directory, and store its path in OUT.  The name must leave
 * room for workspace files within the 63-character DOS path limit.
 */
static int dos_workspace(char *out, const plan_t *plan)
{
    const char *parent;
    char candidate[ACC_DOS_PATH];
    char pattern[ACC_DOS_PATH];
    int attempt;
    size_t base;

    parent = getenv("CCTEMP");
    if (parent == 0 || *parent == '\0')
        parent = getenv("TEMP");
    if (parent == 0 || *parent == '\0')
        parent = plan->cwd;
    base = strlen(parent) + ((parent[0] != '\0' &&
                              parent[strlen(parent) - 1] != '\\') ? 1 : 0);
    if (base + 7 > 63)
        return fail("DOS workspace parent exceeds 63-character path limit", parent);
    if (!dos_join(pattern, parent, "AXXXXXX"))
        return 0;
    base = strlen(pattern) - 7;
    for (attempt = 0; attempt < 26; ++attempt) {
        pattern[base] = (char)('A' + attempt);
        strcpy(candidate, pattern);
        if (mktemp(candidate) == 0 || *candidate == '\0')
            return fail("cannot generate a unique DOS workspace name", pattern);
        if (mkdir(candidate) == 0) {
            strcpy(out, candidate);
            return 1;
        }
    }
    return fail("cannot create a unique DOS workspace", parent);
}

/*
 * Translate workspace guest path GUEST (E:\NAME) to its path in WORK.
 * Anything else is refused, so acc only ever writes or removes files it
 * owns.
 */
static int owned_path(char *out, const char *work, const char *guest)
{
    if (strncmp(guest, "E:\\", 3) != 0 ||
        strchr(guest + 3, '\\') != 0 || strchr(guest + 3, '/') != 0)
        return fail("invalid planned workspace artifact", guest);
    return dos_join(out, work, guest + 3);
}

/* Write CMD's linker response file in WORK, one CRLF-terminated argument
 * per line, with workspace paths translated. */
static int response_file(const command_t *cmd, const char *work)
{
    char path[ACC_DOS_PATH];
    char argument[ACC_DOS_PATH];
    FILE *stream;
    int i;
    int ok;

    if (!owned_path(path, work, cmd->response_file))
        return 0;
    stream = fopen(path, "w");
    if (stream == 0)
        return dos_error("cannot create linker response file", path);
    for (i = 1; i < cmd->response.count; ++i) {
        if (cmd->workspace_arg[i]) {
            if (!owned_path(argument, work, cmd->response.items[i])) {
                fclose(stream);
                return 0;
            }
            fprintf(stream, "%s\r\n", argument);
        } else {
            fprintf(stream, "%s\r\n", cmd->response.items[i]);
        }
    }
    ok = !ferror(stream);
    if (fclose(stream) != 0)
        ok = 0;
    return ok ? 1 : dos_error("cannot write linker response file", path);
}

/*
 * Run CMD's tool from the tool root with environment segment ENVSEG and
 * wait for it.  Workspace arguments are translated to WORK; a command with
 * a response file is run as "TOOL -f FILE".  The PSP environment pointer
 * is swapped only around fexecv, which passes it to the child.
 * Returns the child's exit status, or 1 after a diagnostic.
 */
static int dos_launch(const command_t *cmd, const plan_t *plan, unsigned envseg,
                     const char *work)
{
    char tool[ACC_DOS_PATH];
    char rsp_path[ACC_DOS_PATH];
    char *translated[ACC_LIMIT];
    char *arguments[ACC_LIMIT + 1];
    int translated_count;
    unsigned far *psp;
    unsigned oldenv;
    int i;
    int result;
    int wait_status;

    if (strlen(plan->root) + 13 >= sizeof(tool))
        return fail("DOS tool path is too long", plan->root), 1;
    strcpy(tool, plan->root);
    if (tool[strlen(tool) - 1] != '\\')
        strcat(tool, "\\");
    strcat(tool, "bin\\");
    {
        const char *name = strrchr(cmd->arguments.items[0], '\\');
        if (name == 0) {
            fail("invalid planned DOS tool path", cmd->arguments.items[0]);
            return 1;
        }
        strcat(tool, name + 1);
    }
    arguments[0] = tool;
    translated_count = 0;
    for (i = 1; i < cmd->arguments.count; ++i) {
        if (cmd->response.count == 0 && cmd->workspace_arg[i]) {
            if (translated_count >= ACC_LIMIT) {
                while (translated_count > 0)
                    free(translated[--translated_count]);
                return 1;
            }
            translated[translated_count] = malloc(ACC_DOS_PATH);
            if (translated[translated_count] == 0) {
                while (translated_count > 0)
                    free(translated[--translated_count]);
                fail("out of memory translating DOS arguments", NULL);
                return 1;
            }
            if (!owned_path(translated[translated_count], work,
                            cmd->arguments.items[i])) {
                free(translated[translated_count]);
                while (translated_count > 0)
                    free(translated[--translated_count]);
                return 1;
            }
            arguments[i] = translated[translated_count++];
        } else {
            arguments[i] = cmd->arguments.items[i];
        }
    }
    if (cmd->response.count != 0) {
        if (!owned_path(rsp_path, work, cmd->response_file))
            return 1;
        arguments[1] = "-f";
        arguments[2] = rsp_path;
        arguments[3] = NULL;
    } else {
        arguments[cmd->arguments.count] = NULL;
    }
    {
        unsigned long length;

        length = 0;
        for (i = 1; arguments[i] != 0; ++i) {
            if (strpbrk(arguments[i], " \t\r\n") != 0) {
                fail("translated DOS argument contains whitespace", arguments[i]);
                break;
            }
            length += strlen(arguments[i]) + 1;
        }
        if (arguments[i] != 0 || length > ACC_DOS_TAIL) {
            if (arguments[i] == 0)
                fail("translated DOS command line too long", cmd->source);
            while (translated_count > 0)
                free(translated[--translated_count]);
            return 1;
        }
    }
    psp = (unsigned far *)dos_far(_PSP, 0);
    oldenv = psp[0x2c / 2];
    psp[0x2c / 2] = envseg;
    result = fexecv(tool, arguments);
    psp[0x2c / 2] = oldenv;
    if (result == -1) {
        fprintf(stderr, "acc: cannot execute %s for %s\n", tool, cmd->source);
        while (translated_count > 0)
            free(translated[--translated_count]);
        return 1;
    }
    wait_status = wait();
    if (wait_status < 0) {
        fprintf(stderr, "acc: cannot retrieve child status for %s\n", tool);
        while (translated_count > 0)
            free(translated[--translated_count]);
        return 1;
    }
    while (translated_count > 0)
        free(translated[--translated_count]);
    result = wait_status & 0xff;
    if (result != 0)
        fprintf(stderr, "acc: backend failed: %s for %s (status %d)\n",
                tool, cmd->source, result);
    (void)plan;
    return result;
}

/* Host path in WORK of workspace product GUEST. */
static int dos_product(char *out, const char *work, const char *guest)
{
    return owned_path(out, work, guest);
}

/* Fail unless the tool created PATH as a non-empty file: some Aztec
 * tools report success without writing their output. */
static int dos_verify_product(const char *path)
{
    struct stat status;

    if (stat(path, &status) != 0 || (status.st_attr & ST_DIRECT) != 0 ||
        status.st_size == 0)
        return fail("backend did not create its output", path);
    return 1;
}

/* Aztec mktemp can exhaust a template when two files coexist in the
 * same clock tick. Vary the prefix to stage a whole batch in one directory. */
static int publication_name(char *out, const char *destination,
                            const char *suffix)
{
    char parent[ACC_DOS_PATH];
    char pattern[13];
    char *slash;
    unsigned i;

    strcpy(parent, destination);
    slash = strrchr(parent, '\\');
    if (slash == 0)
        return fail("planned output has no DOS directory", destination);
    *slash = '\0';
    for (i = 0; i < 256; ++i) {
        sprintf(pattern, "%02XXXXXXX.%s", i, suffix);
        if (!dos_join(out, parent, pattern))
            return 0;
        if (mktemp(out) != 0 && *out != '\0')
            return 1;
    }
    return fail("cannot reserve DOS publication name", destination);
}

/* Copy a verified product to a new file beside its destination. */
static int stage_output(char *staged, const char *work, const char *guest,
                        const char *destination)
{
    char source[ACC_DOS_PATH];
    char pattern[ACC_DOS_PATH];
    char buffer[512];
    int in;
    int out;
    int amount;
    int wrote;
    int offset;
    int ok;

    if (!owned_path(source, work, guest) || !dos_verify_product(source))
        return 0;
    in = open(source, O_RDONLY);
    if (in == -1)
        return dos_error("cannot open completed output", source);
    if (!publication_name(pattern, destination, "TMP")) {
        close(in);
        return 0;
    }
    out = open(pattern, O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (out == -1) {
        close(in);
        return dos_error("cannot create staged output", pattern);
    }
    strcpy(staged, pattern);
    ok = 1;
    while ((amount = read(in, buffer, sizeof(buffer))) > 0) {
        offset = 0;
        while (offset < amount) {
            wrote = write(out, buffer + offset, amount - offset);
            if (wrote <= 0) {
                ok = 0;
                break;
            }
            offset += wrote;
        }
        if (!ok)
            break;
    }
    if (amount < 0)
        ok = 0;
    if (close(in) != 0)
        ok = 0;
    if (close(out) != 0)
        ok = 0;
    return ok || dos_error("cannot copy completed output", destination);
}

static int backup_output(char *backup, const char *destination)
{
    char pattern[ACC_DOS_PATH];
    struct stat status;

    if (access(destination, 0) != 0)
        return 1;
    if (stat(destination, &status) != 0 || (status.st_attr & ST_DIRECT) != 0)
        return fail("previous output is not a regular file", destination);
    if (!publication_name(pattern, destination, "BAK") ||
        rename(destination, pattern) != 0)
        return dos_error("cannot preserve previous output", destination);
    strcpy(backup, pattern);
    return 1;
}

static int move_output(const char *from, const char *to)
{
    return rename(from, to) == 0 || dos_error("cannot publish output", to);
}

static int remove_output(const char *path)
{
    return unlink(path) == 0 || dos_error("cannot remove staged output", path);
}

static int restore_output(const char *backup, const char *dest, int installed)
{
    if (installed && !remove_output(dest))
        return 0;
    return move_output(backup, dest);
}

static int publication_cancelled(void)
{
    return interrupted != 0;
}

static const publish_ops_t publication = {
    stage_output, backup_output, move_output, restore_output, remove_output,
    publication_cancelled
};

/*
 * Remove every file in workspace WORK, then WORK itself.  Enumerating the
 * directory, rather than removing only the files acc planned, also removes
 * temporaries left by cc, cgen or as (CCTEMP points into WORK), so an
 * invocation never leaves its workspace behind.  WORK has no
 * subdirectories: acc and the tools only create plain files there.
 *
 * scdir yields paths with the directory of the pattern, including "."
 * and ".." and, under emu2, the volume label (all three verified under
 * emu2).  Files are removed during the enumeration; access() filters out
 * entries that are not files.
 */
static int dos_cleanup(const char *work)
{
    char pattern[ACC_DOS_PATH];
    const char *base;
    char *name;
    int ok;

    if (!dos_join(pattern, work, "*.*"))
        return 0;
    ok = 1;
    while ((name = scdir(pattern)) != 0) {
        base = strrchr(name, '\\');
        base = base == 0 ? name : base + 1;
        if (strcmp(base, ".") == 0 || strcmp(base, "..") == 0 ||
            access(name, 0) != 0)
            continue;
        if (unlink(name) != 0)
            ok = dos_error("cannot remove workspace file", name);
    }
    if (rmdir(work) != 0)
        ok = dos_error("cannot remove private workspace", work);
    return ok;
}

/* SIGINT handler: record the request; execute_plan stops and cleans up. */
static void on_interrupt(int number)
{
    (void)number;
    interrupted = 1;
}

/*
 * Reject a full command tail (PSP offset 80h holds its length): the
 * arguments may have been cut, and running a different command than the
 * one typed must never happen silently.
 */
int host_command_line_ok(void)
{
    unsigned char far *psp;

    psp = (unsigned char far *)dos_far(_PSP, 0);
    if (psp[0x80] >= ACC_DOS_TAIL)
        return fail("command line fills the DOS limit (126 bytes) and may be "
                    "truncated; list link inputs in a .inp file", 0);
    return 1;
}

/*
 * Run PLAN: create the workspace and child environment, run each command
 * and verify its product, then publish every output only if all commands
 * succeeded.  Ctrl-C/Ctrl-Break stops before the next command; the
 * workspace is removed in every case unless -save-temps is given.
 * Returns 0, the first failing child status, or 1.
 */
int execute_plan(const request_t *request, const plan_t *plan)
{
    const command_t *cmd;
    unsigned far *psp;
    unsigned original_env;
    unsigned child_env;
    char work[ACC_DOS_PATH];
    char product[ACC_DOS_PATH];
    int status;
    int environment_ready;
    void (*previous)(int);

    psp = (unsigned far *)dos_far(_PSP, 0);
    original_env = psp[0x2c / 2];
    child_env = 0;
    environment_ready = 0;
    work[0] = '\0';
    status = 0;
    if (!dos_workspace(work, plan))
        return 1;
    if (!dos_environment(original_env, &child_env, plan, work)) {
        rmdir(work);
        return 1;
    }
    environment_ready = 1;
    if (request->verbose)
        fprintf(stderr, "acc: workspace: %s\n", work);
    interrupted = 0;
    previous = signal(SIGINT, on_interrupt);
    for (cmd = plan->first; cmd != 0; cmd = cmd->next) {
        if (interrupted) {
            fail("interrupted", 0);
            status = 1;
            break;
        }
        if (cmd->response.count != 0 && !response_file(cmd, work)) {
            status = 1;
            break;
        }
        if (cmd->banner != 0) {
            puts(cmd->banner);
            fflush(stdout);
        }
        status = dos_launch(cmd, plan, child_env, work);
        if (status != 0)
            break;
        if (!dos_product(product, work, cmd->product) ||
            !dos_verify_product(product) ||
            (cmd->header_flags != 0 &&
             !cmdhdr_set_flags(product, cmd->header_flags))) {
            status = 1;
            break;
        }
        if (cmd->align_product != 0 &&
            (!dos_product(product, work, cmd->align_product) ||
             !dos_verify_product(product) || !cmdhdr_align_rsm(product))) {
            status = 1;
            break;
        }
    }
    if (status == 0 && interrupted) {
        fail("interrupted", 0);
        status = 1;
    }
    if (status == 0 && !publish_outputs(plan, work, &publication))
        status = 1;
    if (request->save_temps)
        fprintf(stderr, "acc: workspace retained: %s\n", work);
    else if (!dos_cleanup(work) && status == 0)
        status = 1;
    if (previous != SIG_ERR)
        signal(SIGINT, previous);
    if (environment_ready)
        dos(0x49, child_env, 0, 0, 0, 0);
    return status;
}
