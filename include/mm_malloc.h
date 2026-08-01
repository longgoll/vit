#ifndef _MM_MALLOC_H_INCLUDED
#define _MM_MALLOC_H_INCLUDED

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline void* _mm_malloc(size_t size, size_t align) {
    (void)align;
    return malloc(size);
}

static inline void _mm_free(void* ptr) {
    free(ptr);
}

#ifdef __cplusplus
}
#endif

#endif // _MM_MALLOC_H_INCLUDED
