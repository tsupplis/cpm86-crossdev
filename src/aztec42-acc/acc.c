#include <stdio.h>
#include <string.h>

static void usage(FILE *stream)
{
    fputs("Usage: acc -help | -version\n"
          "Aztec C 4.2 portable driver foundation.\n"
          "Compilation and linking are not implemented yet.\n"
          "Options use a single dash; see design.md for the planned interface.\n",
          stream);
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "-help") == 0) {
        usage(stdout);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "-version") == 0) {
        puts("acc 0.1-dev (driver foundation)");
        return 0;
    }
    if (argc == 1) {
        fputs("acc: no input or information option\n", stderr);
    } else {
        fputs("acc: compilation and linking are not implemented; "
              "only -help and -version are available\n", stderr);
    }
    usage(stderr);
    return 1;
}
