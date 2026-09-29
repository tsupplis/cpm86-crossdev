/* Sample ANSI C program */
#include <stdio.h>

int main(int argc, char **argv)
{
    int res;
    res = ovloader("ovrfunc",10, 20);
    printf("Hello from c (ansi cc with ovrlay): add(10,20)=%d", res);
    return 0;
}
