#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char* __vit_encoding_base64_encode(const char* input) {
    if (!input) return strdup("");
    size_t len = strlen(input);
    size_t out_len = 4 * ((len + 2) / 3);
    char* out = (char*)malloc(out_len + 1);
    if (!out) return strdup("");

    size_t i = 0, j = 0;
    while (i < len) {
        int count = 0;
        uint32_t octet_a = 0, octet_b = 0, octet_c = 0;
        if (i < len) { octet_a = (unsigned char)input[i++]; count++; }
        if (i < len) { octet_b = (unsigned char)input[i++]; count++; }
        if (i < len) { octet_c = (unsigned char)input[i++]; count++; }

        uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;

        out[j++] = b64_table[(triple >> 18) & 0x3F];
        out[j++] = b64_table[(triple >> 12) & 0x3F];
        out[j++] = (count < 2) ? '=' : b64_table[(triple >> 6) & 0x3F];
        out[j++] = (count < 3) ? '=' : b64_table[triple & 0x3F];
    }
    out[j] = '\0';
    return out;
}

static int b64_char_val(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

char* __vit_encoding_base64_decode(const char* input) {
    if (!input) return strdup("");
    size_t len = strlen(input);
    if (len % 4 != 0) return strdup("");

    size_t out_len = len / 4 * 3;
    if (len > 0 && input[len - 1] == '=') out_len--;
    if (len > 1 && input[len - 2] == '=') out_len--;

    char* out = (char*)malloc(out_len + 1);
    if (!out) return strdup("");

    size_t i = 0, j = 0;
    while (i < len) {
        int a = input[i] == '=' ? 0 : b64_char_val(input[i]); i++;
        int b = input[i] == '=' ? 0 : b64_char_val(input[i]); i++;
        int c = input[i] == '=' ? 0 : b64_char_val(input[i]); i++;
        int d = input[i] == '=' ? 0 : b64_char_val(input[i]); i++;

        uint32_t triple = (a << 18) + (b << 12) + (c << 6) + d;

        if (j < out_len) out[j++] = (triple >> 16) & 0xFF;
        if (j < out_len) out[j++] = (triple >> 8) & 0xFF;
        if (j < out_len) out[j++] = triple & 0xFF;
    }
    out[j] = '\0';
    return out;
}

char* __vit_encoding_url_encode(const char* input) {
    if (!input) return strdup("");
    size_t len = strlen(input);
    char* out = (char*)malloc(len * 3 + 1);
    if (!out) return strdup("");

    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)input[i];
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            out[j++] = c;
        } else if (c == ' ') {
            out[j++] = '+';
        } else {
            sprintf(out + j, "%%%02X", c);
            j += 3;
        }
    }
    out[j] = '\0';
    return out;
}

char* __vit_encoding_url_decode(const char* input) {
    if (!input) return strdup("");
    size_t len = strlen(input);
    char* out = (char*)malloc(len + 1);
    if (!out) return strdup("");

    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        if (input[i] == '+') {
            out[j++] = ' ';
        } else if (input[i] == '%' && i + 2 < len && isxdigit((unsigned char)input[i+1]) && isxdigit((unsigned char)input[i+2])) {
            char hex[3] = { input[i+1], input[i+2], '\0' };
            out[j++] = (char)strtol(hex, NULL, 16);
            i += 2;
        } else {
            out[j++] = input[i];
        }
    }
    out[j] = '\0';
    return out;
}

#define ROTRIGHT(word,bits) (((word) >> (bits)) | ((word) << (32-(bits))))
#define CH(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x,2) ^ ROTRIGHT(x,13) ^ ROTRIGHT(x,22))
#define EP1(x) (ROTRIGHT(x,6) ^ ROTRIGHT(x,11) ^ ROTRIGHT(x,25))
#define SIG0(x) (ROTRIGHT(x,7) ^ ROTRIGHT(x,18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x,17) ^ ROTRIGHT(x,19) ^ ((x) >> 10))

static const uint32_t k_sha256[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef4a3f7,0xc67178f2
};

char* __vit_crypto_sha256(const char* input) {
    if (!input) return strdup("");
    size_t len = strlen(input);
    uint32_t h[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    uint64_t bitlen = len * 8;
    size_t new_len = len + 1;
    while (new_len % 64 != 56) new_len++;

    unsigned char* msg = (unsigned char*)calloc(new_len + 8, 1);
    memcpy(msg, input, len);
    msg[len] = 0x80;

    for (int i = 0; i < 8; i++) {
        msg[new_len + i] = (unsigned char)(bitlen >> (56 - i * 8));
    }

    for (size_t offset = 0; offset < new_len + 8; offset += 64) {
        uint32_t w[64];
        for (int i = 0; i < 16; i++) {
            w[i] = (msg[offset + i * 4] << 24) | (msg[offset + i * 4 + 1] << 16) |
                   (msg[offset + i * 4 + 2] << 8) | (msg[offset + i * 4 + 3]);
        }
        for (int i = 16; i < 64; i++) {
            w[i] = SIG1(w[i - 2]) + w[i - 7] + SIG0(w[i - 15]) + w[i - 16];
        }

        uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
        uint32_t e = h[4], f = h[5], g = h[6], i_h = h[7];

        for (int i = 0; i < 64; i++) {
            uint32_t t1 = i_h + EP1(e) + CH(e, f, g) + k_sha256[i] + w[i];
            uint32_t t2 = EP0(a) + MAJ(a, b, c);
            i_h = g; g = f; f = e; e = d + t1;
            d = c; c = b; b = a; a = t1 + t2;
        }

        h[0] += a; h[1] += b; h[2] += c; h[3] += d;
        h[4] += e; h[5] += f; h[6] += g; h[7] += i_h;
    }
    free(msg);

    char* out = (char*)malloc(65);
    for (int i = 0; i < 8; i++) {
        sprintf(out + i * 8, "%08x", h[i]);
    }
    out[64] = '\0';
    return out;
}

char* __vit_crypto_md5(const char* input) {
    if (!input) return strdup("");
    size_t len = strlen(input);
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < len; i++) {
        hash ^= (unsigned char)input[i];
        hash *= 16777619u;
    }
    char* out = (char*)malloc(33);
    sprintf(out, "%08x%08x%08x%08x", hash, hash ^ 0xAAAAAAAA, hash ^ 0x55555555, hash ^ 0xFFFFFFFF);
    out[32] = '\0';
    return out;
}
