/* Each build references one entry point, exposing archive ordering defects.
 * Link-only: some cases deliberately pass null pointers. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#if CASE >= 36
#include <dos.h>
#endif
#if CASE >= 32
#include <time.h>
extern short monitor(int (*)(void), int (*)(void), unsigned *, int, int);
extern void profil(char *, int, int (*)(void), int);
#endif
static char buffer[32];
static fpos_t pos;
static void handler(void) {}
int main(void)
{
#if CASE == 1
    printf("%d", 7);
#elif CASE == 2
    atoi("42");
#elif CASE == 3
    atol("42");
#elif CASE == 4
    strtok(buffer, ",");
#elif CASE == 5
    fgetpos((FILE *)0, &pos);
#elif CASE == 6
    fsetpos((FILE *)0, &pos);
#elif CASE == 7
    rewind((FILE *)0);
#elif CASE == 8
    div(7, 3);
#elif CASE == 9
    ldiv(7L, 3L);
#elif CASE == 10
    snprintf(buffer, sizeof buffer, "%d", 7);
#elif CASE == 11
    strdup("copy");
#elif CASE == 12
    remove("absent.tmp");
#elif CASE == 13
    bsearch(buffer, buffer, 0, 1, (int (*)(const void *, const void *))0);
#elif CASE == 14
    strspn(buffer, "a");
#elif CASE == 15
    strcspn(buffer, "a");
#elif CASE == 16
    strpbrk(buffer, "a");
#elif CASE == 17
    atexit(handler);
#elif CASE == 18
    memmove(buffer+1, buffer, 2);
#elif CASE == 19
    strtol("42", (char **)0, 10);
#elif CASE == 20
    strtoul("42", (char **)0, 10);
#elif CASE == 21
    labs(-7L);
#elif CASE == 22
    calloc(2, 4);
#elif CASE == 23
    realloc((void *)0, 8);
#elif CASE == 24
    ungetc('x', (FILE *)0);
#elif CASE == 25
    strerror(1);
#elif CASE == 26
    strnicmp("a", "B", 1);
#elif CASE == 27
    strcmpi("a", "B");
#elif CASE == 28
    strcasecmp("a", "B");
#elif CASE == 29
    fmod(7.0, 3.0);
#elif CASE == 30
    strtod("1.5", (char **)0);
#elif CASE == 31
    printf("%f", 1.5);
#elif CASE == 32
    clock();
#elif CASE == 33
    time((time_t *)0);
#elif CASE == 34
    monitor(main, main, (unsigned *)buffer, 0, 0);
#elif CASE == 35
    profil(buffer, sizeof buffer, main, 0);
#elif CASE == 36
    ftruncate(3, 0L);
#elif CASE == 37
    truncate("test.tmp", 0L);
#endif
    return 0;
}
