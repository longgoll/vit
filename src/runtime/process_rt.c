#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* __vit_process_exec(const char* command) {
    if (!command || strlen(command) == 0) return strdup("");

#ifdef _WIN32
    FILE* pipe = _popen(command, "r");
#else
    FILE* pipe = popen(command, "r");
#endif

    if (!pipe) return strdup("");

    size_t cap = 2048;
    size_t len = 0;
    char* buf = (char*)malloc(cap);
    if (!buf) {
#ifdef _WIN32
        _pclose(pipe);
#else
        pclose(pipe);
#endif
        return strdup("");
    }

    char chunk[256];
    buf[0] = '\0';
    while (fgets(chunk, sizeof(chunk), pipe) != NULL) {
        size_t chunkLen = strlen(chunk);
        if (len + chunkLen + 1 > cap) {
            cap *= 2;
            buf = (char*)realloc(buf, cap);
        }
        strcat(buf, chunk);
        len += chunkLen;
    }

#ifdef _WIN32
    _pclose(pipe);
#else
    pclose(pipe);
#endif

    return buf;
}
