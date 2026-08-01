#ifndef VIT_ASYNC_IOURING_RT_H
#define VIT_ASYNC_IOURING_RT_H

#if defined(_WIN32) && !defined(_MM_MALLOC_H_INCLUDED)
#define _MM_MALLOC_H_INCLUDED 1
#endif

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VIT_IOURING_QUEUE_DEPTH 4096
#define VIT_IOURING_BUF_COUNT 1024
#define VIT_IOURING_BUF_SIZE 16384

typedef struct vit_iouring_config {
    uint32_t queue_depth;
    uint32_t flags;
    uint32_t sq_entries;
    uint32_t cq_entries;
} vit_iouring_config_t;

typedef struct vit_iouring {
    int ring_fd;
    void* sq_ring;
    void* cq_ring;
    void* sqes;
    uint32_t sq_entries;
    uint32_t cq_entries;
    int is_fallback;
} vit_iouring_t;

typedef struct vit_iouring_worker {
    int worker_id;
    int listen_fd;
    vit_iouring_t ring;
    void* thread_handle;
    int running;
} vit_iouring_worker_t;

typedef struct vit_iouring_worker_group {
    int num_workers;
    const char* host;
    int port;
    vit_iouring_worker_t* workers;
} vit_iouring_worker_group_t;

// io_uring Engine API
int vit_iouring_init(vit_iouring_t* ring, uint32_t depth);
int vit_iouring_submit_and_wait(vit_iouring_t* ring, uint32_t wait_nr);
int vit_iouring_cleanup(vit_iouring_t* ring);

// Zero-Copy Ring Buffer API
int vit_iouring_register_buffers(vit_iouring_t* ring, void* buffer_pool, size_t buf_count, size_t buf_size);

// Multi-Worker REUSEPORT Server Loop API
vit_iouring_worker_group_t* vit_iouring_group_create(const char* host, int port, int num_workers);
int vit_iouring_group_start(vit_iouring_worker_group_t* group, void (*handler)(int client_fd, const char* req, size_t len));
void vit_iouring_group_stop(vit_iouring_worker_group_t* group);

#ifdef __cplusplus
}
#endif

#endif // VIT_ASYNC_IOURING_RT_H
