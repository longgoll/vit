#ifndef VIT_CONCURRENCY_RT_H
#define VIT_CONCURRENCY_RT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Thread Primitives
void* vit_thread_spawn(void* (*func)(void*), void* arg);
void vit_thread_join(void* thread_ptr);
void vit_thread_detach(void* thread_ptr);

// Channel Primitives
void* vit_channel_create(void);
void vit_channel_send(void* channel_ptr, double val);
double vit_channel_receive(void* channel_ptr);
void vit_channel_free(void* channel_ptr);

// Promise & Async Task Primitives
void* vit_promise_create(void);
void vit_promise_resolve(void* promise_ptr, double val);
double vit_promise_await(void* promise_ptr);
void vit_promise_free(void* promise_ptr);
void vit_task_spawn(void (*func)(void*), void* arg);

#ifdef __cplusplus
}
#endif

#endif // VIT_CONCURRENCY_RT_H
