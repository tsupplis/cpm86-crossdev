/* Sample ANSI C program */
#include <stdio.h>

int add(a,b)
    int a; 
    int b;
{
    return a + b;
}

int main(argc,argv)
    int argc; 
    char **argv;
{
    int res;
    res = add(10, 20);
    printf("Hello from c (msc): add(10,20)=%d\n", res);
    return 0;
}
