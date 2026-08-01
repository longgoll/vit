#ifndef VIT_MEMORY_RT_H
#define VIT_MEMORY_RT_H

#if defined(_WIN32) && !defined(_MM_MALLOC_H_INCLUDED)
#define _MM_MALLOC_H_INCLUDED 1
#endif

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VIT_ARENA_DEFAULT_BLOCK_SIZE (64 * 1024) // 64KB per block

typedef struct vit_arena_block {
    char* memory;
    size_t capacity;
    size_t offset;
    struct vit_arena_block* next;
} vit_arena_block_t;

typedef struct vit_arena {
    vit_arena_block_t* head;
    vit_arena_block_t* current;
    size_t default_block_size;
    size_t total_allocated;
} vit_arena_t;

// Arena Allocation API
void vit_arena_init(vit_arena_t* arena, size_t default_block_size);
void* vit_arena_alloc(vit_arena_t* arena, size_t size, size_t align);
void vit_arena_reset(vit_arena_t* arena);
void vit_arena_destroy(vit_arena_t* arena);

// Thread-Local Request-Scoped Arena API for 1ns request resets
vit_arena_t* vit_arena_get_thread_local(void);
void* vit_req_alloc(size_t size);
void vit_req_reset(void);

// Scope / Lifetime Ref-Count Suppression Markers (@no_arc hint support)
void vit_no_arc_scope_enter(void);
void vit_no_arc_scope_exit(void);

#ifdef __cplusplus
}
#endif

#endif // VIT_MEMORY_RT_H
