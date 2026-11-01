#include <stdio.h>
#include <float.h>
static double v[]={0.1,1e-300,1e300,1.7976931348623157e308,DBL_MAX,DBL_MIN};
int main(void) { int i,j;unsigned short *p;for(i=0;i<6;i++){p=(unsigned short *)&v[i];printf("%d ",i);for(j=3;j>=0;j--)printf("%04x",p[j]);puts("");}return 0;}
