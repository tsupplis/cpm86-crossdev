/*
 * Unix execution of a plan_t, running the Aztec DOS tools under emu2.
 *
 * For every tool the emulator maps C: to the invocation directory, D: to
 * the tool root and E: to a private workspace (CCTEMP, TEMP, TMPDIR, then
 * /tmp).  Products are verified after each tool, then atomically published
 * through a temporary file beside each destination.  Owned files are
 * removed on success, failure and handled INT/TERM/HUP signals, unless
 * -save-temps is given.
 */
#define _XOPEN_SOURCE 700
#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#endif
#include "acc.h"
#include "request.h"
#include "plan.h"
#include "path.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

/* Signal caught while a child runs; forwarded to it, then reported. */
static volatile sig_atomic_t interrupted;

/* Print a diagnostic naming PATH and the errno text; always returns 0. */
static int io_error(const char *message, const char *path)
{
    int error;

    error = errno;
    fprintf(stderr, "acc: %s: %s: %s\n", message, path, strerror(error));
    return 0;
}

/* OUT = PARENT/NAME within ACC_PATH; 0 after a diagnostic. */
static int join(char *out, const char *parent, const char *name)
{
    if (strlen(parent) + strlen(name) + 2 > ACC_PATH)
        return fail("host path is too long", name);
    strcpy(out, parent);
    strcat(out, "/");
    strcat(out, name);
    return 1;
}

/*
 * Resolve emulator NAME (-emu, default "emu2") to a heap-allocated
 * absolute path, as a shell would.  NULL after a diagnostic.
 */
static char *emulator_path(const char *name)
{
    char resolved[ACC_PATH];

    if (!path_program(resolved, name)) {
        fail(strchr(name, '/') != 0 ? "emulator is not an executable file" :
             "cannot find emulator in PATH; use -emu=path", name);
        return NULL;
    }
    return copy_text(resolved);
}

/* Create a private workspace directory under CCTEMP, TEMP, TMPDIR or
 * /tmp (first one set) and store its absolute path in OUT. */
static int workspace(char *out)
{
    const char *parent;
    char *resolved;
    int ok;

    parent = getenv("CCTEMP");
    if (parent == 0 || *parent == '\0')
        parent = getenv("TEMP");
    if (parent == 0 || *parent == '\0')
        parent = getenv("TMPDIR");
    if (parent == 0 || *parent == '\0')
        parent = "/tmp";
    resolved = realpath(parent, NULL);
    if (resolved == 0)
        return io_error("cannot resolve workspace parent", parent);
    ok = join(out, resolved, "acc.XXXXXX");
    free(resolved);
    if (!ok)
        return 0;
    if (mkdtemp(out) == 0)
        return io_error("cannot create private workspace", out);
    return 1;
}

/* Remove directory PATH recursively without following symbolic links. */
static int remove_workspace(const char *path)
{
    DIR *dir;
    struct dirent *entry;
    struct stat status;
    char child[ACC_PATH];
    int ok;

    dir = opendir(path);
    if (dir == 0)
        return io_error("cannot open workspace for cleanup", path);
    ok = 1;
    for (;;) {
        errno = 0;
        entry = readdir(dir);
        if (entry == 0) {
            if (errno != 0)
                ok = io_error("cannot read workspace directory", path);
            break;
        }
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        if (!join(child, path, entry->d_name)) {
            ok = 0;
            break;
        }
        if (lstat(child, &status) != 0) {
            ok = io_error("cannot inspect workspace entry", child);
            break;
        }
        if (S_ISDIR(status.st_mode)) {
            if (!remove_workspace(child)) {
                ok = 0;
                break;
            }
        } else if (unlink(child) != 0) {
            ok = io_error("cannot remove workspace entry", child);
            break;
        }
    }
    if (closedir(dir) != 0)
        ok = io_error("cannot close workspace directory", path);
    if (ok && rmdir(path) != 0)
        ok = io_error("cannot remove workspace directory", path);
    return ok;
}

/*
 * Translate workspace guest path GUEST (E:\NAME) to its path in WORK.
 * Anything else is refused, so acc only ever writes or removes files it
 * owns.
 */
