#include <stdarg.h>
#include "abi.h"
struct item bounce(struct item a) { a.i+=7; a.l+=100000L; a.d+=0.25; return a; }
long number(long a,unsigned long b) { return a/7L+(long)(b>>3); }
double real(float a,double b) { return a+b; }
int callback(int (*f)(int),int n) { return (*f)(n); }
int args(int n,...) {
 va_list ap;int i;long l;double d;char *s;
 va_start(ap,n);i=va_arg(ap,int);l=va_arg(ap,long);d=va_arg(ap,double);s=va_arg(ap,char *);va_end(ap);
 return n==4&&i==37&&l==123456L&&d==3.25&&s[0]=='x';
}
int sizes(void) { return sizeof(struct item); }
