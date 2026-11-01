#include <stdio.h>
#include <dos.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
int open(const char *, int, ...);
static int failures;
#define CHECK(x) if (!(x)) { printf("FAIL line %d\n", __LINE__); ++failures; }
int main(void)
{
#ifdef PCE_REPORT
    FILE *report;
#endif
    int fd, n;
    long left;
    char b[127];
    unlink("trunc.tmp");
    fd=open("trunc.tmp",O_CREAT|O_TRUNC|O_RDWR,0666);
    CHECK(fd>=0);
    CHECK(write(fd,"abcdef",6)==6);
    CHECK(lseek(fd,5L,0)==5L);
    CHECK(ftruncate(fd,3L)==0);
    CHECK(lseek(fd,0L,1)==5L);
    CHECK(lseek(fd,0L,2)==3L);
    CHECK(ftruncate(fd,65539L)==0);
    CHECK(lseek(fd,0L,1)==3L);
    left=65536L;
    while(left>0) {
        n=left>127L?127:(int)left;
        CHECK(read(fd,b,n)==n);
        while(n--) { CHECK(b[n]==0); }
        left-=left>127L?127L:left;
    }
    CHECK(lseek(fd,0L,0)==0L);
    CHECK(read(fd,b,3)==3);
    CHECK(b[0]=='a' && b[1]=='b' && b[2]=='c');
    CHECK(ftruncate(fd,65539L)==0);
    CHECK(ftruncate(fd,-1L)==-1 && errno==EINVAL);
    CHECK(close(fd)==0);
    CHECK(ftruncate(fd,0L)==-1 && errno==EBADF);
    CHECK(truncate("trunc.tmp",2L)==0);
    fd=open("trunc.tmp",O_RDONLY);
    CHECK(fd>=0);
    CHECK(lseek(fd,0L,2)==2L);
    CHECK(ftruncate(fd,0L)==-1);
    CHECK(close(fd)==0);
    CHECK(truncate("trunc.tmp",0L)==0);
    fd=open("trunc.tmp",O_RDONLY);
    CHECK(lseek(fd,0L,2)==0L);
    CHECK(close(fd)==0);
    CHECK(unlink("trunc.tmp")==0);
    CHECK(truncate("trunc.tmp",1L)==-1);
    CHECK(open("trunc.tmp",O_RDONLY)==-1);
    CHECK(ftruncate(-1,0L)==-1 && errno==EBADF);
    CHECK(ftruncate(1,0L)==-1);
    if(!failures) puts("TRUNCATE PASS");
#ifdef PCE_REPORT
    report=fopen("RESULT.TXT","w");
    if(report) {
        fprintf(report,"TRUNCATE %s: %d failures\n",
                failures?"FAIL":"PASS",failures);
        fclose(report);
    }
#endif
    return failures!=0;
}
