/* Sample ANSI C program */
#include <stdio.h>

int add(int a, int b)
{
    return a + b;
}

int main(int argc, char **argv)
{
    int res;
    res = add(10, 20);
    printf("Hello from c (ansi cc): add(10,20)=%d\n", res);
    return 0;
}
