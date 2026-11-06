/*
 * acc entry point: -help, -version, then parse, plan, print, execute.
 * See acc.h for the pipeline and portability rules.
 */
#include "acc.h"
#include "request.h"
#include "plan.h"
#include <stdlib.h>
#include <string.h>

/* Driver version; independent of the Aztec toolchain version. */
#define ACC_VERSION "acc 0.3-dev"

/*
 * Option summary printed by -help.  Kept to what the driver implements;
 * MANUAL.md is the full reference.  Split into several fputs calls to
 * stay under the C89 minimum string literal length.
 */
static void usage(FILE *stream)
{
    fputs("Usage: acc [options] inputs | -help | -version\n"
          "Aztec C 4.2 compiler driver.\n"
          "  inputs: .c .asm .o .lib, or one .inp (list of .o/.lib to link)\n"
          "  -c | -S                    object or assembly per source\n"
          "  -o file                    output name\n"
          "  -I dir  -D name[=value]    include area, macro\n"
          "  -L dir  -l name            library area, library\n", stream);
    fputs("  -target=cpm86|dos1|dos2\n"
          "  -mcmodel=small|medium|compact|large   (large models: dos2)\n"
          "  -march=i8086|i80186|i80286\n"
          "  -mfloat=soft|auto|8087\n"
          "  -ansi | -traditional       language mode\n"
          "  -Osize | -Ospeed  -w       code preference, no warnings\n", stream);
    fputs("  -nostartfiles | -nostdlib  omit startup | startup and libc\n"
          "  -overlay-root=CODE,DATA    overlay root (.cmd) and .rsm table\n"
          "  -overlay=ROOT.rsm          overlay (.ovr) for that root\n"
          "  -toolroot=dir              Aztec tool root (else ACCROOT)\n"
          "  -emu=path                  emulator, Unix only (else emu2)\n"
          "  -### | -v | -save-temps    plan only | show plan | keep temps\n",
          stream);
}

/*
 * Run one invocation.  -help and -version must be the only argument.
 * Exit status: 0 on success, 1 for driver errors, otherwise the status of
 * the first failing backend tool.
 */
int main(int argc, char **argv)
{
    request_t *request;
    plan_t *plan;
    int ok;
    int status;

    if (argc == 2 && strcmp(argv[1], "-help") == 0) {
        usage(stdout);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "-version") == 0) {
        puts(ACC_VERSION);
        return 0;
    }
    /* Heap, not stack: both structures are large for a 16-bit DOS stack. */
    request = calloc(1, sizeof(request_t));
    plan = calloc(1, sizeof(plan_t));
    if (request == 0 || plan == 0) {
        free(request);
        free(plan);
        fail("out of memory", NULL);
        return 1;
    }
    ok = parse_request(request, argc, argv);
    if (ok)
        ok = build_plan(request, plan);
    if (ok && (request->dryrun || request->verbose))
        print_plan(request, plan);
    status = ok ? 0 : 1;
    if (ok && !request->dryrun)
        status = execute_plan(request, plan);
    free_request(request);
    free_plan(plan);
    free(request);
    free(plan);
    return status;
}
