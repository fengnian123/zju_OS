#include <string.h>
#include <defs.h>
#include <stddef.h>

void *memset(void *dst, int c, uint64 n) {
    char *cdst = (char *)dst;
    for (uint64 i = 0; i < n; ++i)
        cdst[i] = c;

    return dst;
}

void *memcpy(void *str1, const void *str2, uint64 n) {
    for (int i = 0; i < n; i++) {
        ((char*)str1)[i] = ((const char*)str2)[i];
    }
    return str1;
}

int memcmp(const void *cs, const void *ct, uint64 count)
{
    const unsigned char *su1, *su2;
    int res = 0;
    for (su1 = cs, su2 = ct; 0 < count; ++su1, ++su2, count--)
        if ((res = *su1 - *su2) != 0)
            break;
    return res;
}