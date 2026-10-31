#include <stdio.h>
#include <libc.h>
#include <stat.h>
#include <stat.h>
#include <stdarg.h>
#include <stdarg.h>
#include <stdlib.h>
#include <memory.h>
#include <strings.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <ctype.h>
#ifdef DOS_TIME
#include <time.h>
#endif

#define CHECK(test) if (!(test)) { printf("FAIL line %d\n", __LINE__); return 1; }

static int bounded(char *buf, size_t size, const char *fmt, ...)
{
    va_list args;
    int n;
    va_start(args, fmt);
    n = vsnprintf(buf, size, fmt, args);
    va_end(args);
    return n;
}

static int exit_count;

static void count_exit(void)
{
    ++exit_count;
}

static void mark_exit(void)
{
    if (exit_count == 30)
        ++exit_count;
}

static void final_exit(void)
{
    if (exit_count == 31)
        puts("ATEXIT PASS");
    else
        puts("ATEXIT FAIL");
}

static int compare_int(const void *a, const void *b)
{
    int x = *(const int *)a, y = *(const int *)b;
    return (x > y) - (x < y);
}

static int print_buffer(char *s, const char *fmt, ...)
{
    va_list args;
    int n;
    va_start(args, fmt);
    n = vsprintf(s, fmt, args);
    va_end(args);
    return n;
}

static int print_file(FILE *fp, const char *fmt, ...)
{
    va_list args;
    int n;
    va_start(args, fmt);
    n = vfprintf(fp, fmt, args);
    va_end(args);
    return n;
}

static int print_console(const char *fmt, ...)
{
    va_list args;
    int n;
    va_start(args, fmt);
    n = vprintf(fmt, args);
    va_end(args);
    return n;
}

static int portable(void)
{
    static int values[] = { 1, 3, 5, 7, 9 };
    char *end, *start;
    char text[80], buf[80];
    int key = 7, i;
    FILE *fp;
    double x;
#ifdef DOS_TIME
    time_t stamp;
    clock_t ticks;
    struct tm *date;
    CHECK(time(&stamp) == stamp && stamp != (time_t)-1);
    date = localtime(&stamp);
    CHECK(date && date->tm_year >= 80 && date->tm_mon >= 0 && date->tm_mon < 12);
    CHECK(date->tm_mday >= 1 && date->tm_mday <= 31);
    CHECK(ctime(&stamp) && strlen(ctime(&stamp)) == 25);
    ticks = clock();
    CHECK(ticks >= 0 && clock() >= ticks);
#endif

    errno = EDOM;
    start = " \v-0x7fffffff!";
    CHECK(strtol(start, &end, 0) == -2147483647L && *end == '!' && errno == EDOM);
    CHECK(strtol("-2147483648", &end, 10) == (-2147483647L-1) && !*end);
    errno = 0;
    CHECK(strtol("2147483648end", &end, 10) == 2147483647L && errno == ERANGE && !strcmp(end, "end"));
    errno = 0;
    CHECK(strtol("-2147483649", &end, 10) == (-2147483647L-1) && errno == ERANGE);
    errno = 0;
    CHECK(strtoul("4294967295", &end, 10) == 0xffffffffUL && !errno);
    CHECK(strtoul("-4294967295", &end, 10) == 1);
    CHECK(strtoul("-1", &end, 10) == 0xffffffffUL);
    CHECK(strtoul("4294967296x", &end, 10) == 0xffffffffUL && errno == ERANGE && *end == 'x');
    CHECK(strtol("0x", &end, 0) == 0 && *end == 'x');
    CHECK(strtol("0779", &end, 0) == 63 && *end == '9');
    CHECK(strtol("z", &end, 36) == 35 && !*end);
    start = "  +x";
    CHECK(strtol(start, &end, 10) == 0 && end == start);
    CHECK(strtol(start, &end, 1) == 0 && end == start && errno == EINVAL);
    start = "\3771";
    CHECK(strtol(start, &end, 10) == 0 && end == start);
    CHECK(strtod(start, &end) == 0.0 && end == start);
    CHECK(atoi("\r -23x") == -23 && atol("\f+12345") == 12345L);

    errno = 0;
    CHECK(strtod(" \v-12.5e+2!", &end) == -1250.0 && *end == '!' && !errno);
    CHECK(strtod("1e+", &end) == 1.0 && *end == 'e');
    CHECK(strtod(".25", &end) == 0.25 && !*end);
    start = "  -.";
    CHECK(strtod(start, &end) == 0.0 && end == start);
    CHECK(strtod("123456789012345678901234567890e-29", &end) > 1.23 && !*end);
    x = strtod("1e-300", &end);
    CHECK(x > 0.0 && fabs(x / 1e-300 - 1.0) < 1e-12);
    errno = 0;
    CHECK(strtod("1e999999999999", &end) == HUGE_VAL && !*end && errno == ERANGE);
    errno = 0;
    CHECK(strtod("1e-999999999999", &end) == 0.0 && errno == ERANGE);
    errno = 0;
    CHECK(strtod("4.9406564584124654e-324", &end) == 0.0 && errno == ERANGE);
    errno = 0;
    CHECK(strtod("1.7976931348623159e308", &end) == HUGE_VAL && errno == ERANGE);
    errno = 0;
    x = strtod("2.2250738585072014e-308", &end);
    CHECK(x > 0.0 && !errno);
    CHECK(strtod("0e999999999999", &end) == 0.0 && !errno);

    CHECK(bsearch(&key, values, 5, sizeof(int), compare_int) == &values[3]);
    key = 6;
    CHECK(!bsearch(&key, values, 5, sizeof(int), compare_int));
    CHECK(!bsearch(&key, values, 0, sizeof(int), compare_int));
    CHECK(strspn("abba!", "ab") == 4 && strcspn("abc!", "!") == 3);
    CHECK(strspn("abc", "") == 0 && strcspn("abc", "") == 3);
    CHECK(!strpbrk("abc", "XYZ") && *strpbrk("abc", "cx") == 'c');
    strcpy(text, ",,one;two,,three,");
    CHECK(!strcmp(strtok(text, ",;"), "one"));
    CHECK(!strcmp(strtok(NULL, ",;"), "two"));
    CHECK(!strcmp(strtok(NULL, ","), "three"));
    CHECK(!strtok(NULL, ",") && !strtok(NULL, ","));
    strcpy(text, "a,b");
    CHECK(!strcmp(strtok(text, ""), "a,b") && !strtok(NULL, ""));
    CHECK(print_buffer(buf, "%s:%ld", "abc", 12345L) == 9 && !strcmp(buf, "abc:12345"));
    CHECK(print_console("%s", "") == 0);
    CHECK(print_buffer(buf, "%.2f", 1.25) == 4 && !strcmp(buf, "1.25"));
    fp = fopen("azfmt.tmp", "w");
    CHECK(fp && print_file(fp, "%s:%d", "abc", 42) == 6 && !fclose(fp));
    fp = fopen("azfmt.tmp", "r");
    CHECK(fp && fgets(buf, sizeof(buf), fp) && !strcmp(buf, "abc:42"));
    CHECK(!fclose(fp) && !remove("azfmt.tmp"));

    CHECK(!atexit(final_exit) && !atexit(mark_exit));
    for (i = 0; i < 30; ++i)
        CHECK(!atexit(count_exit));
    CHECK(atexit(count_exit) != 0);
    return 0;
}

