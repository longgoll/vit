#if defined(_WIN32) && !defined(_MM_MALLOC_H_INCLUDED)
#define _MM_MALLOC_H_INCLUDED 1
#endif
#include "net_rt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #ifndef _MM_MALLOC_H_INCLUDED
    #define _MM_MALLOC_H_INCLUDED 1
    #endif
    #ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
    #define _WINSOCK_DEPRECATED_NO_WARNINGS 1
    #endif
    #ifndef __X86INTRIN_H
    #define __X86INTRIN_H
    #endif
    #ifndef _X86INTRIN_H_INCLUDED
    #define _X86INTRIN_H_INCLUDED
    #endif
    #ifndef _X86INTRIN_H_
    #define _X86INTRIN_H_
    #endif
    #ifndef _EMMINTRIN_H_INCLUDED
    #define _EMMINTRIN_H_INCLUDED
    #endif
    #ifndef _EMMINTRIN_H_
    #define _EMMINTRIN_H_
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <fcntl.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <errno.h>
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket(s) close(s)
#endif

static int g_net_initialized = 0;

// Thread-local static buffer pool to eliminate dynamic malloc/free overhead per request
#define FAST_BUFFER_SIZE 8192
// VRI-05: _Thread_local requires C11; use __thread as GCC/Clang portable fallback
#if defined(_MSC_VER)
  #define VIT_THREAD_LOCAL __declspec(thread)
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
  #define VIT_THREAD_LOCAL _Thread_local
#else
  #define VIT_THREAD_LOCAL __thread
#endif
static VIT_THREAD_LOCAL char g_fast_recv_buf[FAST_BUFFER_SIZE + 1];

void vit_net_init(void) {
    if (g_net_initialized) return;
#ifdef _WIN32
    WSADATA wsaData;
    int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (res != 0) {
        fprintf(stderr, "[VIT Net Error] WSAStartup failed: %d\n", res);
    }
#endif
    g_net_initialized = 1;
}

void vit_net_cleanup(void) {
    if (!g_net_initialized) return;
#ifdef _WIN32
    WSACleanup();
#endif
    g_net_initialized = 0;
}

double vit_net_socket_create(void) {
    vit_net_init();
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
        return -1.0;
    }
    
    // Performance Optimizations: TCP_NODELAY & SO_REUSEADDR & SO_REUSEPORT (Linux)
    int opt = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#ifdef SO_REUSEPORT
    setsockopt(s, SOL_SOCKET, SO_REUSEPORT, (const char*)&opt, sizeof(opt));
#endif
    setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof(opt));
    
    // Maximize Socket Send/Receive Buffers (64KB)
    int buf_size = 65536;
    setsockopt(s, SOL_SOCKET, SO_RCVBUF, (const char*)&buf_size, sizeof(buf_size));
    setsockopt(s, SOL_SOCKET, SO_SNDBUF, (const char*)&buf_size, sizeof(buf_size));

    return (double)s;
}

double vit_net_socket_set_reuseport(double fd_dbl, double enable_dbl) {
    int fd = (int)fd_dbl;
    int opt = ((int)enable_dbl) ? 1 : 0;
#ifdef SO_REUSEPORT
    int res = setsockopt((SOCKET)fd, SOL_SOCKET, SO_REUSEPORT, (const char*)&opt, sizeof(opt));
    return (res == 0) ? 0.0 : -1.0;
#else
    return 0.0; // Graceful fallback on OS without SO_REUSEPORT (Windows)
#endif
}

double vit_net_socket_set_nonblocking(double fd_dbl, double enable_dbl) {
    int fd = (int)fd_dbl;
    int enable = (int)enable_dbl;
#ifdef _WIN32
    u_long mode = enable ? 1 : 0;
    int res = ioctlsocket((SOCKET)fd, FIONBIO, &mode);
    return (res == 0) ? 0.0 : -1.0;
#else
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1.0;
    flags = enable ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
    int res = fcntl(fd, F_SETFL, flags);
    return (res == 0) ? 0.0 : -1.0;
#endif
}

