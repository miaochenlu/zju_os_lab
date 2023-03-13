#include "string.h"

void *memset(void *dst, int c, uint64 n) {
    char *cdst = (char *)dst;
    for (uint64 i = 0; i < n; ++i)
        cdst[i] = c;

    return dst;
}

void* memcpy(void* dst, void* src, size_t length) {
    if(dst == NULL || src == NULL) return NULL;

    char* cdst = (char*)dst;
    char* csrc = (char*)src;
    for(size_t i = 0; i < length; i++) {
        cdst[i] = csrc[i];
    }
    return cdst;
}