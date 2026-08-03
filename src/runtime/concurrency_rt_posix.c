// VIT Runtime - POSIX/pthread Concurrency Implementation
// Fix VRI-12: concurrency_rt.c was Windows-only. This file provides the Linux/macOS path.
// NativeCompiler should select this file on non-Windows platforms.

#ifndef _WIN32

#define _GNU_SOURCE
#include "concurrency_rt.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// ─── Thread ────────────────────────────────────────────────────────────────

typedef struct {
    void* (*func)(void*);
    void* arg;
} PosixThreadArg;

static void* posix_thread_proc(void* param) {
    PosixThreadArg* larg = (PosixThreadArg*)param;
    void* (*func)(void*) = larg->func;
    void* arg = larg->arg;
    free(larg);
    return func(arg);
}

void* vit_thread_spawn(void* (*func)(void*), void* arg) {
    PosixThreadArg* larg = (PosixThreadArg*)malloc(sizeof(PosixThreadArg));
    if (!larg) return NULL;
    larg->func = func;
    larg->arg = arg;
    pthread_t* t = (pthread_t*)malloc(sizeof(pthread_t));
    if (!t) { free(larg); return NULL; }
    if (pthread_create(t, NULL, posix_thread_proc, larg) != 0) {
        free(larg);
        free(t);
        return NULL;
    }
    return (void*)t;
}

void vit_thread_join(void* thread_ptr) {
    if (!thread_ptr) return;
    pthread_t* t = (pthread_t*)thread_ptr;
    pthread_join(*t, NULL);
    free(t);
}

void vit_thread_detach(void* thread_ptr) {
    if (!thread_ptr) return;
    pthread_t* t = (pthread_t*)thread_ptr;
    pthread_detach(*t);
    free(t);
}

// ─── Channel ───────────────────────────────────────────────────────────────

typedef struct {
    double*         data;
    size_t          capacity;
    size_t          head;
    size_t          tail;
    size_t          count;
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
} PosixChannel;

void* vit_channel_create(void) {
    PosixChannel* ch = (PosixChannel*)malloc(sizeof(PosixChannel));
    if (!ch) return NULL;
    ch->capacity = 1024;
    ch->data = (double*)malloc(sizeof(double) * ch->capacity);
    ch->head = 0;
    ch->tail = 0;
    ch->count = 0;
    pthread_mutex_init(&ch->mutex, NULL);
    pthread_cond_init(&ch->cond, NULL);
    return (void*)ch;
}

void vit_channel_send(void* channel_ptr, double val) {
    if (!channel_ptr) return;
    PosixChannel* ch = (PosixChannel*)channel_ptr;
    pthread_mutex_lock(&ch->mutex);
    if (ch->count < ch->capacity) {
        ch->data[ch->tail] = val;
        ch->tail = (ch->tail + 1) % ch->capacity;
        ch->count++;
    }
    pthread_cond_signal(&ch->cond);
    pthread_mutex_unlock(&ch->mutex);
}

double vit_channel_receive(void* channel_ptr) {
    if (!channel_ptr) return 0.0;
    PosixChannel* ch = (PosixChannel*)channel_ptr;
    pthread_mutex_lock(&ch->mutex);
    while (ch->count == 0) {
        pthread_cond_wait(&ch->cond, &ch->mutex);
    }
    double val = ch->data[ch->head];
    ch->head = (ch->head + 1) % ch->capacity;
    ch->count--;
    pthread_mutex_unlock(&ch->mutex);
    return val;
}

void vit_channel_free(void* channel_ptr) {
    if (!channel_ptr) return;
    PosixChannel* ch = (PosixChannel*)channel_ptr;
    pthread_mutex_destroy(&ch->mutex);
    pthread_cond_destroy(&ch->cond);
    free(ch->data);
    free(ch);
}

// ─── Promise ───────────────────────────────────────────────────────────────

typedef struct {
    int             state; // 0 = PENDING, 1 = RESOLVED
    double          value;
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
} PosixPromise;

void* vit_promise_create(void) {
    PosixPromise* p = (PosixPromise*)malloc(sizeof(PosixPromise));
    if (!p) return NULL;
    p->state = 0;
    p->value = 0.0;
    pthread_mutex_init(&p->mutex, NULL);
    pthread_cond_init(&p->cond, NULL);
    return (void*)p;
}

void vit_promise_resolve(void* promise_ptr, double val) {
    if (!promise_ptr) return;
    PosixPromise* p = (PosixPromise*)promise_ptr;
    pthread_mutex_lock(&p->mutex);
    p->value = val;
    p->state = 1;
    pthread_cond_broadcast(&p->cond);
    pthread_mutex_unlock(&p->mutex);
}

double vit_promise_await(void* promise_ptr) {
    if (!promise_ptr) return 0.0;
    PosixPromise* p = (PosixPromise*)promise_ptr;
    pthread_mutex_lock(&p->mutex);
    while (p->state == 0) {
        pthread_cond_wait(&p->cond, &p->mutex);
    }
    double val = p->value;
    pthread_mutex_unlock(&p->mutex);
    return val;
}

void vit_promise_free(void* promise_ptr) {
    if (!promise_ptr) return;
    PosixPromise* p = (PosixPromise*)promise_ptr;
    pthread_mutex_destroy(&p->mutex);
    pthread_cond_destroy(&p->cond);
    free(p);
}

// ─── Task (fire-and-forget thread) ─────────────────────────────────────────

typedef struct {
    void (*func)(void*);
    void* arg;
} PosixTaskArg;

static void* posix_task_proc(void* param) {
    PosixTaskArg* larg = (PosixTaskArg*)param;
    void (*func)(void*) = larg->func;
    void* arg = larg->arg;
    free(larg);
    func(arg);
    return NULL;
}

void vit_task_spawn(void (*func)(void*), void* arg) {
    PosixTaskArg* larg = (PosixTaskArg*)malloc(sizeof(PosixTaskArg));
    if (!larg) return;
    larg->func = func;
    larg->arg = arg;
    pthread_t t;
    if (pthread_create(&t, NULL, posix_task_proc, larg) == 0) {
        pthread_detach(t);
    } else {
        free(larg);
    }
}

#endif /* _WIN32 */
