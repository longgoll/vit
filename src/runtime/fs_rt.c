#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <io.h>
#define F_OK 0
#else
#include <unistd.h>
#endif

char* __vit_fs_read_file(const char* path) {
    if (!path) return strdup("");
    FILE* f = fopen(path, "rb");
    if (!f) return strdup("");

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (sz < 0) {
        fclose(f);
        return strdup("");
    }

    char* buf = (char*)malloc(sz + 1);
    if (!buf) {
        fclose(f);
        return strdup("");
    }

    size_t readBytes = fread(buf, 1, sz, f);
    buf[readBytes] = '\0';
    fclose(f);
    return buf;
}

double __vit_fs_write_file(const char* path, const char* content) {
    if (!path || !content) return 0.0;
    FILE* f = fopen(path, "wb");
    if (!f) return 0.0;

    size_t len = strlen(content);
    size_t written = fwrite(content, 1, len, f);
    fclose(f);
    return (written == len) ? 1.0 : 0.0;
}

double __vit_fs_append_file(const char* path, const char* content) {
    if (!path || !content) return 0.0;
    FILE* f = fopen(path, "ab");
    if (!f) return 0.0;

    size_t len = strlen(content);
    size_t written = fwrite(content, 1, len, f);
    fclose(f);
    return (written == len) ? 1.0 : 0.0;
}

double __vit_fs_exists(const char* path) {
    if (!path) return 0.0;
#ifdef _WIN32
    return (_access(path, F_OK) == 0) ? 1.0 : 0.0;
#else
    return (access(path, F_OK) == 0) ? 1.0 : 0.0;
#endif
}

double __vit_fs_remove(const char* path) {
    if (!path) return 0.0;
    return (remove(path) == 0) ? 1.0 : 0.0;
}

double __vit_fs_size(const char* path) {
    if (!path) return 0.0;
    struct stat st;
    if (stat(path, &st) == 0) {
        return (double)st.st_size;
    }
    return 0.0;
}
