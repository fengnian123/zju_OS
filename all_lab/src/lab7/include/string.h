#pragma once

#include "types.h"

void* memset(void *, int, uint64);
void *memcpy(void *str1, const void *str2, uint64 n);
int memcmp(const void *cs, const void *ct, uint64 count);

static inline int strlen(const char *str)
{
    int len = 0;
    while (*str++)
        len++;
    return len;
}