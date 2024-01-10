#include "string.h"

void *memset(void *dst, int c, unsigned long long n) {
    char *cdst = (char *)dst;
    for (unsigned long long i = 0; i < n; ++i)
        cdst[i] = c;

    return dst;
}

void* memcopy(void *copy, void *src, uint64 size){
    char *cdst = (char *)copy;
    char *csrc = (char *)src;
    for (uint64 i = 0; i < size; ++i){
        cdst[i] = csrc[i];
    }
    return copy;
}