struct item { char c; int i; long l; double d; void *p; };
struct item bounce(struct item);
long number(long,unsigned long);
double real(float,double);
int args(int,...);
int callback(int (*)(int),int);
int sizes(void);
