#include "acc.h"
#include <stdlib.h>
#include <string.h>

static void usage(FILE *stream)
{
    fputs("Usage: acc [options] inputs | -help | -version\n"
          "Aztec C 4.2 portable compiler driver.\n"
          "  -c / -S                  Stop at object / assembly output\n"
          "  -o file -I dir -D macro   Output, includes, definitions\n"
          "  -L dir -l name           Library search and ordered inputs\n"
          "  -target=cpm86|dos1|dos2   Select target runtime\n"
          "  -mcmodel=small|medium|compact|large\n"
          "  -march=i8086|i80186|i80286\n", stream);
    fputs(
          "  -mfloat=soft|auto|8087    Select math backend\n"
          "  -ansi -traditional -Osize -Ospeed -w\n"
          "  -nostartfiles -nostdlib  Suppress automatic runtime parts\n"
          "  -toolroot=path           Aztec root (or ACCROOT)\n", stream);
    fputs(
          "  -overlay-root=CODE,DATA  Build CP/M-86 root program and .rsm table\n"
          "  -overlay=ROOT.rsm        Build a CP/M-86 .ovr using the root table\n"
          "  -emu=path                Unix emulator (default: emu2 in PATH)\n"
          "  -### -v -save-temps      Plan only, show commands, retain workspace\n",
          stream);
    fputs("Unix and native DOS execution are supported for qualified plans.\n"
          "Overlay options are CP/M-86 link-only; provide an explicit -o output.\n",
          stream);
    fputs("Debug, raw flags and user response files are not implemented yet.\n"
          "Options use a single dash; see design.md for limitations.\n", stream);
}

int main(int argc, char **argv)
{
    Request *request;
    Plan *plan;
    int ok;
    int status;

    if (argc == 2 && strcmp(argv[1], "-help") == 0) {
        usage(stdout);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "-version") == 0) {
        puts("acc 0.3-dev");
        return 0;
    }
    request = calloc(1, sizeof(Request));
    plan = calloc(1, sizeof(Plan));
    if (request == NULL || plan == NULL) {
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
