#ifndef VIT_KERNEL_BYPASS_RT_H
#define VIT_KERNEL_BYPASS_RT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    VIT_IO_BACKEND_SELECT = 0,
    VIT_IO_BACKEND_EPOLL,
    VIT_IO_BACKEND_IOCP,
    VIT_IO_BACKEND_IO_URING,  // Linux SQPOLL Zero-Syscall Ring
    VIT_IO_BACKEND_WINDOWS_RIO // Windows Registered I/O Kernel Bypass
} vit_io_backend_t;

typedef struct {
    vit_io_backend_t backend;
    bool sqpoll_enabled;
    uint32_t ring_size;
} vit_io_config_t;

vit_io_backend_t vit_io_detect_best_backend(void);
bool vit_io_kernel_bypass_init(const vit_io_config_t* config);
const char* vit_io_backend_name(vit_io_backend_t backend);

#ifdef __cplusplus
}
#endif

#endif // VIT_KERNEL_BYPASS_RT_H
