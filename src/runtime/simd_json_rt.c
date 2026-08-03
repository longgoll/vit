#include "simd_json_rt.h"
#include "slab_allocator_rt.h"  // VRI-10: vit_thread_arena_alloc for zero-malloc JSON parse path
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#endif

// Structural Indexing: Scans 32-byte chunks using SIMD (AVX2 if available, vector fallback otherwise)
size_t vit_simd_json_find_structural_indexes(const char* buf, size_t len, uint32_t* index_out, size_t max_indexes) {
    size_t count = 0;
    size_t i = 0;

#if defined(__AVX2__)
    const __m256i v_brace_o = _mm256_set1_epi8('{');
    const __m256i v_brace_c = _mm256_set1_epi8('}');
    const __m256i v_bracket_o = _mm256_set1_epi8('[');
    const __m256i v_bracket_c = _mm256_set1_epi8(']');
    const __m256i v_colon = _mm256_set1_epi8(':');
    const __m256i v_comma = _mm256_set1_epi8(',');
    const __m256i v_quote = _mm256_set1_epi8('"');

    for (; i + 32 <= len && count < max_indexes; i += 32) {
        __m256i chunk = _mm256_loadu_si256((const __m256i*)(buf + i));
        __m256i eq_brace_o = _mm256_cmpeq_epi8(chunk, v_brace_o);
        __m256i eq_brace_c = _mm256_cmpeq_epi8(chunk, v_brace_c);
        __m256i eq_bracket_o = _mm256_cmpeq_epi8(chunk, v_bracket_o);
        __m256i eq_bracket_c = _mm256_cmpeq_epi8(chunk, v_bracket_c);
        __m256i eq_colon = _mm256_cmpeq_epi8(chunk, v_colon);
        __m256i eq_comma = _mm256_cmpeq_epi8(chunk, v_comma);
        __m256i eq_quote = _mm256_cmpeq_epi8(chunk, v_quote);

        __m256i struct_mask_vec = _mm256_or_si256(
            _mm256_or_si256(_mm256_or_si256(eq_brace_o, eq_brace_c), _mm256_or_si256(eq_bracket_o, eq_bracket_c)),
            _mm256_or_si256(_mm256_or_si256(eq_colon, eq_comma), eq_quote)
        );

        uint32_t bitmask = (uint32_t)_mm256_movemask_epi8(struct_mask_vec);
        while (bitmask != 0 && count < max_indexes) {
#if defined(_MSC_VER)
            unsigned long bit_pos;
            _BitScanForward(&bit_pos, bitmask);
            index_out[count++] = (uint32_t)(i + bit_pos);
            bitmask &= bitmask - 1;
#else
            int bit_pos = __builtin_ctz(bitmask);
            index_out[count++] = (uint32_t)(i + bit_pos);
            bitmask &= bitmask - 1;
#endif
        }
    }
#endif

    // Fallback scalar scan for remaining bytes or non-AVX2 CPUs
    for (; i < len && count < max_indexes; i++) {
        char c = buf[i];
        if (c == '{' || c == '}' || c == '[' || c == ']' || c == ':' || c == ',' || c == '"') {
            index_out[count++] = (uint32_t)i;
        }
    }

    return count;
}

