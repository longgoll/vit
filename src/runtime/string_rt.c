#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#ifdef __cplusplus
#include <regex>
extern "C" {
double __vit_str_regex_match(const char* str, const char* pattern) {
    if (!str || !pattern) return 0.0;
    try {
        std::regex reg(pattern, std::regex::ECMAScript);
        bool match = std::regex_match(str, reg);
        return match ? 1.0 : 0.0;
    } catch (...) {
        return 0.0;
    }
}
}
#else
static int __vit_match_cls(char c, const char* cls, size_t clsLen) {
    if (clsLen == 0) return 0;
    if (clsLen == 2 && cls[0] == '\\' && cls[1] == 'd') return (c >= '0' && c <= '9');
    if (clsLen == 2 && cls[0] == '\\' && cls[1] == 'D') return !(c >= '0' && c <= '9');
    if (clsLen == 2 && cls[0] == '\\' && cls[1] == 'w') return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_');
    if (clsLen == 2 && cls[0] == '\\' && cls[1] == 's') return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
    if (clsLen == 1 && cls[0] == '.') return 1;

    if (cls[0] == '[' && cls[clsLen - 1] == ']') {
        int negate = 0;
        size_t idx = 1;
        if (cls[1] == '^') { negate = 1; idx = 2; }
        int match = 0;
        while (idx < clsLen - 1) {
            if (idx + 2 < clsLen - 1 && cls[idx + 1] == '-') {
                char start = cls[idx];
                char end = cls[idx + 2];
                if (c >= start && c <= end) { match = 1; break; }
                idx += 3;
            } else {
                if (c == cls[idx]) { match = 1; break; }
                idx++;
            }
        }
        return negate ? !match : match;
    }
    return 0;
}

double __vit_str_regex_match(const char* str, const char* pattern) {
    if (!str || !pattern) return 0.0;
    size_t sLen = strlen(str);
    if (sLen == 0) return 0.0;

    // Normalize pattern by removing ^ and $ anchors if present
    const char* pat = pattern;
    size_t pLen = strlen(pattern);
    if (pLen > 0 && pat[0] == '^') { pat++; pLen--; }
    if (pLen > 0 && pat[pLen - 1] == '$') { pLen--; }

    if (pLen == 0) return 1.0;

    // Case 1: \d+ or [0-9]+
    if ((pLen == 2 && pat[0] == '\\' && pat[1] == 'd') ||
        (pLen == 3 && pat[0] == '\\' && pat[1] == 'd' && pat[2] == '+') ||
        (pLen == 6 && strncmp(pat, "[0-9]+", 6) == 0) ||
        (pLen == 5 && strncmp(pat, "[0-9]", 5) == 0)) {
        for (size_t i = 0; i < sLen; i++) {
            if (str[i] < '0' || str[i] > '9') return 0.0;
        }
        return 1.0;
    }

    // Case 2: Bracket expression with + quantifier, e.g. [a-z0-9-]+ or [a-zA-Z]+
    if (pat[0] == '[' && pat[pLen - 1] == '+' && pat[pLen - 2] == ']') {
        for (size_t i = 0; i < sLen; i++) {
            if (!__vit_match_cls(str[i], pat, pLen - 1)) return 0.0;
        }
        return 1.0;
    }

    // Case 3: Bracket expression without quantifier, e.g. [a-z0-9_-]
    if (pat[0] == '[' && pat[pLen - 1] == ']') {
        for (size_t i = 0; i < sLen; i++) {
            if (!__vit_match_cls(str[i], pat, pLen)) return 0.0;
        }
        return 1.0;
    }

    // Case 4: Literal string or substring match fallback
    char tempPat[256];
    if (pLen < sizeof(tempPat)) {
        memcpy(tempPat, pat, pLen);
        tempPat[pLen] = '\0';
        return (strstr(str, tempPat) != NULL) ? 1.0 : 0.0;
    }

    return (strstr(str, pat) != NULL) ? 1.0 : 0.0;
}
#endif

char* __vit_str_char_at(const char* str, double index) {
    if (!str) return strdup("");
    size_t idx = (size_t)index;
    size_t len = strlen(str);
    if (idx >= len) return strdup("");

    char* res = (char*)malloc(2);
    res[0] = str[idx];
    res[1] = '\0';
    return res;
}

double __vit_str_index_of(const char* str, const char* sub) {
    if (!str || !sub) return -1.0;
    const char* pos = strstr(str, sub);
    if (!pos) return -1.0;
    return (double)(pos - str);
}

char* __vit_str_substring(const char* str, double start, double length) {
    if (!str) return strdup("");
    size_t len = strlen(str);
    size_t st = (size_t)start;
    if (st >= len) return strdup("");

    size_t reqLen = (size_t)length;
    if (st + reqLen > len) {
        reqLen = len - st;
    }

    char* res = (char*)malloc(reqLen + 1);
    memcpy(res, str + st, reqLen);
    res[reqLen] = '\0';
    return res;
}

double __vit_str_starts_with(const char* str, const char* prefix) {
    if (!str || !prefix) return 0.0;
    size_t lenStr = strlen(str);
    size_t lenPre = strlen(prefix);
    if (lenPre > lenStr) return 0.0;
    return (strncmp(str, prefix, lenPre) == 0) ? 1.0 : 0.0;
}

double __vit_str_ends_with(const char* str, const char* suffix) {
    if (!str || !suffix) return 0.0;
    size_t lenStr = strlen(str);
    size_t lenSuf = strlen(suffix);
    if (lenSuf > lenStr) return 0.0;
    return (strcmp(str + lenStr - lenSuf, suffix) == 0) ? 1.0 : 0.0;
}

char* __vit_str_trim(const char* str) {
    if (!str) return strdup("");
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return strdup("");

    const char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    size_t len = (size_t)(end - str + 1);
    char* res = (char*)malloc(len + 1);
    memcpy(res, str, len);
    res[len] = '\0';
    return res;
}

char* __vit_str_replace(const char* str, const char* target, const char* replacement) {
    if (!str || !target || !replacement) return strdup(str ? str : "");
    size_t targetLen = strlen(target);
    if (targetLen == 0) return strdup(str);

    size_t count = 0;
    const char* tmp = str;
    while ((tmp = strstr(tmp, target))) {
        count++;
        tmp += targetLen;
    }

    size_t replLen = strlen(replacement);
    size_t newLen = strlen(str) + count * (replLen - targetLen);

    char* result = (char*)malloc(newLen + 1);
    if (!result) return strdup(str);

    char* ins = result;
    tmp = str;
    while (1) {
        const char* pos = strstr(tmp, target);
        if (!pos) {
            strcpy(ins, tmp);
            break;
        }
        size_t len = (size_t)(pos - tmp);
        memcpy(ins, tmp, len);
        ins += len;
        memcpy(ins, replacement, replLen);
        ins += replLen;
        tmp = pos + targetLen;
    }

    return result;
}

static int __vit_rand_initialized = 0;

double __vit_math_random() {
    if (!__vit_rand_initialized) {
        srand((unsigned int)time(NULL));
        __vit_rand_initialized = 1;
    }
    return (double)rand() / (double)RAND_MAX;
}
