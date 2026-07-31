#include "net_rt.h"
#include <stdio.h>
#include <string.h>

void* malloc(size_t size);
void free(void* ptr);
void exit(int status);
int atoi(const char* str);

#ifdef _WIN32
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
    #include <arpa/inet.h>
    #include <unistd.h>
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket(s) close(s)
#endif

static int g_net_initialized = 0;

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

int vit_net_socket_create(void) {
    vit_net_init();
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
        return -1;
    }
    // Allow address reuse
    int opt = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    return (int)s;
}

int vit_net_socket_bind(int fd, const char* host, int port) {
    vit_net_init();
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);

    if (host == NULL || strlen(host) == 0 || strcmp(host, "0.0.0.0") == 0) {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        inet_pton(AF_INET, host, &addr.sin_addr);
    }

    int res = bind((SOCKET)fd, (struct sockaddr*)&addr, sizeof(addr));
    return (res == SOCKET_ERROR) ? -1 : 0;
}

int vit_net_socket_listen(int fd, int backlog) {
    if (backlog <= 0) backlog = 128;
    int res = listen((SOCKET)fd, backlog);
    return (res == SOCKET_ERROR) ? -1 : 0;
}

int vit_net_socket_accept(int fd) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    SOCKET client_fd = accept((SOCKET)fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd == INVALID_SOCKET) {
        return -1;
    }
    return (int)client_fd;
}

int vit_net_socket_connect(int fd, const char* host, int port) {
    vit_net_init();
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    inet_pton(AF_INET, host, &addr.sin_addr);

    int res = connect((SOCKET)fd, (struct sockaddr*)&addr, sizeof(addr));
    return (res == SOCKET_ERROR) ? -1 : 0;
}

int vit_net_socket_send(int fd, const char* data, int len) {
    if (!data) return 0;
    if (len <= 0) len = (int)strlen(data);
    int bytes_sent = send((SOCKET)fd, data, len, 0);
    return (bytes_sent == SOCKET_ERROR) ? -1 : bytes_sent;
}

int vit_net_socket_recv(int fd, char* buf, int max_len) {
    if (!buf || max_len <= 0) return 0;
    int bytes_read = recv((SOCKET)fd, buf, max_len, 0);
    return (bytes_read == SOCKET_ERROR) ? -1 : bytes_read;
}

void vit_net_socket_close(int fd) {
    if (fd >= 0) {
        closesocket((SOCKET)fd);
    }
}

char* vit_net_recv_string(int fd, int max_len) {
    if (max_len <= 0) max_len = 4096;
    char* buf = (char*)malloc(max_len + 1);
    if (!buf) return NULL;
    int bytes_read = vit_net_socket_recv(fd, buf, max_len);
    if (bytes_read <= 0) {
        buf[0] = '\0';
        return buf;
    }
    buf[bytes_read] = '\0';
    return buf;
}
