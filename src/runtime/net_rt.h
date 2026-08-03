#ifndef VIT_NET_RT_H
#define VIT_NET_RT_H

#if defined(_WIN32) && !defined(_MM_MALLOC_H_INCLUDED)
#define _MM_MALLOC_H_INCLUDED 1
#endif

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Socket Subsystem Initialization
void vit_net_init(void);
void vit_net_cleanup(void);

// Non-blocking Socket Primitives
double vit_net_socket_create(void);
double vit_net_socket_set_nonblocking(double fd, double enable);
double vit_net_socket_set_reuseport(double fd, double enable);
double vit_net_socket_bind(double fd, const char* host, double port);
double vit_net_socket_listen(double fd, double backlog);
double vit_net_socket_accept(double fd);
double vit_net_socket_connect(double fd, const char* host, double port);
double vit_net_socket_send(double fd, const char* data, double len);
double vit_net_socket_recv(double fd, char* buf, double max_len);
void vit_net_socket_close(double fd);

// High-Performance Fast Buffer Recv & Static Fast Response
char* vit_net_recv_string(double fd, double max_len);
double vit_net_send_raw(double fd, const char* data, double len);

// Keep-Alive Support
double vit_net_socket_keepalive(double fd, double enable);
char*  vit_net_recv_nonblock(double fd, double max_len);
double vit_net_socket_is_connected(double fd);

// RI-02 + RI-07: Effective CPU count (cgroup quota-aware for Docker --cpus)
double vit_sysconf_nprocs(void);


#ifdef __cplusplus
}
#endif

#endif // VIT_NET_RT_H