static int owned_path(char *out, const char *work, const char *guest)
{
    if (strncmp(guest, "E:\\", 3) != 0 ||
        strpbrk(guest + 3, "/\\") != 0)
        return fail("invalid planned workspace artifact", guest);
    return join(out, work, guest + 3);
}

/* Write CMD's linker response file in WORK, one CRLF-terminated argument
 * per line (guest paths: E: is mapped by the emulator). */
static int response_file(const command_t *cmd, const char *work)
{
    char path[ACC_PATH];
    FILE *stream;
    int i;
    int ok;

    if (!owned_path(path, work, cmd->response_file))
        return 0;
    stream = fopen(path, "wb");
    if (stream == 0)
        return io_error("cannot create linker response file", path);
    for (i = 1; i < cmd->response.count; ++i)
        fprintf(stream, "%s\r\n", cmd->response.items[i]);
    ok = !ferror(stream);
    if (!ok)
        io_error("cannot write linker response file", path);
    if (fclose(stream) != 0)
        ok = io_error("cannot close linker response file", path);
    return ok;
}

/* Build "CLIB=AREA\;AREA\..." in OUT from guest library areas; ln needs
 * the trailing backslash to prefix file names. */
static int library_environment(char *out, const word_list_t *paths)
{
    int i;
    size_t length;

    strcpy(out, "CLIB=");
    for (i = 0; i < paths->count; ++i) {
        length = strlen(paths->items[i]);
        if (strlen(out) + length + 3 > ACC_PATH)
            return fail("translated library environment is too long", NULL);
        if (i != 0)
            strcat(out, ";");
        strcat(out, paths->items[i]);
        if (length != 0 && paths->items[i][length - 1] != '\\')
            strcat(out, "\\");
    }
    return 1;
}

/* Non-zero if environment entry TEXT defines variable NAME. */
static int env_key(const char *text, const char *name)
{
    size_t length;

    length = strlen(name);
    return strncmp(text, name, length) == 0 && text[length] == '=';
}

/* Non-zero if TEXT is an emu2 drive or path setting that acc replaces
 * for every child, so inherited values cannot redirect the tools. */
static int path_setting(const char *text)
{
    return strncmp(text, "EMU2_DRIVE_", 11) == 0 ||
        env_key(text, "EMU2_DEFAULT_DRIVE") || env_key(text, "EMU2_CWD") ||
        env_key(text, "EMU2_PROGNAME") || env_key(text, "EMU2_APPEND");
}

/* Record signal NUMBER; launch forwards it to the running child. */
static void on_signal(int number)
{
    interrupted = number;
}

/*
 * Run CMD under EMULATOR and wait for it.  The child environment is the
 * inherited one without emu2 drive settings, plus C: = invocation
 * directory, D: = tool root, E: = WORK.  DOS variables after "--" give the
 * tool PATH, an empty INCLUDE (include areas are explicit -I arguments),
 * CLIB and CCTEMP.  Returns the child status (128+signal when killed), or
 * 1 after a diagnostic.
 */
