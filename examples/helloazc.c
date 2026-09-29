#include <stdio.h>

#ifndef __STDC__
int main(argc, argv) 
    int argc;
    char **argv;
#else
int main(int argc, char **argv)
#endif
{
    printf("Hello from c (aztec cc)");
    return 0;
}