double vit_net_socket_bind(double fd_dbl, const char* host, double port_dbl) {
    vit_net_init();
    int fd = (int)fd_dbl;
    int port = (int)port_dbl;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);

    if (host == NULL || strlen(host) == 0 || strcmp(host, "0.0.0.0") == 0) {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        addr.sin_addr.s_addr = inet_addr(host);
    }

    int res = bind((SOCKET)fd, (struct sockaddr*)&addr, sizeof(addr));
    return (res == SOCKET_ERROR) ? -1.0 : 0.0;
}

double vit_net_socket_listen(double fd_dbl, double backlog_dbl) {
    int fd = (int)fd_dbl;
    int backlog = (int)backlog_dbl;
    if (backlog <= 0) backlog = 4096;
    int res = listen((SOCKET)fd, backlog);
    return (res == SOCKET_ERROR) ? -1.0 : 0.0;
}

double vit_net_socket_accept(double fd_dbl) {
    int fd = (int)fd_dbl;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    SOCKET client_fd = accept((SOCKET)fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd == INVALID_SOCKET) {
        return -1.0;
    }

    // Set TCP_NODELAY on accepted client socket
    int opt = 1;
    setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof(opt));

    return (double)client_fd;
}

double vit_net_socket_connect(double fd_dbl, const char* host, double port_dbl) {
    vit_net_init();
    int fd = (int)fd_dbl;
    int port = (int)port_dbl;
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    addr.sin_addr.s_addr = inet_addr(host);

    int res = connect((SOCKET)fd, (struct sockaddr*)&addr, sizeof(addr));
    return (res == SOCKET_ERROR) ? -1.0 : 0.0;
}

double vit_net_socket_send(double fd_dbl, const char* data, double len_dbl) {
    int fd = (int)fd_dbl;
    int len = (int)len_dbl;
    if (!data) return 0.0;
    if (len <= 0) len = (int)strlen(data);
    int bytes_sent = send((SOCKET)fd, data, len, 0);
    return (bytes_sent == SOCKET_ERROR) ? -1.0 : (double)bytes_sent;
}

double vit_net_send_raw(double fd_dbl, const char* data, double len_dbl) {
    return vit_net_socket_send(fd_dbl, data, len_dbl);
}

double vit_net_socket_recv(double fd_dbl, char* buf, double max_len_dbl) {
    int fd = (int)fd_dbl;
    int max_len = (int)max_len_dbl;
    if (!buf || max_len <= 0) return 0.0;
    int bytes_read = recv((SOCKET)fd, buf, max_len, 0);
    return (bytes_read == SOCKET_ERROR) ? -1.0 : (double)bytes_read;
}

void vit_net_socket_close(double fd_dbl) {
    int fd = (int)fd_dbl;
    if (fd >= 0) {
        closesocket((SOCKET)fd);
    }
}

// Optimized recv_string using thread-local static buffer pool (Zero Malloc)
char* vit_net_recv_string(double fd_dbl, double max_len_dbl) {
    int fd = (int)fd_dbl;
    int max_len = (int)max_len_dbl;
    if (max_len <= 0 || max_len > FAST_BUFFER_SIZE) max_len = FAST_BUFFER_SIZE;
    
    int bytes_read = recv((SOCKET)fd, g_fast_recv_buf, max_len, 0);
    if (bytes_read <= 0) {
        g_fast_recv_buf[0] = '\0';
    } else {
        g_fast_recv_buf[bytes_read] = '\0';
    }
    return g_fast_recv_buf;
}

// Keep-Alive Support: Enable SO_KEEPALIVE + TCP keepalive probes on a socket
// Returns 0.0 on success, -1.0 on error
double vit_net_socket_keepalive(double fd_dbl, double enable_dbl) {
    int fd = (int)fd_dbl;
    int opt = ((int)enable_dbl) ? 1 : 0;
    int res = setsockopt((SOCKET)fd, SOL_SOCKET, SO_KEEPALIVE, (const char*)&opt, sizeof(opt));
    if (res != 0) return -1.0;
#if defined(TCP_KEEPIDLE) && defined(TCP_KEEPINTVL) && defined(TCP_KEEPCNT)
    // Linux: idle=60s, interval=10s, count=3 probes
    int idle = 60, intvl = 10, cnt = 3;
    setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE,  (const char*)&idle,  sizeof(idle));
    setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, (const char*)&intvl, sizeof(intvl));
    setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT,   (const char*)&cnt,   sizeof(cnt));
