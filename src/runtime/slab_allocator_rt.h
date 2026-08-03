#ifndef VIT_SLAB_ALLOCATOR_RT_H
#define VIT_SLAB_ALLOCATOR_RT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#ifndef _WIN32
#include <pthread.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Enforce 64-byte Cache-Line alignment to completely eliminate False Sharing across CPU cores
#if defined(_MSC_VER)
  #define VIT_CACHE_ALIGN __declspec(align(64))
#else
  #define VIT_CACHE_ALIGN __attribute__((aligned(64)))
#endif

#define VIT_C100K_SLAB_SIZE 256
#define VIT_C100K_MAX_SLABS 100000

typedef struct VIT_CACHE_ALIGN {
    uint32_t socket_fd;
    uint32_t flags;
    uint64_t last_active_ts;
    char rx_buffer[128]; // Compact inline buffer for idle connection state
} vit_connection_slab_t;

typedef struct VIT_CACHE_ALIGN {
    vit_connection_slab_t* slabs;
    uint32_t* free_indices;
    uint32_t free_head;
    uint32_t capacity;
    uint32_t allocated_count;
#ifndef _WIN32
    pthread_mutex_t mutex; // Protects free_head on multi-core (VRI-04)
#endif
} vit_slab_pool_t;

vit_slab_pool_t* vit_slab_pool_create(uint32_t capacity);
vit_connection_slab_t* vit_slab_alloc(vit_slab_pool_t* pool);
void vit_slab_free(vit_slab_pool_t* pool, vit_connection_slab_t* slab);
void vit_slab_pool_destroy(vit_slab_pool_t* pool);

typedef struct VIT_CACHE_ALIGN {
    char arena_buf[64 * 1024]; // 64KB per-thread lock-free arena
    size_t offset;
} vit_thread_arena_t;

vit_thread_arena_t* vit_thread_arena_get(void);
void* vit_thread_arena_alloc(size_t size);
void vit_thread_arena_reset(void);

#ifdef __cplusplus
}
#endif

#endif // VIT_SLAB_ALLOCATOR_RT_H
