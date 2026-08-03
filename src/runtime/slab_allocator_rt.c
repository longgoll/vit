#include "slab_allocator_rt.h"
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <pthread.h>
#endif

vit_slab_pool_t* vit_slab_pool_create(uint32_t capacity) {
    if (capacity == 0) capacity = VIT_C100K_MAX_SLABS;

    vit_slab_pool_t* pool = (vit_slab_pool_t*)malloc(sizeof(vit_slab_pool_t));
    if (!pool) return NULL;

    pool->capacity = capacity;
    pool->allocated_count = 0;
    pool->free_head = capacity;

    // Allocate 64-byte aligned slabs array
#if defined(_WIN32) || defined(_WIN64)
    pool->slabs = (vit_connection_slab_t*)_aligned_malloc(capacity * sizeof(vit_connection_slab_t), 64);
#else
    if (posix_memalign((void**)&pool->slabs, 64, capacity * sizeof(vit_connection_slab_t)) != 0) {
        free(pool);
        return NULL;
    }
#endif

    pool->free_indices = (uint32_t*)malloc(capacity * sizeof(uint32_t));
    if (!pool->free_indices || !pool->slabs) {
        if (pool->slabs) {
#if defined(_MSC_VER)
            _aligned_free(pool->slabs);
#else
            free(pool->slabs);
#endif
        }
        if (pool->free_indices) free(pool->free_indices);
        free(pool);
        return NULL;
    }

    for (uint32_t i = 0; i < capacity; i++) {
        pool->free_indices[i] = capacity - 1 - i;
        memset(&pool->slabs[i], 0, sizeof(vit_connection_slab_t));
    }

#ifndef _WIN32
    pthread_mutex_init(&pool->mutex, NULL); // VRI-04: thread-safe slab alloc/free
#endif

    return pool;
}

vit_connection_slab_t* vit_slab_alloc(vit_slab_pool_t* pool) {
    if (!pool || pool->free_head == 0) return NULL;

#ifndef _WIN32
    pthread_mutex_lock(&pool->mutex);
    if (pool->free_head == 0) { pthread_mutex_unlock(&pool->mutex); return NULL; }
#endif
    uint32_t index = pool->free_indices[--pool->free_head];
    pool->allocated_count++;
    vit_connection_slab_t* slab = &pool->slabs[index];
    memset(slab, 0, sizeof(vit_connection_slab_t));
#ifndef _WIN32
    pthread_mutex_unlock(&pool->mutex);
#endif
    return slab;
}

void vit_slab_free(vit_slab_pool_t* pool, vit_connection_slab_t* slab) {
    if (!pool || !slab) return;
    ptrdiff_t diff = slab - pool->slabs;
    if (diff < 0 || diff >= (ptrdiff_t)pool->capacity) return;

#ifndef _WIN32
    pthread_mutex_lock(&pool->mutex);
#endif
    pool->free_indices[pool->free_head++] = (uint32_t)diff;
    if (pool->allocated_count > 0) pool->allocated_count--;
#ifndef _WIN32
    pthread_mutex_unlock(&pool->mutex);
#endif
}

size_t vit_slab_pool_memory_usage(const vit_slab_pool_t* pool) {
    if (!pool) return 0;
    size_t slab_mem = pool->capacity * sizeof(vit_connection_slab_t);
    size_t index_mem = pool->capacity * sizeof(uint32_t);
    return sizeof(vit_slab_pool_t) + slab_mem + index_mem;
}

void vit_slab_pool_destroy(vit_slab_pool_t* pool) {
    if (!pool) return;
#ifndef _WIN32
    pthread_mutex_destroy(&pool->mutex);
#endif
    if (pool->slabs) {
#if defined(_WIN32) || defined(_WIN64)
        _aligned_free(pool->slabs);
#else
        free(pool->slabs);
#endif
    }
    if (pool->free_indices) free(pool->free_indices);
    free(pool);
}

#if defined(_MSC_VER)
static __declspec(thread) vit_thread_arena_t tls_arena;
static __declspec(thread) bool tls_arena_inited = false;
#else
static __thread vit_thread_arena_t tls_arena;
static __thread bool tls_arena_inited = false;
#endif

vit_thread_arena_t* vit_thread_arena_get(void) {
    if (!tls_arena_inited) {
        tls_arena.offset = 0;
        tls_arena_inited = true;
    }
    return &tls_arena;
}

void* vit_thread_arena_alloc(size_t size) {
    vit_thread_arena_t* arena = vit_thread_arena_get();
    // Align size to 8 bytes
    size_t aligned_size = (size + 7) & ~((size_t)7);
    if (arena->offset + aligned_size > sizeof(arena->arena_buf)) {
        // Fallback to malloc for large allocations exceeding 64KB arena
        return malloc(aligned_size);
    }
    void* ptr = &arena->arena_buf[arena->offset];
    arena->offset += aligned_size;
    return ptr;
}

void vit_thread_arena_reset(void) {
    vit_thread_arena_t* arena = vit_thread_arena_get();
    arena->offset = 0;
}