static int additions(void)
{
    char *p, *q, *held[128];
    int i, n;
    FILE *fp;
    fpos_t pos;
    div_t d;
    ldiv_t ld;
    union { unsigned w[4]; double d; } a,b,r;
    for (i=128; i<256; ++i) CHECK(!isalpha(i) && !isdigit(i) && !isspace(i));
    CHECK(!calloc((size_t)32768UL,2));
    free(NULL);
    p = realloc(NULL,16); CHECK(p != NULL);
    strcpy(p,"preserve");
    q = realloc(p,32); CHECK(q && !strcmp(q,"preserve")); p=q;
    q = realloc(p,8); CHECK(q && !memcmp(q,"preserve",8)); p=q;
    for (n=0; n<128; ++n) { held[n]=malloc((size_t)32768UL); if (!held[n]) break; }
    CHECK(n<128);
    q=realloc(p,(size_t)32768UL); CHECK(!q && !memcmp(p,"preserve",8));
    while (n) free(held[--n]);
    CHECK(!realloc(p,0));
    d=div(-17,5); ld=ldiv(-100003L,1000L);
    CHECK(d.quot==-3 && d.rem==-2 && ld.quot==-100 && ld.rem==-3);
    CHECK(labs(-123456L)==123456L);
    fp=fopen("azpos.tmp","w+"); CHECK(fp);
    CHECK(fputs("abc",fp)>=0 && !fseek(fp,0L,SEEK_SET));
    CHECK(getc(fp)=='a' && !fgetpos(fp,&pos));
    CHECK(getc(fp)=='b' && !fsetpos(fp,&pos) && getc(fp)=='b');
    CHECK(getc(fp)=='c');
    /* CP/M stores whole records; consume any trailing record padding. */
    while (getc(fp)!=EOF) ;
    CHECK(feof(fp));
    CHECK(ungetc('Z',fp)=='Z' && !feof(fp) && getc(fp)=='Z');
    fp->_flags |= _IOERR; rewind(fp); CHECK(!ferror(fp) && getc(fp)=='a');
    CHECK(!fclose(fp) && !remove("azpos.tmp"));
    fp=fopen("azpos.tmp","w+"); CHECK(fp);
    CHECK(ungetc(255,fp)==255 && getc(fp)==255);
    CHECK(!fclose(fp) && !remove("azpos.tmp"));
    CHECK(fmod(5.5,2.0)==1.5 && fmod(-5.5,2.0)==-1.5);
    CHECK(fmod(5.5,-2.0)==1.5 && fmod(2.0,5.5)==2.0);
    a.w[0]=a.w[1]=a.w[2]=0; a.w[3]=0x7fe0;
    CHECK(fmod(a.d,3.0)==2.0);
    errno=0; CHECK(isnan(fmod(1.0,0.0)) && errno==EDOM);
    CHECK(fmod(3.0,INFINITY)==3.0 && isnan(fmod(INFINITY,3.0)));
    a.w[0]=a.w[1]=a.w[2]=0; a.w[3]=0x8000;
    r.d=fmod(a.d,3.0); CHECK(r.w[0]==0 && r.w[1]==0 && r.w[2]==0 && (r.w[3]&0x7fff)==0);
    a.w[0]=3; a.w[3]=0; b=a; b.w[0]=2;
    r.d=fmod(a.d,b.d); CHECK(r.d==0.0); /* Software FP transport flushes subnormals. */
    return 0;
}

