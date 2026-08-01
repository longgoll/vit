#ifndef VIT_SLAB_ALLOCATOR_RT_H
#define VIT_SLAB_ALLOCATOR_RT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

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
} vit_slab_pool_t;

vit_slab_pool_t* vit_slab_pool_create(uint32_t capacity);
vit_connection_slab_t* vit_slab_alloc(vit_slab_pool_t* pool);
void vit_slab_free(vit_slab_pool_t* pool, vit_connection_slab_t* slab);
void vit_slab_pool_destroy(vit_slab_pool_t* pool);

size_t vit_slab_pool_memory_usage(const vit_slab_pool_t* pool);

#ifdef __cplusplus
}
#endif

#endif // VIT_SLAB_ALLOCATOR_RT_H
