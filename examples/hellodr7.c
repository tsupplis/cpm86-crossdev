/* Sample ANSI C program - float division */
#include <stdio.h>

float fdiv(float a, float b)
{
    return a / b;
}

int main(int argc, char **argv)
{
    float res;
    res = fdiv(22.0, 7.0);
    printf("Hello from c (ansi-ish drc): fdiv(22,7)=%.4f\n", res);
    return 0;
}