int main(void)
{
    static char buf[8] = "abcde";
    FILE *fp;
    char *copy;
    static union { unsigned word[4]; double value; } samples[] = {
        { { 0, 0, 0, 0x7ff0 } }, { { 0, 0, 0, 0xfff0 } },
        { { 0, 0, 0, 0x7ff8 } }, { { 1, 0, 0, 0x7ff0 } },
        { { 0, 0, 0, 0xfff8 } }, { { 0, 0, 0, 0 } },
        { { 0, 0, 0, 0x8000 } }, { { 1, 0, 0, 0 } },
        { { 0xffff, 0xffff, 0xffff, 0x7fef } }
    };
    int sample;
#if (sizeof(char *) == 4)
    union { char *p; unsigned word[2]; } alias;
    alias.p = buf + 1;
    --alias.word[1];
    alias.word[0] += 16;
    CHECK(memmove(alias.p, buf, 5) == alias.p);
    CHECK(!strcmp(buf, "aabcde"));
    strcpy(buf, "abcde");
#endif
    for (sample = 0; sample < 9; ++sample) {
        CHECK(!!isinf(samples[sample].value) == (sample < 2));
        CHECK(!!isnan(samples[sample].value) == (sample >= 2 && sample < 5));
    }
    CHECK(!additions());
    copy = strdup(buf);
    CHECK(copy && copy != buf && !strcmp(copy, "abcde"));
    copy[0] = 'X';
    CHECK(buf[0] == 'a');
    free(copy);
    copy = strdup("");
    CHECK(copy && !*copy);
    free(copy);
    CHECK(strcmp("A", "a") < 0 && !strncmp("aB", "ab", 1));
    CHECK(strcasecmp == stricmp && strncasecmp == strnicmp);
    CHECK(!strcmpi("Az", "aZ") && strcmpi("B", "a") > 0);
    CHECK(stricmp("a", "B") < 0 && stricmp("\377", "z") > 0);
    CHECK(!strnicmp("aB!", "Ab?", 2) && strnicmp("ab", "AC", 2) < 0);
    CHECK(!strnicmp("a", "z", 0));
    CHECK(memmove(buf, buf, 0) == buf && !strcmp(buf, "abcde"));
    CHECK(memmove(buf + 1, buf, 5) == buf + 1);
    CHECK(!strcmp(buf, "aabcde"));
    CHECK(memmove(buf, buf + 1, 5) == buf && !strcmp(buf, "abcdee"));
    CHECK(!strcasecmp("AzTeC", "aztec") && strcasecmp("a", "B") < 0);
    CHECK(!stricmp("A", "a") && !strncasecmp("abc", "ABD", 2));
    CHECK(!strncasecmp("a", "b", 0));
    CHECK(strcasecmp("\377", "z") > 0);
    CHECK(snprintf(buf, 4, "%s%d", "abc", 12) == 5);
    CHECK(!strcmp(buf, "abc") && buf[4] == 'e');
    CHECK(snprintf(NULL, 0, "%d", 123) == 3);
    CHECK(snprintf(buf, 1, "x") == 1 && !buf[0]);
    CHECK(bounded(buf, 5, "%s%d", "az", 123) == 5 && !strcmp(buf, "az12"));
    CHECK(bounded(NULL, 0, "%ld", 12345L) == 5);
    CHECK(!strcmp(strerror(ENOENT), "File not found"));
    CHECK(!strcmp(strerror(sys_nerr), "Unknown error"));
    CHECK(isinf(INFINITY) && !isinf(1.0));
    fp = fopen("azreg.tmp", "w");
    CHECK(fp && !fclose(fp));
    CHECK(!remove("azreg.tmp"));
    errno = 0;
    CHECK(remove("azreg.tmp") == -1 && errno == ENOENT);
    errno = 0;
    CHECK(unlink("azreg.tmp") == -1 && errno == ENOENT);
    CHECK(portable() == 0);
    puts("AZTEC PASS");
    return 0;
}
