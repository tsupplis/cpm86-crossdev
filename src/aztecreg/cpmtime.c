/* CP/M-86 clock conversions, run on DOS: the conversion routines of the
 * c86 time module take no OS input.  Every day from 1978-01-01 to
 * 2107-12-31 is decoded from a BDOS stamp and packed, and the packed value
 * is compared with the DOS runtime's mktime and localtime. */
#include <stdio.h>
#include <string.h>
#include <time.h>

extern long _cpmpack(int *);
extern int _cpmstmp(char *, int *);
extern int _cpmpars(char *, int *);

static int fail;
#define CHECK(x) if (!(x)) { printf("CPM TIME FAIL line %d\n", __LINE__); ++fail; }

static int leap(int y)
{
    return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
}

static int length(int y, int m)
{
    static char days[] = {31,28,31,30,31,30,31,31,30,31,30,31};

    return days[m - 1] + (m == 2 && leap(y));
}

static void stamp(char *s, unsigned day, int hour, int min, int sec)
{
    s[0] = (char)(day & 255);
    s[1] = (char)(day >> 8);
    s[2] = (char)hour;
    s[3] = (char)min;
    s[4] = (char)sec;
}

static long packed(int y, int m, int d, int h, int n, int s)
{
    struct tm t;

    memset(&t, 0, sizeof t);
    t.tm_year = y - 1900;
    t.tm_mon = m - 1;
    t.tm_mday = d;
    t.tm_hour = h;
    t.tm_min = n;
    t.tm_sec = s;
    return mktime(&t);
}

int main(void)
{
    int dt[6], y, m, d;
    unsigned day;
    char st[5];
    long value;
    struct tm *back;
    static char *good[] = {
        "11/06/26\t18:34:29", "01/01/00 00:00:00", "12/31/99:23:59:59"
    };
    static char *bad[] = {
        "11-06/26\t18:34:29", "11/06-26\t18:34:29", "11/06/26\t18-34:29",
        "11/06/26\t18:34-29", "1x/06/26\t18:34:29", "11/06/26\t18:34:2",
        "11/06/26\t1:34:29"
    };
    int i;

    y = 1978; m = 1; d = 1;
    for (day = 1; y < 2108; ++day) {
        stamp(st, day, 0x23, 0x59, 0x59);
        if (_cpmstmp(st, dt) != 0 || dt[0] != y || dt[1] != m || dt[2] != d ||
            dt[3] != 23 || dt[4] != 59 || dt[5] != 59) {
            printf("CPM TIME FAIL stamp day %u: %d-%d-%d\n", day, dt[0], dt[1], dt[2]);
            ++fail;
            break;
        }
        value = _cpmpack(dt);
        if (y < 1980) {
            if (value != -1L) {
                printf("CPM TIME FAIL %d-%d-%d packs\n", y, m, d);
                ++fail;
                break;
            }
        } else if (value != packed(y, m, d, 23, 59, 59) || value == -1L) {
            printf("CPM TIME FAIL pack %d-%d-%d\n", y, m, d);
            ++fail;
            break;
        }
        if (++d > length(y, m)) {
            d = 1;
            if (++m > 12) {
                m = 1;
                ++y;
            }
        }
    }
    CHECK(day == 47482U);

    dt[0] = 2026; dt[1] = 11; dt[2] = 6; dt[3] = 18; dt[4] = 34; dt[5] = 29;
    value = _cpmpack(dt);
    back = localtime(&value);
    CHECK(back->tm_year == 126 && back->tm_mon == 10 && back->tm_mday == 6);
    CHECK(back->tm_hour == 18 && back->tm_min == 34 && back->tm_sec == 28);
    CHECK(back->tm_wday == 5 && back->tm_yday == 309);

    stamp(st, 0, 0, 0, 0);
    CHECK(_cpmstmp(st, dt) == -1);
    stamp(st, 1, 0x1a, 0, 0);
    CHECK(_cpmstmp(st, dt) == -1);
    stamp(st, 1, 0, 0x6a, 0);
    CHECK(_cpmstmp(st, dt) == -1);
    stamp(st, 1, 0, 0, 0xa0);
    CHECK(_cpmstmp(st, dt) == -1);

    {
        static int limits[][6] = {
            {1979, 12, 31, 0, 0, 0}, {2108, 1, 1, 0, 0, 0}, {2000, 0, 1, 0, 0, 0},
            {2000, 13, 1, 0, 0, 0}, {2000, 1, 0, 0, 0, 0}, {2000, 1, 32, 0, 0, 0},
            {2000, 4, 31, 0, 0, 0}, {2001, 2, 29, 0, 0, 0}, {2100, 2, 29, 0, 0, 0},
            {2000, 1, 1, 24, 0, 0}, {2000, 1, 1, 0, 60, 0}, {2000, 1, 1, 0, 0, 60},
            {2000, 1, 1, -1, 0, 0}, {-1, 1, 1, 0, 0, 0}
        };
        static int valid[][6] = {
            {1980, 1, 1, 0, 0, 0}, {2107, 12, 31, 23, 59, 59}, {2000, 2, 29, 0, 0, 0},
            {2096, 2, 29, 12, 0, 1}
        };

        for (i = 0; i < sizeof limits / sizeof limits[0]; ++i)
            CHECK(_cpmpack(limits[i]) == -1L);
        for (i = 0; i < sizeof valid / sizeof valid[0]; ++i)
            CHECK(_cpmpack(valid[i]) == packed(valid[i][0], valid[i][1], valid[i][2],
                                               valid[i][3], valid[i][4], valid[i][5]));
    }

    CHECK(_cpmpars(good[0], dt) == 0 && dt[0] == 2026 && dt[1] == 11 && dt[2] == 6);
    CHECK(dt[3] == 18 && dt[4] == 34 && dt[5] == 29);
    CHECK(_cpmpars(good[1], dt) == 0 && dt[0] == 2000 && dt[1] == 1 && dt[2] == 1);
    CHECK(dt[3] == 0 && dt[4] == 0 && dt[5] == 0);
    CHECK(_cpmpars(good[2], dt) == 0 && dt[0] == 2099 && dt[1] == 12 && dt[2] == 31);
    CHECK(dt[3] == 23 && dt[4] == 59 && dt[5] == 59);
    for (i = 0; i < sizeof bad / sizeof bad[0]; ++i)
        CHECK(_cpmpars(bad[i], dt) == -1);

    puts(fail ? "CPM TIME FAIL" : "CPM TIME PASS");
    return fail != 0;
}
