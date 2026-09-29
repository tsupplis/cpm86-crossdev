/* Sample ANSI C program */
#include <stdio.h>

int main(int argc, char **argv)
{
    int res;
    res = ovloader("ovrfunc1",10, 20);
    printf("Hello from c (ansi cc with overlay1): add(10,20)=%d\n", res);
    res = ovloader("ovrfunc2",10, 20);
    printf("Hello from c (ansi cc with overlay2): add(10,20)=%d", res);
    return 0;
}
