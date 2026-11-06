/* Sample ANSI C program */
#include <stdio.h>

/* The DOS build (-target=dos2 defines __MSDOS__) loads its own overlays. */
#ifdef __MSDOS__
#define OVERLAY1 "ovrfnd1"
#define OVERLAY2 "ovrfnd2"
#else
#define OVERLAY1 "ovrfunc1"
#define OVERLAY2 "ovrfunc2"
#endif

int main(int argc, char **argv)
{
    int res;
    res = ovloader(OVERLAY1, 10, 20);
    printf("Hello from c (ansi cc with overlay1): fn1(10,20)=%d\n", res);
    res = ovloader(OVERLAY2, 10, 20);
    printf("Hello from c (ansi cc with overlay2): fn2(10,20)=%d", res);
    return 0;
}