static int launch(const plan_t *plan, const command_t *cmd, const char *work,
                  char *emulator, char *clib)
{
    char tool[ACC_PATH];
    char bindir[ACC_PATH];
    char drive_c[ACC_PATH + 16];
    char drive_d[ACC_PATH + 16];
    char drive_e[ACC_PATH + 16];
    char progname[ACC_PATH + 16];
    char *arguments[ACC_LIMIT + 10];
    char **environment;
    const char *name;
    size_t count;
    size_t n;
    int i;
    int k;
    int status;
    int result;
    pid_t pid;
    pid_t waited;

    name = strrchr(cmd->arguments.items[0], '\\');
    if (name == 0) {
        fail("invalid planned backend path", cmd->arguments.items[0]);
        return 1;
    }
    if (!join(bindir, plan->root, "bin") || !join(tool, bindir, name + 1))
        return 1;
    for (count = 0; environ[count] != 0; ++count)
        ;
    environment = calloc(count + 7, sizeof(char *));
    if (environment == 0) {
        fail("out of memory", NULL);
        return 1;
    }
    n = 0;
    for (count = 0; environ[count] != 0; ++count) {
        if (!path_setting(environ[count]))
            environment[n++] = environ[count];
    }
    sprintf(drive_c, "EMU2_DRIVE_C=%s", plan->cwd);
    sprintf(drive_d, "EMU2_DRIVE_D=%s", plan->root);
    sprintf(drive_e, "EMU2_DRIVE_E=%s", work);
    sprintf(progname, "EMU2_PROGNAME=%s", cmd->arguments.items[0]);
    environment[n++] = drive_c;
    environment[n++] = drive_d;
    environment[n++] = drive_e;
    environment[n++] = "EMU2_DEFAULT_DRIVE=C";
    environment[n++] = "EMU2_CWD=C:\\";
    environment[n++] = progname;
    environment[n] = NULL;
    k = 0;
    arguments[k++] = emulator;
    arguments[k++] = tool;
    for (i = 1; i < cmd->arguments.count; ++i)
        arguments[k++] = cmd->arguments.items[i];
    arguments[k++] = "--";
    arguments[k++] = "PATH=D:\\bin";
    /* All include areas are already explicit, ordered -I arguments. */
    arguments[k++] = "INCLUDE=";
    arguments[k++] = clib;
    arguments[k++] = "CCTEMP=E:\\";
    arguments[k] = NULL;
    pid = fork();
    if (pid == -1) {
        free(environment);
        io_error("cannot fork emulator", emulator);
        return 1;
    }
    if (pid == 0) {
        execve(emulator, arguments, environment);
        io_error("cannot execute emulator", emulator);
        _exit(127);
    }
    free(environment);
    do {
        if (interrupted && kill(pid, interrupted) != 0 && errno != ESRCH)
            io_error("cannot interrupt emulator", emulator);
        waited = waitpid(pid, &status, 0);
    } while (waited == -1 && errno == EINTR);
    if (waited == -1) {
        io_error("cannot wait for emulator", emulator);
        return 1;
    }
    if (interrupted)
        result = 128 + interrupted;
    else if (WIFEXITED(status))
        result = WEXITSTATUS(status);
    else if (WIFSIGNALED(status))
        result = 128 + WTERMSIG(status);
    else
        result = 1;
    if (result != 0)
        fprintf(stderr, "acc: backend failed: %s for %s (status %d)\n",
                cmd->arguments.items[0], cmd->source, result);
    return result;
}

/* Fail unless the tool created PATH as a non-empty regular file: some
 * Aztec tools report success without writing their output. */
static int verify_product(const char *path)
{
    struct stat status;

    if (lstat(path, &status) != 0 || !S_ISREG(status.st_mode) || status.st_size == 0)
        return fail("backend did not create its output", path);
    return 1;
}

/*
 * Copy SOURCE to DESTINATION atomically: write a temporary file beside
 * DESTINATION (so it works across filesystems), then rename it over the
 * destination.  On failure the previous DESTINATION is preserved.
 */
static int publish(const char *source, const char *destination)
{
    char staged[ACC_PATH];
    char parent[ACC_PATH];
    char buffer[16384];
    char *slash;
    struct stat status;
    ssize_t size;
    ssize_t written;
    ssize_t offset;
    int input;
    int output;
    int ok;

    staged[0] = '\0';
    input = -1;
    output = -1;
    ok = 0;
    strcpy(parent, destination);
    slash = strrchr(parent, '/');
    if (slash == 0)
        return fail("planned output is not absolute", destination);
    if (slash == parent)
        slash[1] = '\0';
    else
        *slash = '\0';
    input = open(source, O_RDONLY | O_NOFOLLOW);
    if (input == -1) {
        io_error("cannot open completed output", source);
        return 0;
    }
    if (fstat(input, &status) != 0 || !S_ISREG(status.st_mode) || status.st_size == 0) {
        fail("completed output is not a nonempty regular file", source);
        goto done;
    }
    if (!join(staged, parent, ".acc.XXXXXX"))
        goto done;
    output = mkstemp(staged);
    if (output == -1) {
        staged[0] = '\0';
        io_error("cannot stage output publication", destination);
        goto done;
    }
    for (;;) {
        if (interrupted)
            goto done;
        size = read(input, buffer, sizeof(buffer));
        if (size == -1) {
            if (errno == EINTR)
                continue;
            io_error("cannot read completed output", source);
            goto done;
        }
        if (size == 0)
            break;
        for (offset = 0; offset < size; offset += written) {
            written = write(output, buffer + offset, (size_t)(size - offset));
            if (written == -1 && errno == EINTR) {
                written = 0;
                if (interrupted)
                    goto done;
                continue;
            }
            if (written <= 0) {
                if (written == 0)
                    errno = EIO;
                io_error("cannot write staged output", destination);
                goto done;
            }
        }
    }
    if (fchmod(output, status.st_mode & 0777) != 0 || fsync(output) != 0) {
        io_error("cannot finish staged output", destination);
        goto done;
    }
    if (close(output) != 0) {
        output = -1;
        io_error("cannot close staged output", destination);
        goto done;
    }
    output = -1;
    if (interrupted)
        goto done;
    if (rename(staged, destination) != 0) {
        io_error("cannot publish output", destination);
        goto done;
    }
    staged[0] = '\0';
    ok = 1;
done:
    if (input != -1 && close(input) != 0)
        ok = io_error("cannot close completed output", source);
    if (output != -1 && close(output) != 0)
        ok = io_error("cannot close staged output", destination);
    if (staged[0] != '\0' && unlink(staged) != 0)
        ok = io_error("cannot remove staged output", staged);
    return ok;
}

