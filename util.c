#include "util.h"

static char numbuf[256];

void print_double(double value)
{
    int last = snprintf(numbuf, 256, "%.6f", value);

    while(numbuf[--last] == '0') {}

    if(numbuf[last] == '.') { numbuf[last] = '\0'; }
    numbuf[last+1] = '\0';

    printf("%s", numbuf);
}
