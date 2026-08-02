#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void normalize_separators(char* str) {
    if (!str) return;
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\\') {
            str[i] = '/';
        }
    }
}

char* __vit_path_join(const char* p1, const char* p2) {
    if (!p1 && !p2) return strdup("");
    if (!p1) return strdup(p2);
    if (!p2) return strdup(p1);

    size_t len1 = strlen(p1);
    size_t len2 = strlen(p2);

    char* buf = (char*)malloc(len1 + len2 + 2);
    if (!buf) return strdup("");

    strcpy(buf, p1);
    normalize_separators(buf);

    size_t curLen = strlen(buf);
    if (curLen > 0 && buf[curLen - 1] != '/' && p2[0] != '/' && p2[0] != '\\') {
        strcat(buf, "/");
    }

    char* tempP2 = strdup(p2);
    normalize_separators(tempP2);

    const char* startP2 = tempP2;
    if ((curLen > 0 && buf[curLen - 1] == '/') && (startP2[0] == '/')) {
        startP2++;
    }

    strcat(buf, startP2);
    free(tempP2);
    return buf;
}

char* __vit_path_basename(const char* path) {
    if (!path || strlen(path) == 0) return strdup("");
    char* temp = strdup(path);
    normalize_separators(temp);

    char* lastSlash = strrchr(temp, '/');
    char* res;
    if (lastSlash) {
        res = strdup(lastSlash + 1);
    } else {
        res = strdup(temp);
    }
    free(temp);
    return res;
}

char* __vit_path_extname(const char* path) {
    if (!path || strlen(path) == 0) return strdup("");
    char* base = __vit_path_basename(path);
    char* lastDot = strrchr(base, '.');
    char* res;
    if (lastDot && lastDot != base) {
        res = strdup(lastDot);
    } else {
        res = strdup("");
    }
    free(base);
    return res;
}

char* __vit_path_dirname(const char* path) {
    if (!path || strlen(path) == 0) return strdup(".");
    char* temp = strdup(path);
    normalize_separators(temp);

    // Remove trailing slash if any
    size_t len = strlen(temp);
    while (len > 1 && temp[len - 1] == '/') {
        temp[len - 1] = '\0';
        len--;
    }

    char* lastSlash = strrchr(temp, '/');
    char* res;
    if (lastSlash) {
        if (lastSlash == temp) {
            res = strdup("/");
        } else {
            *lastSlash = '\0';
            res = strdup(temp);
        }
    } else {
        res = strdup(".");
    }
    free(temp);
    return res;
}
