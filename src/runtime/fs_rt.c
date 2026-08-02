#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#define F_OK 0
#else
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
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

double __vit_fs_mkdir(const char* path) {
    if (!path) return 0.0;
#ifdef _WIN32
    return (CreateDirectoryA(path, NULL) || GetLastError() == ERROR_ALREADY_EXISTS) ? 1.0 : 0.0;
#else
    return (mkdir(path, 0755) == 0) ? 1.0 : 0.0;
#endif
}

char* __vit_fs_read_dir(const char* path) {
    if (!path || strlen(path) == 0) return strdup("");
#ifdef _WIN32
    char searchPath[MAX_PATH];
    snprintf(searchPath, sizeof(searchPath), "%s\\*", path);
    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(searchPath, &fd);
    if (hFind == INVALID_HANDLE_VALUE) return strdup("");

    size_t cap = 1024;
    size_t len = 0;
    char* buf = (char*)malloc(cap);
    if (!buf) { FindClose(hFind); return strdup(""); }
    buf[0] = '\0';

    do {
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) continue;
        size_t nameLen = strlen(fd.cFileName);
        if (len + nameLen + 2 > cap) {
            cap *= 2;
            buf = (char*)realloc(buf, cap);
        }
        if (len > 0) {
            strcat(buf, "\n");
            len++;
        }
        strcat(buf, fd.cFileName);
        len += nameLen;
    } while (FindNextFileA(hFind, &fd));

    FindClose(hFind);
    return buf;
#else
    DIR* dir = opendir(path);
    if (!dir) return strdup("");

    size_t cap = 1024;
    size_t len = 0;
    char* buf = (char*)malloc(cap);
    if (!buf) { closedir(dir); return strdup(""); }
    buf[0] = '\0';

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        size_t nameLen = strlen(entry->d_name);
        if (len + nameLen + 2 > cap) {
            cap *= 2;
            buf = (char*)realloc(buf, cap);
        }
        if (len > 0) {
            strcat(buf, "\n");
            len++;
        }
        strcat(buf, entry->d_name);
        len += nameLen;
    }
    closedir(dir);
    return buf;
#endif
}

