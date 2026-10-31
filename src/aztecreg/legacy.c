#define __NOPROTO__
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <memory.h>
#include <unistd.h>
#include <math.h>

/* Check pointer-valued calls also have declarations in legacy mode. */
char *legacy_error(int e)
{
    return strerror(e);
}
