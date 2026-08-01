#if defined(_WIN32) && !defined(_MM_MALLOC_H_INCLUDED)
#define _MM_MALLOC_H_INCLUDED 1
#endif
#include "runtime/memory_rt.h"
#include <stdlib.h>
#include <string.h>

#if defined(_MSC_VER)
#define THREAD_LOCAL __declspec(thread)
#elif defined(__GNUC__) || defined(__clang__)
#define THREAD_LOCAL __thread
#else
#define THREAD_LOCAL _Thread_local
#endif

static THREAD_LOCAL vit_arena_t g_tls_arena = {0, 0, 0, 0};
static THREAD_LOCAL int g_tls_arena_inited = 0;

static vit_arena_block_t* create_arena_block(size_t capacity) {
    vit_arena_block_t* block = (vit_arena_block_t*)malloc(sizeof(vit_arena_block_t));
    if (!block) return NULL;
    block->memory = (char*)malloc(capacity);
    if (!block->memory) {
        free(block);
        return NULL;
    }
    block->capacity = capacity;
    block->offset = 0;
    block->next = NULL;
    return block;
}

void vit_arena_init(vit_arena_t* arena, size_t default_block_size) {
    if (!arena) return;
    if (default_block_size == 0) {
        default_block_size = VIT_ARENA_DEFAULT_BLOCK_SIZE;
    }
    arena->default_block_size = default_block_size;
    arena->head = create_arena_block(default_block_size);
    arena->current = arena->head;
    arena->total_allocated = arena->head ? default_block_size : 0;
}

void* vit_arena_alloc(vit_arena_t* arena, size_t size, size_t align) {
    if (!arena || size == 0) return NULL;
    if (align < 1) align = 8;

    if (!arena->current) {
        vit_arena_init(arena, 0);
        if (!arena->current) return NULL;
    }

    vit_arena_block_t* cur = arena->current;
    size_t current_addr = (size_t)(cur->memory + cur->offset);
    size_t aligned_addr = (current_addr + (align - 1)) & ~(align - 1);
    size_t padding = aligned_addr - current_addr;

    if (cur->offset + padding + size <= cur->capacity) {
        cur->offset += padding + size;
        return (void*)aligned_addr;
    }

    // Allocate new block if current block is full
    size_t new_cap = arena->default_block_size;
    if (size + align > new_cap) {
        new_cap = size + align + 4096;
    }

    vit_arena_block_t* new_block = create_arena_block(new_cap);
    if (!new_block) return NULL;

    cur->next = new_block;
    arena->current = new_block;
    arena->total_allocated += new_cap;

    current_addr = (size_t)new_block->memory;
    aligned_addr = (current_addr + (align - 1)) & ~(align - 1);
    padding = aligned_addr - current_addr;
    new_block->offset = padding + size;

    return (void*)aligned_addr;
}

void vit_arena_reset(vit_arena_t* arena) {
    if (!arena) return;
    vit_arena_block_t* cur = arena->head;
    while (cur) {
        cur->offset = 0;
        cur = cur->next;
    }
    arena->current = arena->head;
}

void vit_arena_destroy(vit_arena_t* arena) {
    if (!arena) return;
    vit_arena_block_t* cur = arena->head;
    while (cur) {
        vit_arena_block_t* next = cur->next;
        if (cur->memory) free(cur->memory);
        free(cur);
        cur = next;
    }
    arena->head = NULL;
    arena->current = arena->head;
    arena->total_allocated = 0;
}

vit_arena_t* vit_arena_get_thread_local(void) {
    if (!g_tls_arena_inited) {
        vit_arena_init(&g_tls_arena, VIT_ARENA_DEFAULT_BLOCK_SIZE);
        g_tls_arena_inited = 1;
    }
    return &g_tls_arena;
}

void* vit_req_alloc(size_t size) {
    vit_arena_t* arena = vit_arena_get_thread_local();
    return vit_arena_alloc(arena, size, 8);
}

void vit_req_reset(void) {
    if (g_tls_arena_inited) {
        vit_arena_reset(&g_tls_arena);
    }
}

void vit_no_arc_scope_enter(void) {
    // Marker for @no_arc scope entry (runtime hook)
}

void vit_no_arc_scope_exit(void) {
    // Marker for @no_arc scope exit (runtime hook)
}
