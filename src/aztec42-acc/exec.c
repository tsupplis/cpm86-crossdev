#ifndef ACC_DOS
#define _XOPEN_SOURCE 700
#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#endif
#endif
#include "acc.h"

#ifdef ACC_DOS
#define ACC_DOS_PATH 64
#include <dos.h>
#include <fcntl.h>
#include <stat.h>
#include <stdlib.h>
#include <string.h>

extern unsigned _PSP;
extern int fexecv(const char *, char **);
extern int wait(void);
extern int mkdir(const char *);
extern char *mktemp(char *);

static int dos_error(const char *message, const char *path)
{
    fprintf(stderr, "acc: %s: %s\n", message, path);
    return 0;
}

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

static char far *dos_far(unsigned segment, unsigned offset)
{
    unsigned long address;

    address = ((unsigned long)segment << 16) | offset;
    return (char far *)address;
}

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

static int env_copy(char far **target, const char *value)
{
    while (*value != '\0')
        *(*target)++ = *value++;
    *(*target)++ = '\0';
    return 1;
}

static int dos_environment(unsigned original, unsigned *replacement,
                           const Plan *plan, const char *work)
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

static int dos_workspace(char *out, const Plan *plan)
{
    const char *parent;
    char candidate[ACC_DOS_PATH];
    char pattern[ACC_DOS_PATH];
    int attempt;
    size_t base;

    parent = getenv("CCTEMP");
    if (parent == NULL || *parent == '\0')
        parent = getenv("TEMP");
    if (parent == NULL || *parent == '\0')
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
        if (mktemp(candidate) == NULL || *candidate == '\0')
            return fail("cannot generate a unique DOS workspace name", pattern);
        if (mkdir(candidate) == 0) {
            strcpy(out, candidate);
            return 1;
        }
    }
    return fail("cannot create a unique DOS workspace", parent);
}

static int owned_path(char *out, const char *work, const char *guest)
{
    if (strncmp(guest, "E:\\", 3) != 0 ||
        strchr(guest + 3, '\\') != NULL || strchr(guest + 3, '/') != NULL)
        return fail("invalid planned workspace artifact", guest);
    return dos_join(out, work, guest + 3);
}