#endif
    return 0.0;
}

// Non-blocking recv: trả về data nếu có sẵn, trả về "" ngay nếu không có data
// Dùng để poll connection trong Keep-Alive loop mà không block
char* vit_net_recv_nonblock(double fd_dbl, double max_len_dbl) {
    int fd = (int)fd_dbl;
    int max_len = (int)max_len_dbl;
    if (max_len <= 0 || max_len > FAST_BUFFER_SIZE) max_len = FAST_BUFFER_SIZE;

#ifdef _WIN32
    // Windows: dùng select với timeout = 0 để poll
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET((SOCKET)fd, &fds);
    struct timeval tv = {0, 0}; // timeout = 0 = non-blocking poll
    int ready = select(fd + 1, &fds, NULL, NULL, &tv);
    if (ready <= 0) { g_fast_recv_buf[0] = '\0'; return g_fast_recv_buf; }
#else
    // POSIX: MSG_DONTWAIT
    int bytes_read = recv((SOCKET)fd, g_fast_recv_buf, max_len, MSG_DONTWAIT);
    if (bytes_read <= 0) { g_fast_recv_buf[0] = '\0'; return g_fast_recv_buf; }
    g_fast_recv_buf[bytes_read] = '\0';
    return g_fast_recv_buf;
#endif
}

// Check if a socket is still connected (peer đã close chưa)
// Returns 1.0 = connected, 0.0 = disconnected/error
double vit_net_socket_is_connected(double fd_dbl) {
    int fd = (int)fd_dbl;
    if (fd < 0) return 0.0;
    char probe;
#ifdef _WIN32
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET((SOCKET)fd, &fds);
    struct timeval tv = {0, 0};
    int ready = select(fd + 1, &fds, NULL, NULL, &tv);
    if (ready < 0) return 0.0;
    if (ready == 0) return 1.0; // No data yet but socket is alive
    int r = recv((SOCKET)fd, &probe, 1, MSG_PEEK);
    return (r == 0) ? 0.0 : 1.0; // 0 = graceful close
#else
    int r = recv(fd, &probe, 1, MSG_PEEK | MSG_DONTWAIT);
    if (r == 0) return 0.0;             // graceful close
    if (r < 0) {
        int e = errno;
        if (e == EAGAIN || e == EWOULDBLOCK) return 1.0; // no data but alive
        return 0.0; // error = disconnected
    }
    return 1.0; // có data, còn sống
#endif
}

// RI-02 + RI-07: Get effective CPU count for multi-worker scaling.
// Reads cgroup v2 cpu.max quota first (Docker --cpus), falls back to sysconf.
// This prevents spawning 28 workers when container has --cpus=8, which wastes context switching.
double vit_sysconf_nprocs(void) {
#if defined(__linux__)
    // Try cgroup v2 cpu.max: "quota period\n" or "max period\n"
    FILE* f = fopen("/sys/fs/cgroup/cpu.max", "r");
    if (f) {
        char quota_str[32] = {0};
        long quota = -1, period = 100000;
        if (fscanf(f, "%31s %ld", quota_str, &period) == 2) {
            if (quota_str[0] != 'm') { // "max" means unlimited
                quota = atol(quota_str);
            }
        }
        fclose(f);
        if (quota > 0 && period > 0) {
            long cgroup_cores = (quota + period - 1) / period; // round up
            if (cgroup_cores > 0) return (double)cgroup_cores;
        }
    }

    long nprocs = sysconf(_SC_NPROCESSORS_ONLN);
    if (nprocs > 0) return (double)nprocs;
#elif defined(_WIN32)
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return (double)si.dwNumberOfProcessors;
#endif
    return 4.0; // safe fallback
}

