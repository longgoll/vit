#ifndef VIT_SIMD_JSON_RT_H
#define VIT_SIMD_JSON_RT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Structural Indexing Token Types
typedef enum {
    VIT_JSON_TOK_NONE = 0,
    VIT_JSON_TOK_OBJECT_START, // {
    VIT_JSON_TOK_OBJECT_END,   // }
    VIT_JSON_TOK_ARRAY_START,  // [
    VIT_JSON_TOK_ARRAY_END,    // ]
    VIT_JSON_TOK_COLON,        // :
    VIT_JSON_TOK_COMMA,        // ,
    VIT_JSON_TOK_STRING,       // "..."
    VIT_JSON_TOK_NUMBER,       // 123.45
    VIT_JSON_TOK_BOOLEAN,      // true/false
    VIT_JSON_TOK_NULL          // null
} vit_json_type_t;

typedef struct {
    uint32_t offset;
    uint32_t length;
    vit_json_type_t type;
} vit_json_token_t;

typedef struct {
    const char* json_src;
    size_t json_len;
    vit_json_token_t* tokens;
    size_t token_capacity;
    size_t token_count;
} vit_simd_json_doc_t;

// API functions
vit_simd_json_doc_t* vit_simd_json_parse(const char* json_str, size_t length);
void vit_simd_json_free(vit_simd_json_doc_t* doc);

// SIMD Structural Indexing Scanner
size_t vit_simd_json_find_structural_indexes(const char* buf, size_t len, uint32_t* index_out, size_t max_indexes);

// High Performance Zero-Allocation Stringifier
size_t vit_simd_json_stringify_fast(const vit_simd_json_doc_t* doc, char* out_buf, size_t out_cap);

// Zero-Allocation Direct Field Lookup
const char* vit_simd_json_get_field(const vit_simd_json_doc_t* doc, const char* key, size_t* val_len_out);

#ifdef __cplusplus
}
#endif

#endif // VIT_SIMD_JSON_RT_H
