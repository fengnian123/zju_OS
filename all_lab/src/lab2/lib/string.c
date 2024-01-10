#include "string.h"

void *memset(void *dst, int c, unsigned long long n) {
    char *cdst = (char *)dst;
    for (unsigned long long i = 0; i < n; ++i)
        cdst[i] = c;

    return dst;
}