static int response_file(const Command *cmd, const char *work)
{
    char path[ACC_DOS_PATH];
    char argument[ACC_DOS_PATH];
    FILE *stream;
    int i;
    int ok;

    if (!owned_path(path, work, "E:\\link.rsp"))
        return 0;
    stream = fopen(path, "w");
    if (stream == NULL)
        return dos_error("cannot create linker response file", path);
    for (i = 1; i < cmd->response.count; ++i) {
        if (strncmp(cmd->response.items[i], "E:\\", 3) == 0) {
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

static int dos_launch(const Command *cmd, const Plan *plan, unsigned envseg,
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
        if (name == NULL) {
            fail("invalid planned DOS tool path", cmd->arguments.items[0]);
            return 1;
        }
        strcat(tool, name + 1);
    }
    arguments[0] = tool;
    translated_count = 0;
    for (i = 1; i < cmd->arguments.count; ++i) {
        if (strncmp(cmd->arguments.items[i], "E:\\", 3) == 0) {
            if (translated_count >= ACC_LIMIT) {
                while (translated_count > 0)
                    free(translated[--translated_count]);
                return 1;
            }
            translated[translated_count] = malloc(ACC_DOS_PATH);
            if (translated[translated_count] == NULL) {
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
        if (!dos_join(rsp_path, work, "link.rsp"))
            return 1;
        arguments[1] = "-f";
        arguments[2] = rsp_path;
        arguments[3] = NULL;
    } else {
        arguments[cmd->arguments.count] = NULL;
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

static int dos_product(char *out, const char *work, const char *guest)
{
    return owned_path(out, work, guest);
}

static int dos_verify_product(const char *path)
{
    struct stat status;

    if (stat(path, &status) != 0 || (status.st_attr & ST_DIRECT) != 0 ||
        status.st_size == 0)
        return fail("backend did not create its output", path);
    return 1;
}

static int dos_publish(const char *source, const char *destination)
{
    char parent[ACC_DOS_PATH];
    char staged[ACC_DOS_PATH];
    char backup[ACC_DOS_PATH];
    char pattern[ACC_DOS_PATH];
    char buffer[512];
    char *slash;
    int in;
    int out;
    int amount;
    int wrote;
    int offset;
    int had_old;
    int ok;

    strcpy(parent, destination);
    slash = strrchr(parent, '\\');
    if (slash == NULL)
        return fail("planned output has no DOS directory", destination);
    *slash = '\0';
    in = open(source, O_RDONLY);
    if (in == -1)
        return dos_error("cannot open completed output", source);
    if (!dos_join(pattern, parent, "AXXXXXX.TMP")) {
        close(in);
        return 0;
    }
    if (mktemp(pattern) == NULL || *pattern == '\0') {
        close(in);
        return fail("cannot reserve DOS publication name", destination);
    }
    strcpy(staged, pattern);
    out = open(staged, O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (out == -1) {
        close(in);
        return dos_error("cannot create staged output", staged);
    }
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
    if (close(in) != 0 || close(out) != 0)
        ok = 0;
    if (!ok) {
        unlink(staged);
        return dos_error("cannot copy completed output", destination);
    }
    had_old = access(destination, 0) == 0;
    if (had_old) {
        if (!dos_join(pattern, parent, "AXXXXXX.BAK") ||
            mktemp(pattern) == NULL || *pattern == '\0' ||
            rename(destination, pattern) != 0) {
            unlink(staged);
            return dos_error("cannot preserve previous output", destination);
        }
        strcpy(backup, pattern);
    }
    if (rename(staged, destination) != 0) {
        if (had_old && rename(backup, destination) != 0)
            fprintf(stderr, "acc: cannot restore previous output: %s\n", destination);
        unlink(staged);
        return dos_error("cannot publish output", destination);
    }
    if (had_old && unlink(backup) != 0)
        return dos_error("cannot remove previous output backup", backup);
    return 1;
}

static int dos_cleanup(const char *work, const Plan *plan)
{
    const Command *cmd;
    char path[ACC_DOS_PATH];
    int ok;
    int i;

    ok = 1;
    for (cmd = plan->first; cmd != NULL; cmd = cmd->next) {
        if (owned_path(path, work, cmd->product) && access(path, 0) == 0 &&
            unlink(path) != 0)
            ok = dos_error("cannot remove workspace artifact", path);
    }
    for (i = 0; i < plan->products.count; ++i) {
        if (owned_path(path, work, plan->products.items[i]) &&
            access(path, 0) == 0 && unlink(path) != 0)
            ok = dos_error("cannot remove workspace output", path);
    }
    if (dos_join(path, work, "link.rsp") && access(path, 0) == 0 &&
        unlink(path) != 0)
        ok = dos_error("cannot remove response file", path);
    if (rmdir(work) != 0)
        ok = dos_error("cannot remove private workspace", work);
    return ok;
}

int execute_plan(const Request *request, const Plan *plan)
{
    const Command *cmd;
    unsigned far *psp;
    unsigned original_env;
    unsigned child_env;
    char work[ACC_DOS_PATH];
    char product[ACC_DOS_PATH];
    int status;
    int i;
    int environment_ready;

    if (request->target < 0 || request->target > 2)
        return fail("invalid target profile", NULL);
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
    for (cmd = plan->first; cmd != NULL; cmd = cmd->next) {
        if (cmd->response.count != 0 && !response_file(cmd, work)) {
            status = 1;
            break;
        }
        status = dos_launch(cmd, plan, child_env, work);
        if (status != 0)
            break;
        if (!dos_product(product, work, cmd->product) ||
            !dos_verify_product(product)) {
            status = 1;
            break;
        }
    }
    for (i = 0; status == 0 && i < plan->outputs.count; ++i) {
        if (!dos_product(product, work, plan->products.items[i]) ||
            !dos_verify_product(product) ||
            !dos_publish(product, plan->host_outputs.items[i]))
            status = 1;
    }
    if (request->save_temps)
        fprintf(stderr, "acc: workspace retained: %s\n", work);
    else if (!dos_cleanup(work, plan) && status == 0)
        status = 1;
    if (environment_ready)
        dos(0x49, child_env, 0, 0, 0, 0);
    return status;
}
#else
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
static volatile sig_atomic_t interrupted;

static int io_error(const char *message, const char *path)
{
    int error;

    error = errno;
    fprintf(stderr, "acc: %s: %s: %s\n", message, path, strerror(error));
    return 0;
}

static int join(char *out, const char *parent, const char *name)
{
    if (strlen(parent) + strlen(name) + 2 > ACC_PATH)
        return fail("host path is too long", name);
    strcpy(out, parent);
    strcat(out, "/");
    strcat(out, name);
    return 1;
}

static char *emulator_path(const char *name)
{
    const char *path;
    const char *end;
    char directory[ACC_PATH];
    char candidate[ACC_PATH];
    char *resolved;
    char *result;
    struct stat status;
    size_t length;

    if (strchr(name, '/') != NULL) {
        resolved = realpath(name, NULL);
        if (resolved == NULL) {
            io_error("cannot resolve emulator", name);
            return NULL;
        }
        if (stat(resolved, &status) != 0 || !S_ISREG(status.st_mode) ||
            access(resolved, X_OK) != 0) {
            fail("emulator is not an executable file", name);
            free(resolved);
            return NULL;
        }
        result = copy_text(resolved);
        free(resolved);
        return result;
    }
    path = getenv("PATH");
    if (path == NULL)
        path = "/usr/bin:/bin";
    do {
        end = strchr(path, ':');
        length = end == 0 ? strlen(path) : (size_t)(end - path);
        if (length >= sizeof(directory)) {
            fail("emulator search directory is too long", name);
            return NULL;
        }
        memcpy(directory, path, length);
        directory[length] = '\0';
        if (!join(candidate, length == 0 ? "." : directory, name))
            return NULL;
        if (stat(candidate, &status) == 0 && S_ISREG(status.st_mode) &&
            access(candidate, X_OK) == 0)
            return emulator_path(candidate);
        if (end == 0)
            break;
        path = end + 1;
    } while (1);
    fail("cannot find emulator in PATH; use -emu=path", name);
    return NULL;
}

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
    if (resolved == NULL)
        return io_error("cannot resolve workspace parent", parent);
    ok = join(out, resolved, "acc.XXXXXX");
    free(resolved);
    if (!ok)
        return 0;
    if (mkdtemp(out) == NULL)
        return io_error("cannot create private workspace", out);
    return 1;
}

static int remove_workspace(const char *path)
{
    DIR *dir;
    struct dirent *entry;
    struct stat status;
    char child[ACC_PATH];
    int ok;

    dir = opendir(path);
    if (dir == NULL)
        return io_error("cannot open workspace for cleanup", path);
    ok = 1;
    for (;;) {
        errno = 0;
        entry = readdir(dir);
        if (entry == NULL) {
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

static int owned_path(char *out, const char *work, const char *guest)
{
    if (strncmp(guest, "E:\\", 3) != 0 ||
        strpbrk(guest + 3, "/\\") != NULL)
        return fail("invalid planned workspace artifact", guest);
    return join(out, work, guest + 3);
}

static int response_file(const Command *cmd, const char *work)
{
    char path[ACC_PATH];
    FILE *stream;
    int i;
    int ok;

    if (!owned_path(path, work, "E:\\link.rsp"))
        return 0;
    stream = fopen(path, "wb");
    if (stream == NULL)
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

static int library_environment(char *out, const Words *paths)
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

static int env_key(const char *text, const char *name)
{
    size_t length;

    length = strlen(name);
    return strncmp(text, name, length) == 0 && text[length] == '=';
}

static int path_setting(const char *text)
{
    return strncmp(text, "EMU2_DRIVE_", 11) == 0 ||
        env_key(text, "EMU2_DEFAULT_DRIVE") || env_key(text, "EMU2_CWD") ||
        env_key(text, "EMU2_PROGNAME") || env_key(text, "EMU2_APPEND");
}

static void on_signal(int number)
{
    interrupted = number;
}

static int launch(const Plan *plan, const Command *cmd, const char *work,
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
    for (count = 0; environ[count] != NULL; ++count)
        ;
    environment = calloc(count + 7, sizeof(char *));
    if (environment == NULL) {
        fail("out of memory", NULL);
        return 1;
    }
    n = 0;
    for (count = 0; environ[count] != NULL; ++count) {
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

static int verify_product(const char *path)
{
    struct stat status;

    if (lstat(path, &status) != 0 || !S_ISREG(status.st_mode) || status.st_size == 0)
        return fail("backend did not create its output", path);
    return 1;
}

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
    if (slash == NULL)
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

int execute_plan(const Request *request, const Plan *plan)
{
    static const int signals[] = { SIGINT, SIGTERM, SIGHUP };
    struct sigaction action;
    struct sigaction previous[3];
    const Command *cmd;
    char work[ACC_PATH];
    char product[ACC_PATH];
    char clib[ACC_PATH];
    char *emulator;
    int installed;
    int status;
    int i;

    emulator = emulator_path(request->emulator == NULL ? "emu2" : request->emulator);
    if (emulator == NULL)
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
#endif
