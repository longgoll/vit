#define _GNU_SOURCE
#if defined(_WIN32) && !defined(_MM_MALLOC_H_INCLUDED)
#define _MM_MALLOC_H_INCLUDED 1
#endif
#include "runtime/async_iouring_rt.h"
#include "runtime/memory_rt.h"
#if __has_include("runtime/net_rt.h")
#include "runtime/net_rt.h"
#else
#include "net_rt.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#endif

#if defined(__linux__)
#include <sys/epoll.h>
#include <sys/syscall.h>
#include <sys/mman.h>

#ifndef __NR_io_uring_setup
#define __NR_io_uring_setup 425
#define __NR_io_uring_enter 426
#define __NR_io_uring_register 427
#endif

static long sys_io_uring_setup(uint32_t entries, void* p) {
    return syscall(__NR_io_uring_setup, entries, p);
}

static long sys_io_uring_enter(int fd, uint32_t to_submit, uint32_t min_complete, uint32_t flags, void* sig) {
    return syscall(__NR_io_uring_enter, fd, to_submit, min_complete, flags, sig, 0);
}

static long sys_io_uring_register(int fd, unsigned int opcode, void* arg, unsigned int nr_args) {
    return syscall(__NR_io_uring_register, fd, opcode, arg, nr_args);
}
#endif

typedef struct worker_arg {
    vit_iouring_worker_t* worker;
    void (*handler)(int client_fd, const char* req, size_t len);
} worker_arg_t;

#if defined(__linux__)

#define MAX_EVENTS 2048

static void set_nonblocking_fd(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
}

static void* worker_thread_loop(void* arg_ptr) {
    worker_arg_t* warg = (worker_arg_t*)arg_ptr;
    vit_iouring_worker_t* worker = warg->worker;
    void (*handler)(int client_fd, const char* req, size_t len) = warg->handler;
    free(warg);

#if defined(__linux__)
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    long num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (num_cores > 0) {
        CPU_SET(worker->worker_id % num_cores, &cpuset);
        pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    }
#endif

    int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) return 0;

    set_nonblocking_fd(worker->listen_fd);

    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = worker->listen_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, worker->listen_fd, &ev);

    char buf[16384];

    while (worker->running) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, 50);
        for (int n = 0; n < nfds; n++) {
            int fd = events[n].data.fd;
            if (fd == worker->listen_fd) {
                // Accept all incoming non-blocking client connections
                while (1) {
                    struct sockaddr_in client_addr;
                    socklen_t addr_len = sizeof(client_addr);
                    int client_fd = accept(worker->listen_fd, (struct sockaddr*)&client_addr, &addr_len);
                    if (client_fd < 0) break;

                    set_nonblocking_fd(client_fd);
                    struct epoll_event client_ev;
                    client_ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
                    client_ev.data.fd = client_fd;
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_ev);
                }
            } else {
                // High-performance async socket handler
                while (1) {
                    ssize_t bytes_read = recv(fd, buf, sizeof(buf) - 1, 0);
                    if (bytes_read > 0) {
                        buf[bytes_read] = '\0';
                        handler(fd, buf, (size_t)bytes_read);

                        // Re-arm connection event for HTTP Keep-Alive
                        struct epoll_event client_ev;
                        client_ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
                        client_ev.data.fd = fd;
                        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &client_ev);
                    } else if (bytes_read == 0 || (bytes_read < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                        close(fd);
                        break;
                    } else {
                        break;
                    }
                }
            }
        }
    }
    close(epoll_fd);
    return 0;
}

#else

#ifdef _WIN32
static DWORD WINAPI worker_thread_loop(LPVOID arg_ptr)
#else
static void* worker_thread_loop(void* arg_ptr)
#endif
{
    worker_arg_t* warg = (worker_arg_t*)arg_ptr;
    vit_iouring_worker_t* worker = warg->worker;
    void (*handler)(int client_fd, const char* req, size_t len) = warg->handler;
    free(warg);

    char buf[16384];

    while (worker->running) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_fd = (int)accept(worker->listen_fd, (struct sockaddr*)&client_addr, &addr_len);
        if (client_fd >= 0) {
            while (worker->running) {
                int n = (int)recv(client_fd, buf, sizeof(buf) - 1, 0);
                if (n <= 0) break;
                buf[n] = '\0';
                handler(client_fd, buf, (size_t)n);
            }
#ifdef _WIN32
            closesocket(client_fd);
#else
            close(client_fd);
#endif
        } else {
#ifdef _WIN32
            Sleep(1);
#else
            usleep(100);
#endif
        }
    }
    return 0;
}

#endif

#ifndef IORING_SETUP_SQPOLL
#define IORING_SETUP_SQPOLL (1U << 1)
#endif

