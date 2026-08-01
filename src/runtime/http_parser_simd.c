#if defined(_WIN32) && !defined(_MM_MALLOC_H_INCLUDED)
#define _MM_MALLOC_H_INCLUDED 1
#endif
#include "runtime/http_parser_simd.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#if defined(__AVX2__) && defined(__has_include)
#if __has_include(<immintrin.h>)
#include <immintrin.h>
#define HAS_AVX2_SIMD 1
#endif
#endif

// 64-bit Vectorized Byte Matcher (SWAR)
static inline const char* find_char_swar64(const char* buf, const char* end, char target) {
    const char* p = buf;
    uint64_t target_mask = (uint64_t)(unsigned char)target * 0x0101010101010101ULL;

    while (p + 8 <= end) {
        uint64_t val;
        memcpy(&val, p, 8);
        uint64_t xor_val = val ^ target_mask;
        uint64_t has_zero = (xor_val - 0x0101010101010101ULL) & ~xor_val & 0x8080808080808080ULL;
        if (has_zero) {
            for (int i = 0; i < 8; i++) {
                if (p[i] == target) return p + i;
            }
        }
        p += 8;
    }
    while (p < end) {
        if (*p == target) return p;
        p++;
    }
    return NULL;
}

static const char* find_char_simd(const char* buf, const char* end, char target) {
#if defined(HAS_AVX2_SIMD)
    const char* p = buf;
    __m256i target_vec = _mm256_set1_epi8(target);
    while (p + 32 <= end) {
        __m256i chunk = _mm256_loadu_si256((const __m256i*)p);
        __m256i cmp = _mm256_cmpeq_epi8(chunk, target_vec);
        int mask = _mm256_movemask_epi8(cmp);
        if (mask != 0) {
#if defined(_MSC_VER) && !defined(__clang__)
            unsigned long idx;
            _BitScanForward(&idx, (unsigned long)mask);
            return p + idx;
#else
            int idx = __builtin_ctz((unsigned int)mask);
            return p + idx;
#endif
        }
        p += 32;
    }
    return find_char_swar64(p, end, target);
#else
    return find_char_swar64(buf, end, target);
#endif
}

int vit_http_parse_simd(const char* buf, size_t len, vit_http_request_span_t* req) {
    if (!buf || len == 0 || !req) return -1;
    memset(req, 0, sizeof(vit_http_request_span_t));

    const char* p = buf;
    const char* end = buf + len;

    // 1. Parse Method (e.g., GET, POST)
    const char* sp1 = find_char_simd(p, end, ' ');
    if (!sp1) return -1;
    req->method.ptr = p;
    req->method.len = (size_t)(sp1 - p);

    // 2. Parse URI Path (e.g., /api/users)
    p = sp1 + 1;
    const char* sp2 = find_char_simd(p, end, ' ');
    if (!sp2) return -1;
    req->path.ptr = p;
    req->path.len = (size_t)(sp2 - p);

    // 3. Parse HTTP Version (e.g., HTTP/1.1)
    p = sp2 + 1;
    const char* cr1 = find_char_simd(p, end, '\r');
    if (!cr1 || cr1 + 1 >= end || *(cr1 + 1) != '\n') return -1;
    req->version.ptr = p;
    req->version.len = (size_t)(cr1 - p);

    // 4. Parse Headers
    p = cr1 + 2;
    while (p < end && req->num_headers < 32) {
        if (p + 1 < end && *p == '\r' && *(p + 1) == '\n') {
            p += 2;
            break; // End of HTTP Headers
        }

        const char* colon = find_char_simd(p, end, ':');
        if (!colon) break;

        const char* line_end = find_char_simd(colon, end, '\r');
        if (!line_end || line_end + 1 >= end || *(line_end + 1) != '\n') break;

        vit_http_header_span_t* header = &req->headers[req->num_headers++];
        header->name.ptr = p;
        header->name.len = (size_t)(colon - p);

        const char* val_start = colon + 1;
        while (val_start < line_end && (*val_start == ' ' || *val_start == '\t')) {
            val_start++;
        }
        header->value.ptr = val_start;
        header->value.len = (size_t)(line_end - val_start);

        p = line_end + 2;
    }

    // 5. Remaining bytes form body span
    if (p < end) {
        req->body.ptr = p;
        req->body.len = (size_t)(end - p);
    } else {
        req->body.ptr = NULL;
        req->body.len = 0;
    }

    return 0;
}

int vit_span_equals(vit_string_span_t span, const char* str) {
    if (!str) return 0;
    size_t slen = strlen(str);
    if (span.len != slen) return 0;
    return strncmp(span.ptr, str, slen) == 0;
}

int vit_span_to_string(vit_string_span_t span, char* out_buf, size_t max_len) {
    if (!out_buf || max_len == 0) return 0;
    size_t copy_len = span.len < (max_len - 1) ? span.len : (max_len - 1);
    if (span.ptr && copy_len > 0) {
        memcpy(out_buf, span.ptr, copy_len);
    }
    out_buf[copy_len] = '\0';
    return (int)copy_len;
}
