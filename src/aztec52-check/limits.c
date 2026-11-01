#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
union bits {double d; unsigned short w[4];};
int main(void) {
 union bits a,b;char *end;int i;
 a.d=HUGE_VAL; errno=0; b.d=strtod("1e999999999999",&end);
 for(i=3;i>=0;i--)printf("%04x",a.w[i]);puts(" HUGE_VAL");
 for(i=3;i>=0;i--)printf("%04x",b.w[i]);puts(" strtod");
 printf("eq=%d errno=%d end=%d\n",a.d==b.d,errno,*end);
 return 0;
}
