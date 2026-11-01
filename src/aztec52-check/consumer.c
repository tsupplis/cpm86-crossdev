#include <stdio.h>
#include "abi.h"
static int twice(int n) { return n*2; }
int main(void) {
 struct item a,b;int fail=0;
 a.c='q';a.i=10;a.l=654321L;a.d=1.5;a.p=&a;
 b=bounce(a);
 if(b.c!='q'||b.i!=17||b.l!=754321L||b.d!=1.75||b.p!=&a)fail++;
 if(number(-7000L,80000UL)!=9000L)fail++;
 if(real(1.25,2.0)!=3.25)fail++;
 if(!args(4,37,123456L,3.25,"xyz"))fail++;
 if(callback(twice,21)!=42)fail++;
 if(sizes()!=sizeof a)fail++;
 printf("ABI %s sizeof=%d pointers=%d/%d failures=%d\n",fail?"FAIL":"PASS",sizeof a,sizeof(void *),sizeof(&twice),fail);
 return fail;
}