int vit_iouring_init(vit_iouring_t* ring, uint32_t depth) {
    if (!ring) return -1;
    memset(ring, 0, sizeof(vit_iouring_t));
    if (depth == 0) depth = VIT_IOURING_QUEUE_DEPTH;

#if defined(__linux__)
    // VRI-02: Use correct kernel io_uring_params ABI (matching linux/io_uring.h).
    // sq_off and cq_off are nested structs (struct io_sqring_offsets = 40 bytes each),
    // NOT uint32_t[10]. Using the correct layout prevents EFAULT on io_uring_setup syscall.
    struct vit_io_sqring_offsets {
        uint32_t head;
        uint32_t tail;
        uint32_t ring_mask;
        uint32_t ring_entries;
        uint32_t flags;
        uint32_t dropped;
        uint32_t array;
        uint32_t resv1;
        uint64_t resv2;
    };

    struct vit_io_cqring_offsets {
        uint32_t head;
        uint32_t tail;
        uint32_t ring_mask;
        uint32_t ring_entries;
        uint32_t overflow;
        uint32_t cqes;
        uint64_t resv[2];
    };

    struct vit_io_uring_params {
        uint32_t sq_entries;
        uint32_t cq_entries;
        uint32_t flags;
        uint32_t sq_thread_cpu;
        uint32_t sq_thread_idle;
        uint32_t features;
        uint32_t wq_fd;
        uint32_t resv[3];
        struct vit_io_sqring_offsets sq_off;
        struct vit_io_cqring_offsets cq_off;
    } p;
    memset(&p, 0, sizeof(p));

    // Attempt SQPOLL zero-syscall kernel polling thread
    p.flags = IORING_SETUP_SQPOLL;
    p.sq_thread_idle = 2000;
    int res = sys_io_uring_setup(depth, &p);

    if (res < 0) {
        // Fallback to standard io_uring if SQPOLL permission is ungranted
        memset(&p, 0, sizeof(p));
        res = sys_io_uring_setup(depth, &p);
    }

    if (res >= 0) {
        ring->ring_fd = res;
        ring->sq_entries = p.sq_entries;
        ring->cq_entries = p.cq_entries;
        ring->is_fallback = 0;
        return 0;
    }
#endif

    ring->ring_fd = -1;
    ring->sq_entries = depth;
    ring->cq_entries = depth;
    ring->is_fallback = 1;
    return 0;
}


int vit_iouring_submit_and_wait(vit_iouring_t* ring, uint32_t wait_nr) {
    if (!ring) return -1;
    if (ring->is_fallback) {
        return 0;
    }

#if defined(__linux__)
    return (int)sys_io_uring_enter(ring->ring_fd, 1, wait_nr, 1, NULL);
#else
    (void)wait_nr;
    return 0;
#endif
}

int vit_iouring_cleanup(vit_iouring_t* ring) {
    if (!ring) return -1;
#if defined(__linux__)
    if (ring->ring_fd >= 0) {
        close(ring->ring_fd);
        ring->ring_fd = -1;
    }
#endif
    return 0;
}

int vit_iouring_register_buffers(vit_iouring_t* ring, void* buffer_pool, size_t buf_count, size_t buf_size) {
    if (!ring) return -1;
    (void)buffer_pool;
    (void)buf_count;
    (void)buf_size;
    if (ring->is_fallback) {
        return 0;
    }
#if defined(__linux__)
    return (int)sys_io_uring_register(ring->ring_fd, 0, buffer_pool, (unsigned int)buf_count);
#else
    return 0;
#endif
}

vit_iouring_worker_group_t* vit_iouring_group_create(const char* host, int port, int num_workers) {
    if (num_workers <= 0) num_workers = 4;
    vit_iouring_worker_group_t* group = (vit_iouring_worker_group_t*)malloc(sizeof(vit_iouring_worker_group_t));
    if (!group) return NULL;

    group->host = host ? host : "0.0.0.0";
    group->port = port;
    group->num_workers = num_workers;
    group->workers = (vit_iouring_worker_t*)calloc(num_workers, sizeof(vit_iouring_worker_t));

    for (int i = 0; i < num_workers; i++) {
        group->workers[i].worker_id = i;
        group->workers[i].listen_fd = -1;
        vit_iouring_init(&group->workers[i].ring, VIT_IOURING_QUEUE_DEPTH);
    }

    return group;
}

int vit_iouring_group_start(vit_iouring_worker_group_t* group, void (*handler)(int client_fd, const char* req, size_t len)) {
    if (!group || !handler) return -1;

    vit_net_init();
    for (int i = 0; i < group->num_workers; i++) {
        double fd = vit_net_socket_create();
        if (fd < 0) continue;
        vit_net_socket_set_reuseport(fd, 1);
        vit_net_socket_set_nonblocking(fd, 1);
        if (vit_net_socket_bind(fd, group->host, (double)group->port) == 0) {
            vit_net_socket_listen(fd, 8192);
            group->workers[i].listen_fd = (int)fd;
            group->workers[i].running = 1;

            worker_arg_t* warg = (worker_arg_t*)malloc(sizeof(worker_arg_t));
            warg->worker = &group->workers[i];
            warg->handler = handler;

#ifdef _WIN32
            group->workers[i].thread_handle = CreateThread(NULL, 0, worker_thread_loop, warg, 0, NULL);
#else
            pthread_t thread;
            pthread_create(&thread, NULL, worker_thread_loop, warg);
            group->workers[i].thread_handle = (void*)thread;
#endif
        }
    }
    return 0;
}

void vit_iouring_group_stop(vit_iouring_worker_group_t* group) {
    if (!group) return;
    // Signal all workers to stop
    for (int i = 0; i < group->num_workers; i++) {
        if (group->workers[i].running) {
            group->workers[i].running = 0;
            if (group->workers[i].listen_fd >= 0) {
                vit_net_socket_close((double)group->workers[i].listen_fd);
                group->workers[i].listen_fd = -1;
            }
        }
    }
    // Join all threads before freeing memory (VRI-03: prevent UAF)
    for (int i = 0; i < group->num_workers; i++) {
        if (group->workers[i].thread_handle) {
#ifdef _WIN32
            WaitForSingleObject((HANDLE)group->workers[i].thread_handle, 5000);
            CloseHandle((HANDLE)group->workers[i].thread_handle);
#else
            pthread_join((pthread_t)(uintptr_t)group->workers[i].thread_handle, NULL);
#endif
            group->workers[i].thread_handle = NULL;
        }
        vit_iouring_cleanup(&group->workers[i].ring);
    }
    free(group->workers);
    free(group);
    vit_net_cleanup();
}