vit_simd_json_doc_t* vit_simd_json_parse(const char* json_str, size_t length) {
    if (!json_str || length == 0) return NULL;

    // VRI-10: Use thread-local arena for doc + tokens to avoid 3x malloc per request
    // at high throughput (300K+ RPS). Arena is reset per-request by caller or auto-reset.
    vit_simd_json_doc_t* doc = (vit_simd_json_doc_t*)vit_thread_arena_alloc(sizeof(vit_simd_json_doc_t));
    if (!doc) {
        // Arena full — fallback to heap
        doc = (vit_simd_json_doc_t*)malloc(sizeof(vit_simd_json_doc_t));
        if (!doc) return NULL;
    }

    doc->json_src = json_str;
    doc->json_len = length;
    // Cap token capacity: at most length structural chars, max 4096
    size_t token_cap = length < 4096 ? length : 4096;
    if (token_cap < 64) token_cap = 64;
    doc->token_capacity = token_cap;
    doc->tokens = (vit_json_token_t*)vit_thread_arena_alloc(token_cap * sizeof(vit_json_token_t));
    if (!doc->tokens) {
        doc->tokens = (vit_json_token_t*)malloc(token_cap * sizeof(vit_json_token_t));
        if (!doc->tokens) return NULL;
    }
    doc->token_count = 0;

    // VRI-10: struct_indexes uses stack for small JSON (<=4096 chars), heap for large
    // This avoids malloc(length * sizeof(uint32_t)) with potentially huge length.
    uint32_t stack_indexes[4096];
    uint32_t* struct_indexes;
    size_t max_indexes = length < 4096 ? length : 4096;
    if (max_indexes <= 4096) {
        struct_indexes = stack_indexes;
    } else {
        struct_indexes = (uint32_t*)malloc(max_indexes * sizeof(uint32_t));
        if (!struct_indexes) return NULL;
    }

    size_t num_structs = vit_simd_json_find_structural_indexes(json_str, length, struct_indexes, max_indexes);

    for (size_t k = 0; k < num_structs && doc->token_count < doc->token_capacity; k++) {
        uint32_t idx = struct_indexes[k];
        char c = json_str[idx];
        vit_json_token_t tok;
        tok.offset = idx;
        tok.length = 1;

        switch (c) {
            case '{': tok.type = VIT_JSON_TOK_OBJECT_START; break;
            case '}': tok.type = VIT_JSON_TOK_OBJECT_END; break;
            case '[': tok.type = VIT_JSON_TOK_ARRAY_START; break;
            case ']': tok.type = VIT_JSON_TOK_ARRAY_END; break;
            case ':': tok.type = VIT_JSON_TOK_COLON; break;
            case ',': tok.type = VIT_JSON_TOK_COMMA; break;
            case '"': tok.type = VIT_JSON_TOK_STRING; break;
            default: tok.type = VIT_JSON_TOK_NONE; break;
        }
        doc->tokens[doc->token_count++] = tok;
    }

    if (struct_indexes != stack_indexes) {
        free(struct_indexes);
    }
    return doc;
}

void vit_simd_json_free(vit_simd_json_doc_t* doc) {
    if (!doc) return;
    if (doc->tokens) free(doc->tokens);
    free(doc);
}

size_t vit_simd_json_stringify_fast(const vit_simd_json_doc_t* doc, char* out_buf, size_t out_cap) {
    if (!doc || !out_buf || out_cap < doc->json_len) return 0;
    memcpy(out_buf, doc->json_src, doc->json_len);
    if (doc->json_len < out_cap) out_buf[doc->json_len] = '\0';
    return doc->json_len;
}

const char* vit_simd_json_get_field(const vit_simd_json_doc_t* doc, const char* key, size_t* val_len_out) {
    if (!doc || !key || !doc->tokens) return NULL;
    size_t key_len = strlen(key);

    for (size_t i = 0; i + 2 < doc->token_count; i++) {
        if (doc->tokens[i].type == VIT_JSON_TOK_STRING) {
            uint32_t str_offset = doc->tokens[i].offset + 1; // skip leading "
            if (str_offset + key_len <= doc->json_len &&
                strncmp(doc->json_src + str_offset, key, key_len) == 0 &&
                doc->json_src[str_offset + key_len] == '"') {
                
                // Found matching key token, look for subsequent COLON and value
                if (doc->tokens[i + 1].type == VIT_JSON_TOK_COLON) {
                    uint32_t val_start = doc->tokens[i + 2].offset;
                    uint32_t val_end = doc->json_len;
                    if (i + 3 < doc->token_count) {
                        val_end = doc->tokens[i + 3].offset;
                    }
                    if (val_len_out) *val_len_out = (size_t)(val_end - val_start);
                    return doc->json_src + val_start;
                }
            }
        }
    }
    return NULL;
}
