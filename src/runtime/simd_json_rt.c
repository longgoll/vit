#include "simd_json_rt.h"
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

    vit_simd_json_doc_t* doc = (vit_simd_json_doc_t*)malloc(sizeof(vit_simd_json_doc_t));
    if (!doc) return NULL;

    doc->json_src = json_str;
    doc->json_len = length;
    doc->token_capacity = length > 64 ? length : 64;
    doc->tokens = (vit_json_token_t*)malloc(doc->token_capacity * sizeof(vit_json_token_t));
    doc->token_count = 0;

    uint32_t* struct_indexes = (uint32_t*)malloc(length * sizeof(uint32_t));
    if (!struct_indexes) {
        free(doc->tokens);
        free(doc);
        return NULL;
    }

    size_t num_structs = vit_simd_json_find_structural_indexes(json_str, length, struct_indexes, length);

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

    free(struct_indexes);
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
