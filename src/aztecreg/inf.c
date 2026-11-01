#include <stdio.h>
#include <math.h>
#define CHECK(x) if (!(x)) { printf("FAIL %d\n",__LINE__); return 1; }
volatile double zero=0.0, one=1.0, two=2.0;
int main(void) {
 double i,n,z,v;
 float f;
 union { unsigned w[2]; float f; } small;
 union { unsigned w[4]; double d; } bits;
 i=one/zero; CHECK(isinf(i));
 n=zero/zero; CHECK(isnan(n));
 CHECK(isinf(i+one)); CHECK(isinf(i+i));
 CHECK(isnan(i-i)); CHECK(isnan(i*zero)); CHECK(isnan(i/i));
 CHECK(isinf(one/zero)); CHECK(isnan(n+one)); CHECK(isnan(one*n));
 z=-zero; bits.d=z; CHECK(bits.w[3]==0x8000);
 CHECK(z==zero); CHECK(!(z<zero)); CHECK(!(zero<z));
 bits.d=one/(-i); CHECK(bits.w[3]==0x8000);
 CHECK(isinf((-one)/zero)); CHECK((-one)/zero<zero);
 bits.w[0]=bits.w[1]=bits.w[2]=0xffff;bits.w[3]=0x7fef;
 v=bits.d; CHECK(isinf(v*two));
 CHECK(i>one && -i<zero);
 CHECK(one+two==3.0 && two*two==4.0 && one/two==0.5);
 f=i; CHECK(isinf((double)f));
 f=n; CHECK(isnan((double)f));
 f=z; small.f=f; CHECK(small.w[1]==0x8000);
 v=1.0e100; f=v; CHECK(isinf((double)f));
 CHECK(isinf(log(zero)) && log(zero)<zero);
 CHECK(isinf(log(i)) && log(i)>zero);
 CHECK(isnan(log(-one)) && isnan(log(n)));
 CHECK(isinf(log10(zero)) && log10(zero)<zero);
 puts("INF CORE PASS");
 printf("NaN comparisons: eq=%d ne=%d lt=%d le=%d gt=%d ge=%d\n",n==n,n!=n,n<one,n<=one,n>one,n>=one);

#ifdef REQUIRE_UNORDERED
 CHECK(n!=n && !(n==n));
 CHECK(!(n<one) && !(n<=one) && !(n>one) && !(n>=one));
#endif
 return 0;
}