/* Unix passes argv intact: there is no command tail to truncate. */
int host_command_line_ok(void)
{
    return 1;
}

/*
 * Run PLAN: resolve the emulator, create the workspace, run each command
 * (writing its response file first) and verify its product, then publish
 * every output only if all commands succeeded.  Signals are caught so the
 * workspace is always cleaned up.  Returns 0, the first failing child
 * status, or 1.
 */
int execute_plan(const request_t *request, const plan_t *plan)
{
    static const int signals[] = { SIGINT, SIGTERM, SIGHUP };
    struct sigaction action;
    struct sigaction previous[3];
    const command_t *cmd;
    char work[ACC_PATH];
    char product[ACC_PATH];
    char clib[ACC_PATH];
    char *emulator;
    int installed;
    int status;
    int i;

    emulator = emulator_path(request->emulator == 0 ? "emu2" : request->emulator);
    if (emulator == 0)
        return 1;
    if (!library_environment(clib, &plan->library_paths)) {
        free(emulator);
        return 1;
    }
    memset(&action, 0, sizeof(action));
    action.sa_handler = on_signal;
    sigemptyset(&action.sa_mask);
    interrupted = 0;
    status = 0;
    work[0] = '\0';
    for (installed = 0; installed < 3; ++installed) {
        if (sigaction(signals[installed], &action, &previous[installed]) != 0) {
            io_error("cannot install interrupt handler", "acc");
            status = 1;
            goto done;
        }
    }
    if (!workspace(work)) {
        work[0] = '\0';
        status = 1;
        goto done;
    }
    if (request->verbose)
        fprintf(stderr, "acc: workspace: %s\n", work);
    for (cmd = plan->first; cmd != 0; cmd = cmd->next) {
        if (interrupted)
            break;
        if (cmd->response.count != 0 && !response_file(cmd, work)) {
            status = 1;
            break;
        }
        status = launch(plan, cmd, work, emulator, clib);
        if (status != 0)
            break;
        if (!owned_path(product, work, cmd->product) || !verify_product(product)) {
            status = 1;
            break;
        }
    }
    for (i = 0; status == 0 && !interrupted && i < plan->outputs.count; ++i) {
        if (!owned_path(product, work, plan->products.items[i]) ||
            !verify_product(product) ||
            !publish(product, plan->host_outputs.items[i]))
            status = 1;
    }
done:
    if (work[0] != '\0') {
        if (request->save_temps)
            fprintf(stderr, "acc: workspace retained: %s\n", work);
        else if (!remove_workspace(work)) {
            fprintf(stderr, "acc: workspace cleanup incomplete: %s\n", work);
            if (status == 0)
                status = 1;
        }
    }
    while (installed > 0) {
        --installed;
        if (sigaction(signals[installed], &previous[installed], NULL) != 0) {
            io_error("cannot restore interrupt handler", "acc");
            if (status == 0)
                status = 1;
        }
    }
    free(emulator);
    if (interrupted) {
        fprintf(stderr, "acc: interrupted by signal %d\n", (int)interrupted);
        status = 128 + interrupted;
    }
    return status;
}
