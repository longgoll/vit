#ifndef VIT_HTTP_PARSER_SIMD_H
#define VIT_HTTP_PARSER_SIMD_H

#if defined(_WIN32) && !defined(_MM_MALLOC_H_INCLUDED)
#define _MM_MALLOC_H_INCLUDED 1
#endif

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vit_string_span {
    const char* ptr;
    size_t len;
} vit_string_span_t;

typedef struct vit_http_header_span {
    vit_string_span_t name;
    vit_string_span_t value;
} vit_http_header_span_t;

typedef struct vit_http_request_span {
    vit_string_span_t method;
    vit_string_span_t path;
    vit_string_span_t version;
    vit_http_header_span_t headers[32];
    size_t num_headers;
    vit_string_span_t body;
} vit_http_request_span_t;

// Vectorized SIMD Fast Scanning HTTP Request Parser (parses ONE request)
int vit_http_parse_simd(const char* buf, size_t len, vit_http_request_span_t* req);

// VRI-06: Batch pipelining parser — parse all HTTP/1.1 pipelined requests in a single buffer.
// Returns number of requests parsed. Each req_out[i] points into buf (zero-copy spans).
// next_offset_out receives the byte offset of unprocessed data after the last complete request.
// max_requests: maximum entries in req_out array.
int vit_http_parse_simd_batch(
    const char* buf, size_t len,
    vit_http_request_span_t* req_out, int max_requests,
    size_t* next_offset_out
);

// Zero-copy Span comparison utilities
int vit_span_equals(vit_string_span_t span, const char* str);
int vit_span_to_string(vit_string_span_t span, char* out_buf, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif // VIT_HTTP_PARSER_SIMD_H
