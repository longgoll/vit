#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include <sys/socket.h>
#include <unistd.h>
#else
#include <windows.h>
#endif

#include "runtime/memory_rt.h"
#include "runtime/async_iouring_rt.h"
#include "runtime/http_parser_simd.h"

static const char* HTTP_200_JSON = 
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: application/json\r\n"
    "Content-Length: 27\r\n"
    "Connection: keep-alive\r\n\r\n"
    "{\"message\":\"Hello, World!\"}";

static void handle_client(int client_fd, const char* raw_req, size_t req_len) {
    vit_arena_t* arena = vit_arena_get_thread_local();

    // 1. Zero-Allocation SIMD HTTP Header Parsing
    vit_http_request_span_t req;
    if (vit_http_parse_simd(raw_req, req_len, &req) == 0) {
        // 2. Request-Scoped Arena Allocation
        char* resp_buf = (char*)vit_arena_alloc(arena, 256, 8);
        if (resp_buf) {
            memcpy(resp_buf, HTTP_200_JSON, strlen(HTTP_200_JSON));
            send(client_fd, resp_buf, (int)strlen(HTTP_200_JSON), 0);
        }
    }

    // 3. 1ns Request-Scoped Arena Reset (0 bytes malloc/free per request)
    vit_arena_reset(arena);
}

int main(int argc, char** argv) {
    int port = 8080;
    int workers = 4;
    if (argc > 1) port = atoi(argv[1]);
    if (argc > 2) workers = atoi(argv[2]);

    printf("[Extreme Perf Engine] Starting io_uring multi-worker server on port %d with %d threads...\n", port, workers);

    vit_iouring_worker_group_t* group = vit_iouring_group_create("0.0.0.0", port, workers);
    if (!group) {
        fprintf(stderr, "[Error] Failed to create io_uring worker group\n");
        return 1;
    }

    if (vit_iouring_group_start(group, handle_client) != 0) {
        fprintf(stderr, "[Error] Failed to start io_uring worker group\n");
        return 1;
    }

    printf("[Extreme Perf Engine] Server running on http://0.0.0.0:%d\n", port);
    
    while (1) {
#ifdef _WIN32
        Sleep(1000);
#else
        sleep(1);
#endif
    }

    vit_iouring_group_stop(group);
    return 0;
}
