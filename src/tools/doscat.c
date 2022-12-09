#include <stdio.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "ERR: No files specified\n");
        return -1;
    }
    for (int i = 1; i < argc; i++) {
        int c;
        FILE *f = fopen(argv[i], "r");
        if (!f) {
            fprintf(stderr, "ERR: No files specified\n");
            return -1;
        }
        while (1) {
            c = fgetc(f);
            if (c == EOF || c == 26) {
                break;
            }
            putchar(c);
        }
        fclose(f);
    }
    return 0;
}
