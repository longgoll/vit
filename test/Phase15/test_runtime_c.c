#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "../../include/runtime/memory_rt.h"
#include "../../include/runtime/async_iouring_rt.h"
#include "../../include/runtime/http_parser_simd.h"

int main(void) {
    printf("[Phase 15 C Runtime Test] Initializing...\n");

    // 1. Test Request Arena Memory Allocator
    vit_arena_t arena;
    vit_arena_init(&arena, 1024);
    void* ptr1 = vit_arena_alloc(&arena, 64, 8);
    void* ptr2 = vit_arena_alloc(&arena, 128, 8);
    assert(ptr1 != NULL);
    assert(ptr2 != NULL);
    assert(ptr2 > ptr1);
    vit_arena_reset(&arena);
    void* ptr3 = vit_arena_alloc(&arena, 64, 8);
    assert(ptr3 == ptr1); // Reused same memory offset in 1ns
    vit_arena_destroy(&arena);
    printf("  [1/3] Memory Arena Allocator: PASS\n");

    // 2. Test io_uring Engine Initialization
    vit_iouring_t ring;
    int res = vit_iouring_init(&ring, 128);
    assert(res == 0);
    vit_iouring_cleanup(&ring);
    printf("  [2/3] io_uring Engine Setup: PASS\n");

    // 3. Test SIMD Zero-Alloc HTTP Parser
    const char* raw_req = "GET /api/v1/users HTTP/1.1\r\nHost: localhost:8080\r\nUser-Agent: VitClient/1.0\r\n\r\nHello World";
    vit_http_request_span_t req;
    int parse_res = vit_http_parse_simd(raw_req, strlen(raw_req), &req);
    assert(parse_res == 0);
    assert(vit_span_equals(req.method, "GET"));
    assert(vit_span_equals(req.path, "/api/v1/users"));
    assert(vit_span_equals(req.version, "HTTP/1.1"));
    assert(req.num_headers == 2);
    assert(vit_span_equals(req.headers[0].name, "Host"));
    assert(vit_span_equals(req.headers[0].value, "localhost:8080"));
    assert(vit_span_equals(req.body, "Hello World"));
    printf("  [3/3] SIMD HTTP Zero-Alloc Parser: PASS\n");

    printf("\n\033[32m[SUCCESS]\033[0m All Extreme Performance C Runtime tests passed!\n");
    return 0;
}
