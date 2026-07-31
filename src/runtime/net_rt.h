#ifndef VIT_NET_RT_H
#define VIT_NET_RT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Socket Subsystem Initialization
void vit_net_init(void);
void vit_net_cleanup(void);

// Socket Primitives
int vit_net_socket_create(void);
int vit_net_socket_bind(int fd, const char* host, int port);
int vit_net_socket_listen(int fd, int backlog);
int vit_net_socket_accept(int fd);
int vit_net_socket_connect(int fd, const char* host, int port);
int vit_net_socket_send(int fd, const char* data, int len);
int vit_net_socket_recv(int fd, char* buf, int max_len);
void vit_net_socket_close(int fd);

// Helper Utilities
char* vit_net_recv_string(int fd, int max_len);

#ifdef __cplusplus
}
#endif

#endif // VIT_NET_RT_H
