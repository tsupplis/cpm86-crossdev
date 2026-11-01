/* The injected BDOS wrapper closes normally, then reports an OS close error. */
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
int open(const char *, int, ...);
extern int failclose;
int main(void)
{
    int fd, bad;
    FILE *f;
    bad=0;
    fd=open("CLOSE.TMP",O_CREAT|O_TRUNC|O_RDWR,0666);
    if(fd<0) return 1;
    failclose=1;
    if(close(fd)!=-1 || errno!=EIO) bad++;
    f=fopen("CLOSE.TMP","w");
    if(!f) return 1;
    if(putc('x',f)==EOF) return 1;
    failclose=1;
    if(fclose(f)!=EOF || errno!=EIO) bad++;
    if(unlink("CLOSE.TMP")<0) bad++;
    if(!bad) puts("CLOSE ERROR PASS");
    return bad!=0;
}
